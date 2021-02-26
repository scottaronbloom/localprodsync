// The MIT License( MIT )
//
// Copyright( c ) 2020-2021 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Softwa , and to permit persons to whom the Software is
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

#include "MainWindow.h"
#include "AddAttribute.h"
#include "MainLib/DataFile.h"

#include "ui_MainWindow.h"

#include <QSettings>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QTimer>
#include <QItemSelection>
#include <QTextCursor>
#include <QProgressDialog>

CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow ),
    fSettings( new NLoadProdSync::CSettings )

{
    fImpl->setupUi( this );
    fRuntimeTimer.first = new QTimer( this );
    fRuntimeTimer.first->setSingleShot( false );
    fRuntimeTimer.first->setInterval( 500 );
    fRuntimeTimer.first->callOnTimeout( this, &CMainWindow::slotUpdateRuntimeLabel );

    fLocalUProdModel = new QFileSystemModel( this );
    fLocalUProdModel->setReadOnly( true );
    fLocalUProdModel->setResolveSymlinks( true );
    fLocalUProdModel->setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );

    connect( fImpl->openProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotOpenProjectFile );
    connect( fImpl->saveProjectFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSaveProjectFile );

    connect( fLocalUProdModel, &QFileSystemModel::directoryLoaded, this, &CMainWindow::slotPathLoaded );
    connect( fImpl->localUProdDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectLocalUProdDir );
    connect( fImpl->localProdDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectLocalProdDir );
    connect( fImpl->dataFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectDataFile );
    connect( fImpl->saveDataFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSaveDataFile );
    
    connect( fImpl->rsyncExecBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectRSyncExec );
    connect( fImpl->bashExecBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectBashExec );

    connect( fImpl->delItemBtn, &QToolButton::clicked, this, &CMainWindow::slotDelItem );
    connect( fImpl->addItemBtn, &QToolButton::clicked, this, &CMainWindow::slotAddItem );
    connect( fImpl->addAsExcludeBtn, &QToolButton::clicked, this, &CMainWindow::slotAddAsExclude );
    connect( fImpl->editItemBtn, &QToolButton::clicked, this, &CMainWindow::slotEditLocalProdDirItem );
    connect( fImpl->localProdDirTree, &QTreeWidget::itemDoubleClicked, this, &CMainWindow::slotEditLocalProdDirItem );

    connect( fImpl->runBtn, &QPushButton::clicked, this, &CMainWindow::slotRun );

    fImpl->uProdDirTree->setModel( this->fLocalUProdModel );
    fImpl->uProdDirTree->hideColumn( 1 );
    fImpl->uProdDirTree->hideColumn( 2 );

    fImpl->localProdDirTree->setHeaderLabels( QStringList() << "Directory" << "Exclude" << "Extra" );
    setRunning( false );
    QSettings settings;
    setProjects( settings.value( "RecentProjects" ).toStringList() );
    fImpl->projectFile->setFocus();

    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );

    popDisconnected( true );
}

CMainWindow::~CMainWindow()
{
    saveSettings();

    QSettings settings;
    settings.setValue( "RecentProjects", getProjects() );
}

void CMainWindow::saveSettings()
{
    slotSaveDataFile();

    fSettings->setRemoteUProdDir( fImpl->remoteUProdDir->text() );
    fSettings->setLocalUProdDir( fImpl->localUProdDir->text() );
    fSettings->setLocalProdDir( fImpl->localProdDir->text() );
    fSettings->setDataFile( fImpl->dataFile->text() );
    fSettings->setRsyncServer( fImpl->rsyncServer->text() );
    fSettings->setRsyncExec( fImpl->rsyncExec->text() );
    fSettings->setBashExec( fImpl->bashExec->text() );
    fSettings->setRsyncType( getDrivePrefix() );
    fSettings->setVerbose( fImpl->verbose->isChecked() );
    fSettings->setNoRun( fImpl->norun->isChecked() );
}

