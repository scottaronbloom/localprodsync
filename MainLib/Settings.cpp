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

 #include "Settings.h"
 #include <QApplication>
 #include <QProgressDialog>
 namespace NLoadProdSync
 {
     CSettings::CSettings()
     {
         qRegisterMetaTypeStreamOperators< NLoadProdSync::ERSyncType >( "NLoadProdSync::ERSyncType" );
         registerSettings();
         loadSettings();
     }

     CSettings::CSettings( const QString & fileName )
     {
         qRegisterMetaTypeStreamOperators< NLoadProdSync::ERSyncType >( "NLoadProdSync::ERSyncType" );
         registerSettings();
         loadSettings( fileName );
     }

     CSettings::~CSettings()
     {
         saveSettings();
     }

     void CSettings::clear()
     {
         setFileName( QString(), false );
         for ( auto && ii : fSettingsMap )
             *ii.second = QVariant();
     }

     QString CSettings::fileName() const
     {
         if ( !fSettingsFile )
             return QString();
         auto fname = fSettingsFile->fileName();
         if ( fname.startsWith( "\\HKEY" ) )
             return QString();
         return fname;
     }

     bool CSettings::saveSettings()
     {
         if ( !fSettingsFile )
             return false;

         for ( auto && ii : fSettingsMap )
             fSettingsFile->setValue( ii.first, *( ii.second ) );

         return true;
     }

     void CSettings::loadSettings()
     {
         loadSettings( QString() );
     }

     void CSettings::setFileName( const QString & fileName, bool andSave )
     {
         if ( fileName.isEmpty() )
             fSettingsFile = std::make_unique< QSettings >();
         else
             fSettingsFile = std::make_unique< QSettings >( fileName, QSettings::IniFormat );
         if ( andSave )
             saveSettings();
     }

     void CSettings::incProgress( QProgressDialog * progress ) const
     {
         if ( !progress )
             return;

         progress->setValue( progress->value() + 1 );
         qApp->processEvents();
     }

     bool CSettings::loadSettings( const QString & fileName )
     {
         saveSettings();
         setFileName( fileName, false );
         auto retVal = loadData();
         return retVal;
     }

     void CSettings::registerSettings()
     {
         ADD_SETTING_VALUE( QString, RemoteUProdDir );
         ADD_SETTING_VALUE( QString, LocalUProdDir );
         ADD_SETTING_VALUE( QString, LocalProdDir );
         ADD_SETTING_VALUE( QString, DataFile );

         ADD_SETTING_VALUE( QString, RsyncServer );
         ADD_SETTING_VALUE( QString, RsyncExec );
         ADD_SETTING_VALUE( QString, BashExec );
         ADD_SETTING_VALUE( bool, Verbose );
         ADD_SETTING_VALUE( bool, NoRun );
         ADD_SETTING_VALUE( NLoadProdSync::ERSyncType, RsyncType );
     }

     void CSettings::dump() const
     {
         for ( auto && ii : fSettingsMap )
         {
             qDebug() << ii.first << *ii.second;
         }
     }

     bool CSettings::loadData()
     {
         auto keys = fSettingsFile->childKeys();
         for ( auto && ii : keys )
         {
             auto pos = fSettingsMap.find( ii );
             if ( pos == fSettingsMap.end() )
                 continue;

             auto value = fSettingsFile->value( ii );
             *( *pos ).second = value;
         }
         return true;
     }

 }

 QDataStream & operator<<( QDataStream & stream, NLoadProdSync::ERSyncType value )
 {
     stream << (int)value;
     return stream;
 }

 QDataStream & operator>>( QDataStream & stream, NLoadProdSync::ERSyncType & value )
 {
     int tmp;
     stream >> tmp;

     value = static_cast<NLoadProdSync::ERSyncType>( tmp );
     return stream;
 }

