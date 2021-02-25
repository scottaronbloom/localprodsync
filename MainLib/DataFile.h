// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __DATAFILE_H
#define __DATAFILE_H

#include <QString>
#include <QStringList>
#include <QDebug>

#include <list>
#include <memory>

class QProcess;
class QEventLoop;
class QWidget;
namespace NLoadProdSync
{
    enum class EDrivePrefix
    {
        eNative,
        eCygwin,
        eMSys64
    };

    struct SDirectory
    {
        SDirectory( const QString & path );

        SDirectory( const QString & path, const QStringList & excludes, const QStringList & extras );

        ~SDirectory();

        void stop();
        std::pair< int, QString > run( const QString & uProdDir, const QString & server, const QString & localProdDir, const QString & rsyncExec, const QString & bashExec, bool verbose, bool norun, std::function< void( const QString & ) > msgFunc );

        QString fPath;
        QStringList fExcludes;
        QStringList fExtras;

        std::unique_ptr< QProcess > fProcess;
        std::unique_ptr< QEventLoop > fEventLoop;
    };

    class CMsgHandler;
    class CDataFile
    {
    public:
        CDataFile();
        ~CDataFile();
        bool load( const QString & fileName, QWidget * parent );
        bool save( QWidget * parent );

        void clear() { fDirs.clear(); }
        void addDir( std::shared_ptr< SDirectory > dir ) { fDirs.push_back( dir ); }
        QString fileName() const { return fFileName; }
        
        const std::list< std::shared_ptr< SDirectory > > & dirs() const { return fDirs; }

        void stop();
        std::pair< int, QString > run( const QString & uProdDir, const QString & server, const QString & localProdDir, const QString & rsyncExec, const QString & bashExec, bool verbose, bool norun, EDrivePrefix drivePrefix, std::function< void( const QString & ) > msgFunc ) const;

    private:
        std::pair<bool, QString> getRealPath( const QString & path, EDrivePrefix drivePrefix ) const;
        bool loadFromJSON( QWidget * parent );
        bool loadFromXML( QWidget * parent );
        QString fFileName;
        CMsgHandler * fMsgHandler{ nullptr };
        std::list< std::shared_ptr< SDirectory > > fDirs;
    };
};

#endif 