void CMainWindow::loadSettings()
{
    pushDisconnected();

    fImpl->remoteUProdDir->setText( fSettings->getRemoteUProdDir( "/u/prod" ) );
    fImpl->localUProdDir->setText( fSettings->getLocalUProdDir( "\\\\mgc\\home\\prod" ) );
    fImpl->localProdDir->setText( fSettings->getLocalProdDir( "C:\\localprod" ) );
    auto dataFile = qEnvironmentVariable( "P4_CLIENT_DIR" );
    bool isSet = fSettings->isDataFileSet();
    auto tmp = fSettings->fileName();
    if ( dataFile.isEmpty() && !fSettings->isDataFileSet() )
    {
        if ( QMessageBox::question( this, tr( "P4_CLIENT_DIR not set" ), tr( "Environmental variable P4_CLIENT_DIR is not set, would you like to select the directory?" ), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No ) == QMessageBox::StandardButton::Yes )
        {
            dataFile = QFileDialog::getExistingDirectory( this, tr( "Select P4_CLIENT_DIR directory" ) );
        }
    }
    if ( !dataFile.isEmpty() )
        dataFile += "/src/misc/WinLocalProdSyncData.xml";
    fImpl->dataFile->setText( fSettings->getDataFile( dataFile ) );
    fImpl->rsyncServer->setText( fSettings->getRsyncServer( "local-rsync-vm1.wv.mentorg.com" ) );
    fImpl->rsyncExec->setText( fSettings->getRsyncExec( "C:/msys64/usr/bin/rsync.exe" ) );
    fImpl->bashExec->setText( fSettings->getBashExec( "C:/msys64/usr/bin/bash.exe" ) );
    fImpl->verbose->setChecked( fSettings->getVerbose( true ) );
    fImpl->norun->setChecked( fSettings->getNoRun( false ) );
    auto drivePrefix = fSettings->getRsyncType( autoGetDrivePrefix() );
    if ( drivePrefix == NLoadProdSync::ERSyncType::eNative )
        fImpl->rsyncNative->setChecked( true );
    else if ( drivePrefix == NLoadProdSync::ERSyncType::eCygwin )
        fImpl->rsyncCygwin->setChecked( true );
    else if ( drivePrefix == NLoadProdSync::ERSyncType::eMSys64 )
        fImpl->rsyncMSys64->setChecked( true );

    popDisconnected();

    QTimer::singleShot( 0, this, &CMainWindow::setLocalUProdDir );
    QTimer::singleShot( 0, this, &CMainWindow::loadLocalProdDir );
}

void CMainWindow::setProjects( QStringList projects )
{
    for ( int ii = 0; ii < projects.count(); ++ii )
    {
        if ( projects[ ii ].isEmpty() || !QFileInfo( projects[ ii ] ).exists() )
        {
            projects.removeAt( ii );
            ii--;
        }
    }

    pushDisconnected();
    fImpl->projectFile->clear();
    fImpl->projectFile->addItems( QStringList() << QString() << projects );
    popDisconnected();
}

void CMainWindow::slotOpenProjectFile()
{
    auto currPath = fImpl->projectFile->currentText();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto projFile = QFileDialog::getOpenFileName( this, tr( "Select Project File to open" ), currPath, "Project Files *.ini;;All Files *.*" );
    setProjectFile( projFile, true );
}

void CMainWindow::slotSaveProjectFile()
{
    auto currPath = fImpl->projectFile->currentText();
    if ( currPath.isEmpty() )
        currPath = fSettings->fileName();
    if ( currPath.isEmpty() )
        currPath = QString();
    auto projFile = QFileDialog::getSaveFileName( this, tr( "Select Project File to save" ), currPath, "Project Files *.ini;;All Files *.*" );
    setProjectFile( projFile, false );
}

void CMainWindow::slotCurrentProjectChanged( const QString & projFile )
{
    setProjectFile( projFile, true );
}

void CMainWindow::setProjectFile( const QString & projFile, bool load )
{
    if ( projFile.isEmpty() )
        return;
    setCurrentProject( projFile );

    if ( load )
    {
        if ( !fSettings->loadSettings( projFile ) )
        {
            QMessageBox::critical( this, tr( "Project File not Opened" ), QString( "Error: '%1' is not a valid project file" ).arg( projFile ) );
            return;
        }
    }
    else
        fSettings->setFileName( projFile );
    setWindowTitle( tr( "Local Prod Sync - %1" ).arg( projFile ) );
    if ( load )
    {
        loadSettings();
        slotChanged();
    }
    else
    {
        saveSettings();
        fSettings->saveSettings();
    }
}

