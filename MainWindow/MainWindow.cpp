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

CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow )
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
    connect( fImpl->stopBtn, &QPushButton::clicked, this, &CMainWindow::slotStop );

    fImpl->uProdDirTree->setModel( this->fLocalUProdModel );
    fImpl->uProdDirTree->hideColumn( 1 );
    fImpl->uProdDirTree->hideColumn( 2 );

    fImpl->localProdDirTree->setHeaderLabels( QStringList() << "Directory" << "Exclude" << "Extra" );
    setRunning( false );
    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );
    popDisconnected( true );
}

CMainWindow::~CMainWindow()
{
    saveSettings();
}

NLoadProdSync::EDrivePrefix CMainWindow::getDrivePrefix() const
{
    if ( fImpl->rsyncNative->isChecked() )
        return NLoadProdSync::EDrivePrefix::eNative;
    else if ( fImpl->rsyncCygwin->isChecked() )
        return NLoadProdSync::EDrivePrefix::eCygwin;
    else if ( fImpl->rsyncMSys64->isChecked() )
        return NLoadProdSync::EDrivePrefix::eMSys64;
    return NLoadProdSync::EDrivePrefix::eNative;
}

NLoadProdSync::EDrivePrefix CMainWindow::autoGetDrivePrefix() const
{
    auto path = fImpl->rsyncExec->text();
    if ( path.contains( "cygwin", Qt::CaseInsensitive ) )
        return NLoadProdSync::EDrivePrefix::eCygwin;
    if ( path.contains( "msys64", Qt::CaseInsensitive ) )
        return NLoadProdSync::EDrivePrefix::eMSys64;

    return NLoadProdSync::EDrivePrefix::eNative;
}

void CMainWindow::saveSettings()
{
    slotSaveDataFile();

    QSettings settings;

    settings.setValue( "remoteUProdDir", fImpl->remoteUProdDir->text() );
    settings.setValue( "localUProdDir", fImpl->localUProdDir->text() );
    settings.setValue( "localProdDir", fImpl->localProdDir->text() );
    settings.setValue( "dataFile", fImpl->dataFile->text() );
    settings.setValue( "rsyncServer", fImpl->rsyncServer->text() );
    settings.setValue( "rsyncExec", fImpl->rsyncExec->text() );
    settings.setValue( "bashExec", fImpl->bashExec->text() );
    settings.setValue( "rsyncDrivePrefix", (int)getDrivePrefix() );
    settings.setValue( "verbose", fImpl->verbose->isChecked() );
    settings.setValue( "norun", fImpl->norun->isChecked() );
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

void CMainWindow::loadSettings()
{
    pushDisconnected();

    QSettings settings;
    fImpl->remoteUProdDir->setText( settings.value( "remoteUProdDir", "/u/prod" ).toString() );
    fImpl->localUProdDir->setText( settings.value( "localUProdDir", "\\\\mgc\\home\\prod" ).toString() );
    fImpl->localProdDir->setText( settings.value( "localProdDir", "C:\\localprod" ).toString() );
    auto dataFile = qEnvironmentVariable( "P4_CLIENT_DIR" );
    if ( dataFile.isEmpty() && !settings.contains( "dataFile" ) )
    {
        if ( QMessageBox::question( this, tr( "P4_CLIENT_DIR not set" ), tr( "Environmental variable P4_CLIENT_DIR is not set, would you like to select the directory?" ), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No ) == QMessageBox::StandardButton::Yes )
        {
            dataFile = QFileDialog::getExistingDirectory( this, tr( "Select P4_CLIENT_DIR directory" ) );
        }
    }
    if ( !dataFile.isEmpty() )
        dataFile += "/src/misc/WinLocalProdSyncData.xml";
    fImpl->dataFile->setText( settings.value( "dataFile", dataFile ).toString() );
    fImpl->rsyncServer->setText( settings.value( "rsyncServer", "local-rsync-vm1.wv.mentorg.com" ).toString() );
    fImpl->rsyncExec->setText( settings.value( "rsyncExec", "C:/msys64/usr/bin/rsync.exe" ).toString() );
    fImpl->bashExec->setText( settings.value( "bashExec", "C:/msys64/usr/bin/bash.exe" ).toString() );
    fImpl->verbose->setChecked( settings.value( "verbose", true ).toBool() );
    fImpl->norun->setChecked( settings.value( "norun", false ).toBool() );
    auto drivePrefix = static_cast<NLoadProdSync::EDrivePrefix>( settings.value( "rsyncDrivePrefix", (int)autoGetDrivePrefix() ).toInt() );
    if ( drivePrefix == NLoadProdSync::EDrivePrefix::eNative )
        fImpl->rsyncNative->setChecked( true );
    else if ( drivePrefix == NLoadProdSync::EDrivePrefix::eCygwin )
        fImpl->rsyncCygwin->setChecked( true );
    else if ( drivePrefix == NLoadProdSync::EDrivePrefix::eMSys64 )
        fImpl->rsyncMSys64->setChecked( true );

    popDisconnected();

    QTimer::singleShot( 0, this, &CMainWindow::setLocalUProdDir );
    QTimer::singleShot( 0, this, &CMainWindow::loadLocalProdDir );
}

void CMainWindow::popDisconnected( bool force )
{
    if ( !force && fDisconnected )
        fDisconnected--;
    if ( force || fDisconnected == 0 )
    {
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
    fImpl->stopBtn->setEnabled( running );
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
    setRunning( true );
    fImpl->runtimeLabel->setText( "Runtime 00:00:00" );
    fRuntimeTimer.second = std::chrono::system_clock::now();
    fRuntimeTimer.first->start();

    if ( fDataFile )
    {
        fImpl->log->clear();
        auto retVal = fDataFile->run( fImpl->remoteUProdDir->text(), fImpl->rsyncServer->text(), fImpl->localProdDir->text(), fImpl->rsyncExec->text(), fImpl->bashExec->text(), fImpl->verbose->isChecked(), fImpl->norun->isChecked(), getDrivePrefix(),
                        [this]( const QString & msg )
                        {
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