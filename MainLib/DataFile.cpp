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

#include "DataFile.h"

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QXmlStreamWriter>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QAbstractMessageHandler>
#include <QProcess>
#include <QDebug>
#include <QEventLoop>
#include <QProgressDialog>

#include <optional>

namespace NLoadProdSync
{
    class CMsgHandler : public QAbstractMessageHandler
    {
    public:
        virtual void handleMessage( QtMsgType /*type*/, const QString & desc, const QUrl & identifier, const QSourceLocation & location ) override
        {
            qDebug() << "Desc: " << desc << " Url: " << identifier << " Location: " << location;
        }

    };

    void dumpStatus( const QString & status )
    {
        qDebug() << "Status: " << qPrintable( status );
    }


    bool CDataFile::save( QWidget * parent )
    {
        auto fn = fFileName;
        auto fi = QFileInfo( fn );
        auto newFN = QDir( fi.path() ).absoluteFilePath( fi.baseName() + ".xml" );

        fi = QFileInfo( newFN );
        if ( fi.exists() )
        {
            QFile::rename( newFN, newFN + ".bak" );
        }
        QFile file( newFN );
        if ( !file.open( QIODevice::Text | QIODevice::WriteOnly ) )
        {
            QMessageBox::critical( parent, "Could not open file for write", QString( "File: %1 could not be opened" ).arg( newFN ) );
            return false;
        }
        QXmlStreamWriter writer( &file );
        writer.setAutoFormatting( true );
        writer.writeStartDocument();
        writer.writeStartElement( "directories" );
        for ( auto && ii : fDirs )
        {
            writer.writeStartElement( "dir" );
            writer.writeAttribute( "path", ii->fPath );
            for ( auto && ii : ii->fExcludes )
                writer.writeTextElement( "exclude", ii );
            for ( auto && ii : ii->fExtras )
                writer.writeTextElement( "extra", ii );
            writer.writeEndElement();
        }
        writer.writeEndElement();
        writer.writeEndDocument();

        return true;
    }


    void CDataFile::stop()
    {
        for ( auto && ii : fDirs )
            ii->stop();
    }

    std::pair< int, QString > CDataFile::run( const QString & uProdDir, const QString & server, const QString & localProdDir, const QString & rsyncExec, const QString & bashExec, bool verbose, bool norun, ERSyncType drivePrefix, QProgressDialog * progress, std::function< void( const QString & ) > msgFunc ) const
    {
        QFileInfo fi( localProdDir );
        if ( !fi.exists() )
        {
            QDir dir( localProdDir );
            if ( !dir.mkpath( "." ) )
            {
                return { false, QString( "Could not create path '%1'" ).arg( localProdDir ) };
            }
        }

        if ( !fi.exists() )
        {
            return { false, QString( "'%1' does not exist" ).arg( localProdDir ) };
        }

        if ( !fi.exists() || !fi.isDir() || !fi.isWritable() || !fi.isExecutable() )
        {
            return { false, QString( "'%1' exists, but is either not a directory, not writable or you do not have executable permissions" ).arg( localProdDir ) };
        }

        fi = QFileInfo( rsyncExec );
        if ( !fi.exists() || !fi.isExecutable() )
        {
            return { false, QString( "'%1' exists, but is either not a executable" ).arg( rsyncExec ) };
        }

        bool aOK;
        QString realRsyncExec;
        QString realBashExec;
        if ( drivePrefix != NLoadProdSync::ERSyncType::eNative )
        {
            if ( !fi.exists() || !fi.isExecutable() )
            {
                return { false, QString( "'%1' exists, but is either not a executable" ).arg( rsyncExec ) };
            }

            std::tie( aOK, realRsyncExec ) = getRealPath( rsyncExec, drivePrefix );
            if ( !aOK )
                return { aOK, realRsyncExec };
        }
        else
        {
            realRsyncExec = rsyncExec;
        }

        QString realProdDir;
        std::tie( aOK, realProdDir ) = getRealPath( localProdDir, drivePrefix );
        if ( !aOK )
            return { aOK, realProdDir };

        if ( verbose )
        {
            msgFunc( QString( "Syncing to localprod: %1" ).arg( localProdDir ) );
            msgFunc( QString( " uprod: %1:%2" ).arg( server ).arg( uProdDir ) );
            msgFunc( QString( " rync: %1" ).arg( rsyncExec ) );
            msgFunc( QString( " bash: %1" ).arg( bashExec ) );
        }

        /*
        Number of files: 17 (reg: 6, dir: 11)
        Number of created files: 0
        Number of regular files transferred: 0
        Total file size: 478,738 bytes
        Total transferred file size: 0 bytes
        Literal data: 0 bytes
        Matched data: 0 bytes
        File list size: 367
        File list generation time: 0.002 seconds
        File list transfer time: 0.000 seconds
        Total bytes sent: 73
        Total bytes received: 437

        sent 73 bytes  received 437 bytes  340.00 bytes/sec
        total size is 478,738  speedup is 938.70
        */

        for ( auto && ii : fDirs )
        {
            auto retVal = ii->run( uProdDir, server, realProdDir, realRsyncExec, bashExec, verbose, norun, msgFunc );
            if ( ( retVal.first != 0 ) && ( retVal.first != 23 ) )
                return retVal;
            if ( retVal.first == 23 )
                msgFunc( "WARNING: Partial transfer occurred please see above to verify results." );

            if ( progress )
                progress->setValue( progress->value() + 1 );
        }
        return { 0, QString() };
    }