void CMainWindow::setCurrentProject( const QString & projFile )
{
    pushDisconnected();
    auto projects = getProjects();
    for ( int ii = 0; ii < projects.count(); ++ii )
    {
        if ( projects[ ii ] == projFile )
        {
            projects.removeAt( ii );
            ii--;
        }
    }
    projects.insert( 0, projFile );
    setProjects( projects );
    fImpl->projectFile->setCurrentText( projFile );
    popDisconnected();
}

QStringList CMainWindow::getProjects() const
{
    QStringList projects;
    for ( int ii = 1; ii < fImpl->projectFile->count(); ++ii )
        projects << fImpl->projectFile->itemText( ii );
    return projects;
}

NLoadProdSync::ERSyncType CMainWindow::getDrivePrefix() const
{
    if ( fImpl->rsyncNative->isChecked() )
        return NLoadProdSync::ERSyncType::eNative;
    else if ( fImpl->rsyncCygwin->isChecked() )
        return NLoadProdSync::ERSyncType::eCygwin;
    else if ( fImpl->rsyncMSys64->isChecked() )
        return NLoadProdSync::ERSyncType::eMSys64;
    return NLoadProdSync::ERSyncType::eNative;
}

NLoadProdSync::ERSyncType CMainWindow::autoGetDrivePrefix() const
{
    auto path = fImpl->rsyncExec->text();
    if ( path.contains( "cygwin", Qt::CaseInsensitive ) )
        return NLoadProdSync::ERSyncType::eCygwin;
    if ( path.contains( "msys64", Qt::CaseInsensitive ) )
        return NLoadProdSync::ERSyncType::eMSys64;

    return NLoadProdSync::ERSyncType::eNative;
}

void CMainWindow::slotSaveDataFile()
{
    if ( !isModified() )
        return;

    if ( !fDataFile )
        return;

    fDataFile->clear();
    for ( int ii = 0; ii < fImpl->localProdDirTree->topLevelItemCount(); ++ii )
    {
        auto item = fImpl->localProdDirTree->topLevelItem( ii );
        if ( !item )
            continue;

        auto dir = std::make_shared< NLoadProdSync::SDirectory >( item->text( 0 ) );
        for ( int jj = 0; jj < item->childCount(); ++jj )
        {
            auto child = item->child( jj );
            if ( !child )
                continue;

            if ( !child->text( 1 ).isEmpty() )
                dir->fExcludes << child->text( 1 );
            if ( !child->text( 2 ).isEmpty() )
                dir->fExtras << child->text( 2 );
        }
        fDataFile->addDir( dir );
    }

    if ( fDataFile->save( this ) )
    {
        pushDisconnected();
        fImpl->dataFile->setText( fDataFile->fileName() );
        setModified( false );
        popDisconnected();
    }
}

