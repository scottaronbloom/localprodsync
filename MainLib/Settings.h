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

#ifndef __SETTINGS_H
#define __SETTINGS_H

namespace NLoadProdSync { enum class ERSyncType; }
#include <QString>
#include <QSettings>
#include <functional>
#include <QVariant>
#include <QDebug>
class QProgressDialog;

#include <memory>

#define ADD_SETTING( Type, Name ) \
public: \
inline void set ## Name( const Type & value ){ qDebug() << #Name; f ## Name = QVariant::fromValue( value ); } \
inline [[nodiscard]] Type get ## Name( const Type & defValue = Type() ) const \
{ if ( f ## Name.isNull() ) return defValue; return f ## Name.value< Type >(); } \
inline [[nodiscard]] bool is ## Name ## Set() const { return !( f ## Name.isNull() ); } \
private: \
QVariant f ## Name

#define ADD_SETTING_VALUE( Type, Name ) \
registerSetting< Type >( #Name, const_cast<QVariant *>( &f ## Name ) );

namespace NLoadProdSync
{
    enum class ERSyncType
    {
        eNative,
        eCygwin,
        eMSys64
    };
    Q_DECLARE_METATYPE( ERSyncType );

    class CSettings
    {
    public:
        CSettings();
        CSettings( const QString & fileName );

        ~CSettings();

        void clear();
        bool saveSettings(); // only valid if filename has been set;
        void loadSettings(); // loads from the registry
        bool loadSettings( const QString & fileName ); // sets the filename and loads from it
        void setFileName( const QString & fileName, bool andSave = true );

        QString fileName() const;
        void dump() const;

        ADD_SETTING( QString, RemoteUProdDir );  // /u/prod
        ADD_SETTING( QString, LocalUProdDir );   // \\mgc\home\prod
        ADD_SETTING( QString, LocalProdDir );    // c:\localprod
        ADD_SETTING( QString, DataFile );        // P4_CLIENT_DIR/src/misc/WinLocalProdSyncData.xml
        ADD_SETTING( QString, RsyncServer );     // "local-rsync-vm1.wv.mentorg.com"
        ADD_SETTING( QString, RsyncExec );       // c:/msys64/usr/bin/rsync.exe
        ADD_SETTING( QString, BashExec );        // c:/msys64/usr/bin/bash.exec
        ADD_SETTING( bool, Verbose );            // true
        ADD_SETTING( bool, NoRun );              // false
        ADD_SETTING( NLoadProdSync::ERSyncType, RsyncType ); // NLoadProdSync::ERSyncType::eMSys64
    private:
        template< typename Type >
        void registerSetting( const QString & attribName, QVariant * value ) const
        {
            auto pos = fSettingsMap.find( attribName );
            if ( pos != fSettingsMap.end() )
                return;

            Q_ASSERT( value );
            if ( !value )
                return;
            //if ( !value->isValid() )
            //    *value = QVariant::fromValue( Type() );
            //qDebug() << attribName << *value;
            fSettingsMap[ attribName ] = value;
        }
        bool loadData();
        void incProgress( QProgressDialog * progress ) const;
        void registerSettings();

        std::unique_ptr< QSettings > fSettingsFile;
        mutable std::unordered_map< QString, QVariant * > fSettingsMap;
    };
}

QDataStream & operator<<( QDataStream & stream, NLoadProdSync::ERSyncType value );
QDataStream & operator>>( QDataStream & stream, NLoadProdSync::ERSyncType & value );

#endif 