    std::pair< bool, QString > CDataFile::getRealPath( const QString & path, ERSyncType drivePrefix ) const
    {
        QFileInfo fi( path );
        auto retVal = fi.absoluteFilePath();
        if ( drivePrefix != ERSyncType::eNative )
        {
            if ( retVal.length() < 3 )
            {
                return { false, QString( "'%1' exists, but could not determine the appropriate drive prefix from a native path" ).arg( retVal ) };
            }

            QString drive = retVal[ 0 ];
            retVal = retVal.mid( 3 );
            if ( drivePrefix == ERSyncType::eMSys64 )
                retVal = "/" + drive + "/" + retVal;
            else if ( drivePrefix == ERSyncType::eCygwin )
                retVal = "/cygdrive/" + drive + "/" + retVal;
        }
        return { true, retVal };
    }

    std::optional< QStringList > getStringList( const QString & tagName, const QJsonValue & value )
    {
        QStringList retVal;
        if ( !value[ tagName ].isUndefined() )
        {
            if ( !value[ tagName ].isArray() )
            {
                return {};
            }
            auto currObj = value[ tagName ].toArray();
            for ( auto && curr : currObj )
            {
                retVal << curr.toString();
            }
        }
        return retVal;
    }

    bool CDataFile::loadFromXML( QWidget * parent )
    {
        QXmlQuery query;
        query.setMessageHandler( fMsgHandler );

        auto file = QFile( fFileName );
        if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) || !query.setFocus( &file ) )
        {
            QMessageBox::warning( parent, "Could not open", QString( "'%1' could not be opened" ).arg( fFileName ) );
            return false;
        }

        query.setQuery( "directories/dir/@path/string()" );
        if ( !query.isValid() )
            return false;

        QStringList paths;
        query.evaluateTo( &paths );
        for ( auto && ii : paths )
        {
            ii = ii.trimmed();
        }

        for ( const auto & path : paths )
        {
            auto queryString = QString( "directories/dir[@path='%1']/extra/string()" ).arg( path );
            query.setQuery( queryString );
            if ( !query.isValid() )
                return false;

            QStringList extras;
            query.evaluateTo( &extras );

            queryString = QString( "directories/dir[@path='%1']/exclude/string()" ).arg( path );
            query.setQuery( queryString );
            if ( !query.isValid() )
                return false;

            QStringList excludes;
            query.evaluateTo( &excludes );
            fDirs.push_back( std::make_shared< SDirectory >( path, excludes, extras ) );
        }

        return true;
    }

    bool CDataFile::loadFromJSON( QWidget * parent )
    {
        auto file = QFile( fFileName );
        if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            QMessageBox::warning( parent, "Could not open", QString( "'%1' could not be opened" ).arg( fFileName ) );
            return false;
        }
        auto jsonText = file.readAll();
        file.close();
        auto jsonData = QJsonDocument::fromJson( jsonText );
        auto dirObj = jsonData[ "directories" ];
        if ( !dirObj.isArray() )
        {
            QMessageBox::warning( parent, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fFileName ) );
            return false;
        }

        auto dirs = dirObj.toArray();
        for ( int ii = 0; ii < dirs.count(); ++ii )
        {
            const auto && currDir = dirs.at( ii );
            if ( currDir[ "src" ].isNull() )
            {
                QMessageBox::warning( parent, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fFileName ) );
                return false;
            }
            auto src = currDir[ "src" ].toString();

            auto excludes = getStringList( "exclude", currDir );
            auto extras = getStringList( "extra", currDir );
            if ( !excludes.has_value() || !extras.has_value() )
            {
                QMessageBox::warning( parent, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fFileName ) );
                return false;
            }
            fDirs.push_back( std::make_shared< SDirectory >( src, excludes.value(), extras.value() ) );
        }
        return true;
    }

    CDataFile::CDataFile() :
        fMsgHandler( new CMsgHandler )
    {

    }

    CDataFile::~CDataFile()
    {
        delete fMsgHandler;
    }

    bool CDataFile::load( const QString & fileName, QWidget * parent )
    {
        if ( fileName.isEmpty() )
            return false;
        clear();
        fFileName = fileName;
        if ( QFileInfo( fFileName ).suffix() == "json" )
            return loadFromJSON( parent );
        else
            return loadFromXML( parent );
    }


    void runCmdInt( const QString & program, const QStringList & args, QProcess * process, QEventLoop * eLoop, const std::function< void( const QString & ) > & msgFunc )
    {
        // Set up output handling
        QObject::connect( process, &QProcess::readyReadStandardError, 
        [process, msgFunc]()
        {
            msgFunc( "Error: " + process->readAllStandardError() ); 
        }
        );
        QObject::connect( process, &QProcess::readyReadStandardOutput, 
        [process,msgFunc]()
        {
            msgFunc( process->readAllStandardOutput() );
        } );

        QObject::connect( process, &QProcess::errorOccurred,
                          [=]( QProcess::ProcessError error )
        {
            QString errorType;
            switch ( error )
            {
                case QProcess::FailedToStart: errorType = "Failed To Start"; break;
                case QProcess::Crashed: errorType = "Crashed"; break;
                case QProcess::Timedout: errorType = "Timed Out"; break;
                case QProcess::WriteError: errorType = "Write Error"; break;
                case QProcess::ReadError: errorType = "Read Error"; break;
                case QProcess::UnknownError: errorType = "Unknown Error"; break;
            }
            msgFunc( QString( "%1: %2" ).arg( errorType ).arg( process->errorString() ) );
        });

        QObject::connect( process, &QProcess::started,
        [=]()
        {
            dumpStatus( QString( "Process has started\n" ) );
        }
        );

        QObject::connect( process, &QProcess::stateChanged,
                          [=]( QProcess::ProcessState newState )
        {
            auto msg = QString( "State Changed To: %1(%2)\n" ).arg( ( newState == QProcess::NotRunning ) ? "Not Running" : ( ( newState == QProcess::Starting ) ? "Starting" : "Running" ) ).arg( newState );
            dumpStatus( msg );
        }
        );

        // Catch the signal when the process finishes
        QObject::connect( process,
                          QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
                          [=]( int exitCode, QProcess::ExitStatus exitStatus )
        {
            dumpStatus( QString( "Process Finished with ExitCode: %1 ExitStatus: %2\n" ).arg( exitCode ).arg( exitStatus ) );
            eLoop->exit( exitCode );
        } );

        // Start the process
        dumpStatus( QString( "Starting Process: Program %1 Args: %2\n" ).arg( program ).arg( args.join( " " ) ) );
        process->start( program, args );
    }

    std::pair< int, QString > runCmd( const QString & program, const QStringList & args, QProcess * process, QEventLoop * eventLoop, const std::function< void( const QString & msg ) > & msgFunc )
    {
        dumpStatus( "About to call runCmd with process/eventloop\n" );

        runCmdInt( program, args, process, eventLoop, msgFunc );

        dumpStatus( QString( "Returned from gRunShellCommandInBackground\n" ) );
        int exitCode = -1;
        if ( process->state() != QProcess::NotRunning )
        {
            dumpStatus( QString( "Had not exited yet, starting Eventloop\n" ) );
            exitCode = eventLoop->exec();
            dumpStatus( QString( "Eventloop exited with ExitCode: %1\n" ).arg( exitCode ) );
        }
        else
        {
            exitCode = process->exitCode();
            dumpStatus( QString( "Finished before return from gRunShellCommandInBackground with ExitCode: %1\n" ).arg( exitCode ) );
        }
        QString msg;
        if ( exitCode != 0 || ( process->error() != QProcess::UnknownError ) )
        {
            if ( exitCode == 0 )
                exitCode = -1;
            msg = QString( "Finished before return from  with ExitCode: %1 ErrorCode: %2 ErrorString: %3\n" ).arg( exitCode ).arg( process->error() ).arg( process->errorString() );
        }

        return { exitCode, msg };
    }

    SDirectory::SDirectory( const QString & path ) :
        fPath( path )
    {

    }

    SDirectory::SDirectory( const QString & path, const QStringList & excludes, const QStringList & extras ) :
        fPath( path ),
        fExcludes( excludes ),
        fExtras( extras )
    {

    }

    SDirectory::~SDirectory()
    {

    }

    void SDirectory::stop()
    {
        if ( fProcess )
            fProcess->kill();
    }

    std::pair< int, QString > SDirectory::run( const QString & uProdDir, const QString & server, const QString & localProdDir, const QString & rsyncExec, const QString & bashExec, bool verbose, bool norun, std::function< void( const QString & ) > msgFunc )
    {
        QStringList args;
        args << "-aRLvz" << "--stats";
        if ( verbose )
            args << "--info=progress2";
        args << fExtras;

        for ( auto && ii : fExcludes )
            args << "--exclude" << "\""+ii+"\"";

        args
            << QString( "%1:%2/./%3" ).arg( server ).arg( uProdDir ).arg( fPath )
            << localProdDir
            ;

        if ( verbose || norun )
        {
            if ( bashExec.isEmpty() )
                msgFunc( rsyncExec + " " + args.join( " " ) );
            else
                msgFunc( bashExec + "--login -c '" + rsyncExec + " " + args.join( " " ) + "'" );
        }
        fProcess = std::make_unique< QProcess >();
        fEventLoop = std::make_unique< QEventLoop >();
        std::pair< int, QString > retVal;
        if ( !norun )
        {
            if ( bashExec.isEmpty() )
                retVal = runCmd( rsyncExec, args, fProcess.get(), fEventLoop.get(), msgFunc );
            else
            {
                QStringList newArgs;
                newArgs << "--login" << "-c";
                args.push_front( rsyncExec );
                newArgs << args.join( " " );
                retVal = runCmd( bashExec, newArgs, fProcess.get(), fEventLoop.get(), msgFunc );
            }
        }
        else
            retVal = { 0, QString() };

        fProcess.reset();
        fEventLoop.reset();
        return retVal;
    }

}