void CMainWindow::popDisconnected( bool force )
{
    if ( !force && fDisconnected )
        fDisconnected--;
    if ( force || fDisconnected == 0 )
    {
        connect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        connect( fImpl->localUProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->localProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->dataFile, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncServer, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->bashExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncNative, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncCygwin, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncMSys64, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        connect( fImpl->norun, &QCheckBox::clicked, this, &CMainWindow::slotChanged );
        connect( fImpl->uProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotUProdDirSelectionChanged );
        connect( fImpl->uProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->localProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotLocalProdDirSelectionChanged );
        connect( fImpl->localProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotChanged );
    }
}

void CMainWindow::pushDisconnected()
{
    if ( fDisconnected == 0 )
    {
        disconnect( fImpl->projectFile, &QComboBox::currentTextChanged, this, &CMainWindow::slotCurrentProjectChanged );
        disconnect( fImpl->localUProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->localProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->dataFile, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncServer, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->bashExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncNative, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncCygwin, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncMSys64, &QRadioButton::clicked, this, &CMainWindow::slotChanged );
        disconnect( fImpl->verbose, &QCheckBox::clicked, this, &CMainWindow::slotChanged );
        disconnect( fImpl->norun, &QCheckBox::clicked, this, &CMainWindow::slotChanged );
        disconnect( fImpl->uProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotUProdDirSelectionChanged );
        disconnect( fImpl->uProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->localProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotLocalProdDirSelectionChanged );
        disconnect( fImpl->localProdDirTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CMainWindow::slotChanged );
    }
    fDisconnected++;
}

void CMainWindow::appendToLog( const QString & txt )
{
    fImpl->log->appendPlainText( txt.trimmed() );
    fImpl->log->moveCursor( QTextCursor::End );
}

void CMainWindow::slotChanged()
{
    if ( localUProdDirChanged() )
        setLocalUProdDir();

    if ( localProdDirChanged() )
        loadLocalProdDir();

    bool aOK = !fUProdLoading;
    auto fi = QFileInfo( fImpl->localUProdDir->text() );
    aOK = aOK && fi.exists() && fi.isDir() && fi.isReadable();

    fi = QFileInfo( fImpl->dataFile->text() );
    aOK = aOK && fi.exists() && fi.isFile() && fi.isReadable();

    fi = QFileInfo( fImpl->rsyncServer->text() );
    aOK = aOK && !fImpl->rsyncServer->text().isEmpty();

    fi = QFileInfo( fImpl->rsyncExec->text() );
    aOK = aOK && fi.exists() && fi.isFile() && fi.isExecutable();

    if ( !fImpl->rsyncNative->isChecked() )
    {
        fi = QFileInfo( fImpl->bashExec->text() );
        aOK = aOK && fi.exists() && fi.isFile() && fi.isExecutable();
    }
    setRunning( false );
    fImpl->runBtn->setEnabled( aOK );
    
    fImpl->addAsExcludeBtn->setEnabled( !fImpl->uProdDirTree->selectionModel()->selection().isEmpty() && !fImpl->localProdDirTree->selectionModel()->selection().isEmpty() );
    fImpl->addItemBtn->setEnabled( !fImpl->uProdDirTree->selectionModel()->selection().isEmpty() );
    fImpl->delItemBtn->setEnabled( !fImpl->localProdDirTree->selectionModel()->selection().isEmpty() );
    fImpl->editItemBtn->setEnabled( !fImpl->localProdDirTree->selectionModel()->selection().isEmpty() );
}

QString getHierText( QModelIndex idx )
{
    if ( !idx.isValid() )
        return QString();
    auto currText = idx.data().toString();
    if ( currText.isEmpty() )
        return QString();
    auto parentText = getHierText( idx.parent() );
    if ( !parentText.isEmpty() )
        parentText += "/";
    parentText += currText;
    return parentText;
}


QString getHierText( QTreeWidgetItem * item )
{
    if ( !item )
        return QString();

    auto currText = item->text( 0 );
    auto parentText = getHierText( item->parent() );
    if ( !parentText.isEmpty() )
        parentText += "/";
    parentText += currText;
    return parentText;
}

QString CMainWindow::getSelectedLocalProdDirPath() const
{ 
    auto selected = fImpl->localProdDirTree->currentItem();
    if ( !selected )
        return QString();
    auto dir = selected->text( 0 );
    if ( dir.endsWith( "*" ) )
    {
        QFileInfo fi( dir );
        dir = fi.path();
    }
    return dir;
}

void CMainWindow::setModified( bool modified )
{
    setWindowTitle( tr( "Local Prod Sync Tool%1" ).arg( modified ? "*" : "" ) );
    fModified = modified;
}

bool CMainWindow::isModified() const
{
    if ( fModified )
        return true;
    return ( QFileInfo( fImpl->dataFile->text() ).suffix() == "json" );
}

QString CMainWindow::getSelectedUProdDirPath() const
{
    auto selected = fImpl->uProdDirTree->selectionModel()->selectedIndexes();
    if ( selected.isEmpty() )
        return QString();

    auto idx = selected.front();
    auto dir = getHierText( idx );
    dir = dir.replace( "\\", "/" );
    dir = dir.replace( "//", "" );

    auto localUProdDir = fImpl->localUProdDir->text();
    localUProdDir = localUProdDir.replace( "\\", "/" );
    localUProdDir = localUProdDir.replace( "//", "" );
    if ( dir.startsWith( localUProdDir ) )
        dir = dir.mid( localUProdDir.length() + 1 );

    return dir;
}

QTreeWidgetItem * CMainWindow::getTopItem( QTreeWidgetItem * item ) const
{
    if ( item )
    {
        while ( item->parent() )
        {
            item = item->parent();
        }
    }
    return item;
}

QList< QTreeWidgetItem * > CMainWindow::getItems( int columnNum, const QStringList & items )
{
    if ( items.isEmpty() )
        return {};

    auto prefix = QStringList();
    for ( int ii = 0; ii < columnNum; ++ii )
        prefix << QString();

    QList< QTreeWidgetItem * > retVal;
    for ( auto && ii : items )
    {
        auto curr = new QTreeWidgetItem( QStringList() << prefix << ii );
        retVal << curr;
    }
    return retVal;
}

QTreeWidgetItem * CMainWindow::getCurrLocalItem( bool andSelect ) const
{
    auto currItem = fImpl->localProdDirTree->currentItem();
    auto retVal = getTopItem( currItem );
    if ( retVal && andSelect )
    {
        selectLocalItem( retVal );
    }
    return retVal;
}

void CMainWindow::selectLocalItem( QTreeWidgetItem * retVal ) const
{
    fImpl->localProdDirTree->setCurrentItem( retVal, QItemSelectionModel::ClearAndSelect );
    fImpl->localProdDirTree->scrollToItem( retVal );
}

void CMainWindow::slotLocalProdDirSelectionChanged()
{
    auto dir = getSelectedLocalProdDirPath();
    if ( dir.isEmpty() )
        return;

    dir = QDir( fImpl->localUProdDir->text() ).absoluteFilePath( dir );
    auto idx = fLocalUProdModel->index( dir );
    pushDisconnected();
    fImpl->uProdDirTree->scrollTo( idx );
    fImpl->uProdDirTree->setCurrentIndex( idx );
    fImpl->uProdDirTree->selectionModel()->select( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    popDisconnected();
}

void CMainWindow::slotUProdDirSelectionChanged()
{
    auto dir = getSelectedUProdDirPath();
    if ( dir.isEmpty() )
        return;

    std::pair< QTreeWidgetItem *, int > firstBeginsWith = { nullptr, 0 };
    for ( int ii = 0; ii < fImpl->localProdDirTree->topLevelItemCount(); ++ii )
    {
        auto currItem = fImpl->localProdDirTree->topLevelItem( ii );
        if ( !currItem )
            continue;

        auto currText = getHierText( currItem );
        if ( ( currText == dir ) || ( dir + "/*" == currText ) )
        {
            currItem->setSelected( true );
            selectLocalItem( currItem );
            return;
        }
        if ( currText.startsWith( dir ) )
        {
            if ( !firstBeginsWith.first || ( currText.length() > firstBeginsWith.second ) )
            {
                firstBeginsWith = { currItem, currText.length() };
            }
        }
    }

    if ( firstBeginsWith.first )
        selectLocalItem( firstBeginsWith.first );
}

void CMainWindow::slotAddAsExclude()
{
    auto leafName = QFileInfo( getSelectedUProdDirPath() ).fileName();
    if ( leafName.isEmpty() )
        return;

    auto currItem = getCurrLocalItem( true );
    auto items = getItems( 1, { leafName } );
    currItem->addChildren( items );
    currItem->setExpanded( true );
    auto newChild = items.isEmpty() ? items.front() : nullptr;
    if ( newChild )
    {
        newChild->setSelected( true );
        fImpl->localProdDirTree->scrollToItem( newChild );
    }

    setModified( true );
}

void CMainWindow::slotAddItem()
{
    auto dir = getSelectedUProdDirPath();
    if ( dir.isEmpty() )
        return;

    auto currItem = getCurrLocalItem( true );

    auto newItem = new QTreeWidgetItem( QStringList() << dir );
    fImpl->localProdDirTree->insertTopLevelItem( fImpl->localProdDirTree->indexOfTopLevelItem( currItem )+1, newItem );
    setModified( true );
}

void CMainWindow::slotDelItem()
{
    auto currItem = getCurrLocalItem( true );
    if ( !currItem )
        return;

    delete currItem;
    setModified( true );
}

void CMainWindow::slotEditLocalProdDirItem()
{
    auto currItem = getCurrLocalItem( false );
    if ( !currItem )
        return;

    CAddAttribute dlg( currItem, this );
    if ( dlg.exec() == QDialog::Accepted )
    {
        auto idx = fImpl->localProdDirTree->indexOfTopLevelItem( currItem );
        loadItem( idx, dlg.dir(), dlg.excludes(), dlg.extras() );
        delete currItem;
        setModified( true );
    }
}

bool CMainWindow::localUProdDirChanged() const
{
    if ( !fCurrLocalUProdDir.has_value() )
        return true;
    return fCurrLocalUProdDir.value() != QDir( fImpl->localUProdDir->text() );
}

bool CMainWindow::localProdDirChanged() const
{
    if ( !fCurrLocalProdDir.has_value() )
        return true;
    if ( fCurrLocalProdDir.value() != QDir( fImpl->localProdDir->text() ) )
         return true;
    if ( !fCurrDataFile.has_value() )
        return true;
    if ( fCurrDataFile.value() != QFileInfo( fImpl->dataFile->text() ) )
        return true;
    return false;
}

void CMainWindow::slotSelectLocalUProdDir()
{
    auto currPath = fImpl->localUProdDir->text();
    auto path = QFileDialog::getExistingDirectory( this, tr( "Select U Prod Directory" ), currPath );
    if ( path.isEmpty() )
        return;
    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::warning( this, "Invalid U Prod directory", QString( "'%1' is not a directory that is readable" ).arg( path ) );
        return;
    }

    fImpl->localUProdDir->setText( path );
}

void CMainWindow::slotSelectLocalProdDir()
{
    auto currPath = fImpl->localProdDir->text();
    auto path = QFileDialog::getExistingDirectory( this, tr( "Select Local Prod Directory" ), currPath );
    if ( path.isEmpty() )
        return;

    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isDir() || !fi.isWritable() )
    {
        QMessageBox::warning( this, "Invalid Local Prod directory", QString( "'%1' is not a directory that is writable" ).arg( path ) );
        return;
    }

    fImpl->localProdDir->setText( path );
}

void CMainWindow::slotSelectDataFile()
{
    auto currPath = fImpl->dataFile->text();
    auto path = QFileDialog::getOpenFileName( this, tr( "Select Data File" ), currPath, "XML Data Files *.xml;;JSON Data Files *.json;;All Files *.*" );
    if ( path.isEmpty() )
        return;

    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isFile() || !fi.isReadable() )
    {
        QMessageBox::warning( this, "Invalid Data File", QString( "'%1' is not a file that is readable" ).arg( path ) );
        return;
    }

    fImpl->dataFile->setText( path );
}

void CMainWindow::slotSelectRSyncExec()
{
    auto currPath = fImpl->dataFile->text();
    auto path = QFileDialog::getOpenFileName( this, tr( "Select RSync Executable" ), currPath, "Executable Files *.exe" );
    if ( path.isEmpty() )
        return;

    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isFile() || !fi.isExecutable() )
    {
        QMessageBox::warning( this, "Invalid RSync Executable File", QString( "'%1' is not a file that is executable" ).arg( path ) );
        return;
    }

    fImpl->rsyncExec->setText( path );
}

void CMainWindow::slotSelectBashExec()
{
    auto currPath = fImpl->dataFile->text();
    auto path = QFileDialog::getOpenFileName( this, tr( "Select Bash Executable" ), currPath, "Executable Files *.exe" );
    if ( path.isEmpty() )
        return;

    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isFile() || !fi.isExecutable() )
    {
        QMessageBox::warning( this, "Invalid Bash Executable File", QString( "'%1' is not a file that is executable" ).arg( path ) );
        return;
    }

    fImpl->bashExec->setText( path );
}

void CMainWindow::setLocalUProdDir()
{
    if ( !localUProdDirChanged() )
        return;

    pushDisconnected();
    QApplication::setOverrideCursor( Qt::WaitCursor );
    fCurrLocalUProdDir = QDir( fImpl->localUProdDir->text() );
    qApp->processEvents();
    fUProdLoading = true;
    setEnabled( false );
    qApp->processEvents();
    auto path = fCurrLocalUProdDir.value().absolutePath();
    fImpl->uProdDirTree->setRootIndex( fLocalUProdModel->index( path ) );
    fLocalUProdModel->setRootPath( path );
    popDisconnected();
}

void CMainWindow::slotPathLoaded( const QString & path )
{
    qDebug() << "Path Loaded: " << path;
    if ( QDir( path ) == QDir( fImpl->localUProdDir->text() ) )
    {
        QApplication::restoreOverrideCursor();
        fUProdLoading = false;
        setEnabled( true );
        slotChanged();
    }
    else
    {
        auto idx = fLocalUProdModel->index( path );
        auto selected = fImpl->uProdDirTree->currentIndex();
        if ( selected.parent() == idx )
        {
            fImpl->uProdDirTree->scrollTo( idx );
        }
    }
}

void CMainWindow::loadLocalProdDir()
{
    if ( !localProdDirChanged() )
        return;

    fImpl->localProdDirTree->clear();
    fImpl->localProdDirTree->setHeaderLabels( QStringList() << "Directory" << "Exclude" << "Extra" );

    fDataFile = std::make_shared< NLoadProdSync::CDataFile >();
    if ( !fDataFile->load( fImpl->dataFile->text(), this ) )
    {
        fDataFile->clear();
        fCurrDataFile.reset();
        fCurrLocalProdDir.reset();
        return;
    }
    for ( auto && ii : fDataFile->dirs() )
    {
        loadItem( -1, ii->fPath, ii->fExcludes, ii->fExtras );
    }

    fCurrDataFile = QFileInfo( fImpl->dataFile->text() );
    fCurrLocalProdDir = QDir( fImpl->localProdDir->text() );
    fImpl->localProdDirTree->sortItems( 0, Qt::SortOrder::AscendingOrder );
}

void CMainWindow::loadItem( int pos, const QString & src, const QStringList & excludes, const QStringList & extras )
{
    auto currItem = new QTreeWidgetItem( QStringList() << src );

    currItem->addChildren( getItems( 1, excludes ) );
    currItem->addChildren( getItems( 2, extras ) );

    if ( pos == -1 )
        fImpl->localProdDirTree->addTopLevelItem( currItem );
    else
        fImpl->localProdDirTree->insertTopLevelItem( pos, currItem );
}

void CMainWindow::setRunning( bool running )
{
    fImpl->runBtn->setEnabled( !running );
    fImpl->runtimeLabel->setVisible( running );
}

void CMainWindow::slotUpdateRuntimeLabel()
{
    auto now = std::chrono::system_clock::now();
    auto elapsedTime = now - fRuntimeTimer.second;

    double secs = 1.0 * std::chrono::duration_cast<std::chrono::seconds>( elapsedTime ).count();
    auto hours = std::chrono::duration_cast<std::chrono::hours>( elapsedTime ).count();
    auto mins = std::chrono::duration_cast<std::chrono::minutes>( elapsedTime ).count();

    secs -= ( ( mins * 60 ) + ( hours * 3600 ) );

    QString text = QString( "Runtime %1:%2:%3" ).arg( hours, 2, 10, QChar( '0' ) ).arg( mins, 2, 10, QChar( '0' ) ).arg( static_cast<int>( secs ), 2, 10, QChar( '0' ) );
    fImpl->runtimeLabel->setText( text );
}

void CMainWindow::slotRun()
{
    saveSettings();
    if ( !fDataFile )
        return;

    auto progress = new QProgressDialog( tr( "Synchronizing the prod Directory" ), tr( "Cancel" ), 0, 0, this );
    auto pb = new QPushButton( tr( "Cancel" ) );
    progress->setCancelButton( pb );
    connect( pb, &QPushButton::clicked, this, &CMainWindow::slotStop );
    progress->setWindowFlags( progress->windowFlags() & ~Qt::WindowCloseButtonHint );
    progress->setAutoReset( false );
    progress->setAutoClose( true );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );
    progress->setRange( 0, fDataFile->numDirs() );
    progress->setValue( 0 );
    qApp->processEvents();

    setRunning( true );
    fImpl->runtimeLabel->setText( "Runtime 00:00:00" );
    fRuntimeTimer.second = std::chrono::system_clock::now();
    fRuntimeTimer.first->start();

    if ( fDataFile )
    {
        fImpl->log->clear();
        auto retVal = fDataFile->run( fImpl->remoteUProdDir->text(), fImpl->rsyncServer->text(), fImpl->localProdDir->text(), fImpl->rsyncExec->text(), fImpl->bashExec->text(), fImpl->verbose->isChecked(), fImpl->norun->isChecked(), getDrivePrefix(), progress,
                        [this]( const QString & msg )
                        {
                            appendToLog( msg );
                        } );
        if ( !retVal.first )
            appendToLog( retVal.second );
    }
    setRunning( false );
}

void CMainWindow::slotStop()
{
    if ( fDataFile )
        fDataFile->stop();
}