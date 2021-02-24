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

#include "ui_MainWindow.h"

#include <QSettings>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QItemSelection>

CMainWindow::CMainWindow( QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fUProdModel = new QFileSystemModel( this );
    fUProdModel->setReadOnly( true );
    fUProdModel->setResolveSymlinks( true );
    fUProdModel->setFilter( QDir::AllDirs | QDir::NoDotAndDotDot );

    connect( fUProdModel, &QFileSystemModel::directoryLoaded, this, &CMainWindow::slotPathLoaded );
    connect( fImpl->uProdDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectUProdDir );
    connect( fImpl->localProdDirBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectLocalProdDir );
    connect( fImpl->dataFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectDataFile );
    connect( fImpl->saveDataFileBtn, &QToolButton::clicked, this, &CMainWindow::slotSaveDataFile );
    
    connect( fImpl->rsyncExecBtn, &QToolButton::clicked, this, &CMainWindow::slotSelectRSyncExec );
    
    connect( fImpl->delItemBtn, &QPushButton::clicked, this, &CMainWindow::slotDelItem );
    connect( fImpl->addItemBtn, &QPushButton::clicked, this, &CMainWindow::slotAddItem );
    connect( fImpl->editItemBtn, &QPushButton::clicked, this, &CMainWindow::slotEditLocalProdDirItem );
    connect( fImpl->localProdDirTree, &QTreeWidget::itemDoubleClicked, this, &CMainWindow::slotEditLocalProdDirItem );

    fImpl->uProdDirTree->setModel( this->fUProdModel );
    fImpl->localProdDirTree->setHeaderLabels( QStringList() << "Directory" << "Exclude" << "Extra" );
    QTimer::singleShot( 0, this, &CMainWindow::loadSettings );
    popDisconnected( true );
}

CMainWindow::~CMainWindow()
{
    saveSettings();
}

void CMainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue( "uProdDir", fImpl->uProdDir->text() );
    settings.setValue( "localProdDir", fImpl->localProdDir->text() );
    settings.setValue( "dataFile", fImpl->dataFile->text() );
    settings.setValue( "rsyncServer", fImpl->rsyncServer->text() );
    settings.setValue( "rsyncExec", fImpl->rsyncExec->text() );
    settings.setValue( "verbose", fImpl->verbose->isChecked() );
    settings.setValue( "norun", fImpl->norun->text() );
}

void CMainWindow::slotSaveDataFile()
{

}

void CMainWindow::loadSettings()
{
    pushDisconnected();

    QSettings settings;
    fImpl->uProdDir->setText( settings.value( "uProdDir", "\\\\mgc\\home\\prod" ).toString() );
    fImpl->localProdDir->setText( settings.value( "localProdDir", "C:\\localprod" ).toString() );
    auto dataFile = qEnvironmentVariable( "P4_CLIENT_DIR" );
    if ( !dataFile.isEmpty() )
        dataFile += "/src/misc/WinLocalProdSyncData.json";
    fImpl->dataFile->setText( settings.value( "dataFile", dataFile ).toString() );
    fImpl->rsyncServer->setText( settings.value( "rsyncServer", "local-rsync-vm1.wv.mentorg.com" ).toString() );
    fImpl->rsyncExec->setText( settings.value( "rsyncExec" ).toString() );
    fImpl->verbose->setChecked( settings.value( "verbose", true ).toBool() );
    fImpl->norun->setChecked( settings.value( "norun", false ).toBool() );


    popDisconnected();
    slotChanged();
}

void CMainWindow::popDisconnected( bool force )
{
    if ( !force && fDisconnected )
        fDisconnected--;
    if ( force || fDisconnected == 0 )
    {
        connect( fImpl->uProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->localProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->dataFile, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncServer, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->rsyncExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        connect( fImpl->verbose, &QCheckBox::clicked, this, &CMainWindow::slotChanged );
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
        disconnect( fImpl->uProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->localProdDir, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->dataFile, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncServer, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
        disconnect( fImpl->rsyncExec, &QLineEdit::textChanged, this, &CMainWindow::slotChanged );
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
}

void CMainWindow::slotChanged()
{
    if ( uProdDirChanged() )
        loadUProdDir();

    if ( localProdDirChanged() )
        loadLocalProdDir();

    bool aOK = !fUProdLoading;
    auto fi = QFileInfo( fImpl->uProdDir->text() );
    aOK = aOK && fi.exists() && fi.isDir() && fi.isReadable();

    fi = QFileInfo( fImpl->localProdDir->text() );
    aOK = aOK && fi.exists() && fi.isDir() && fi.isWritable();

    fi = QFileInfo( fImpl->dataFile->text() );
    aOK = aOK && fi.exists() && fi.isFile() && fi.isReadable();

    fi = QFileInfo( fImpl->rsyncServer->text() );
    aOK = aOK && !fImpl->rsyncServer->text().isEmpty();

    fi = QFileInfo( fImpl->rsyncExec->text() );
    aOK = aOK && fi.exists() && fi.isFile() && fi.isExecutable();

    fImpl->runBtn->setEnabled( aOK );

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

QString CMainWindow::getSelectedUProdDirPath() const
{
    auto selected = fImpl->uProdDirTree->selectionModel()->selectedIndexes();
    if ( selected.isEmpty() )
        return QString();

    auto idx = selected.front();
    auto dir = getHierText( idx );
    dir = dir.replace( "\\", "/" );
    dir = dir.replace( "//", "" );

    auto uProdDir = fImpl->uProdDir->text();
    uProdDir = uProdDir.replace( "\\", "/" );
    uProdDir = uProdDir.replace( "//", "" );
    if ( dir.startsWith( uProdDir ) )
        dir = dir.mid( uProdDir.length() + 1 );

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
    //auto dir = getSelectedLocalProdDirPath();
    //if ( dir.isEmpty() )
    //    return;

    //std::pair< QTreeWidgetItem *, int > firstBeginsWith = { nullptr, 0 };
    //QPersistentModelIndex  idx = fUProdModel->index( QDir( fImpl->localProdDir->text() ).absoluteFilePath( dir ) );
    //while( !idx.isValid() )
    //{
    //    auto pos = dir.lastIndexOf( "/" );
    //    if ( pos == -1 )
    //        break;
    //    dir = dir.left( pos );
    //    idx = fUProdModel->index( QDir( fImpl->localProdDir->text() ).absoluteFilePath( dir ) );
    //}

    //pushDisconnected();
    //fImpl->uProdDirTree->scrollTo( idx );
    //fImpl->uProdDirTree->selectionModel()->select( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    //popDisconnected();
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

    selectLocalItem( firstBeginsWith.first );
}

void CMainWindow::slotAddItem()
{
    auto dir = getSelectedUProdDirPath();
    if ( dir.isEmpty() )
        return;

    auto currItem = getCurrLocalItem( true );
    if ( !currItem )
        return;

    auto newItem = new QTreeWidgetItem( QStringList() << dir );
    fImpl->localProdDirTree->insertTopLevelItem( fImpl->localProdDirTree->indexOfTopLevelItem( currItem )+1, newItem );
}

void CMainWindow::slotDelItem()
{
    auto currItem = getCurrLocalItem( true );
    if ( !currItem )
        return;

    delete currItem;
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
    }
}

bool CMainWindow::uProdDirChanged() const
{
    if ( !fCurrUProdDir.has_value() )
        return true;
    return fCurrUProdDir.value() != QDir( fImpl->uProdDir->text() );
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

void CMainWindow::slotSelectUProdDir()
{
    auto currPath = fImpl->uProdDir->text();
    auto path = QFileDialog::getExistingDirectory( this, tr( "Select U Prod Directory" ), currPath );
    if ( path.isEmpty() )
        return;
    auto fi = QFileInfo( path );
    if ( !fi.exists() || !fi.isDir() || !fi.isReadable() )
    {
        QMessageBox::warning( this, "Invalid U Prod directory", QString( "'%1' is not a directory that is readable" ).arg( path ) );
        return;
    }

    fImpl->uProdDir->setText( path );
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
    auto path = QFileDialog::getOpenFileName( this, tr( "Select Data File" ), currPath, "JSON Data Files *.json;;All Files *.*" );
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


void CMainWindow::loadUProdDir()
{
    if ( !uProdDirChanged() )
        return;

    QApplication::setOverrideCursor( Qt::WaitCursor );
    fCurrUProdDir = QDir( fImpl->uProdDir->text() );
    qApp->processEvents();
    fUProdLoading = true;
    setEnabled( false );
    slotChanged();
    auto path = fCurrUProdDir.value().absolutePath();
    fUProdModel->setRootPath( path );
    fImpl->uProdDirTree->setRootIndex( fUProdModel->index( path ) );
}

void CMainWindow::slotPathLoaded()
{
    QApplication::restoreOverrideCursor();
    fUProdLoading = false;
    setEnabled( true );
    slotChanged();
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

QList< QTreeWidgetItem * > getItems( int columnNum, const QStringList & items )
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

void CMainWindow::loadLocalProdDir()
{
    if ( !localProdDirChanged() )
        return;

    fImpl->localProdDirTree->clear();
    fImpl->localProdDirTree->setHeaderLabels( QStringList() << "Directory" << "Exclude" << "Extra" );

    auto file = QFile( fImpl->dataFile->text() );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        return;
    auto jsonText = file.readAll();
    file.close();
    auto jsonData = QJsonDocument::fromJson( jsonText );
    auto dirObj = jsonData[ "directories" ];
    if ( !dirObj.isArray() )
    {
        QMessageBox::warning( this, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fImpl->dataFile->text() ) );
        return;
    }

    auto dirs = dirObj.toArray();
    for ( int ii = 0; ii < dirs.count(); ++ii )
    {
        const auto && currDir = dirs.at( ii );
        if ( currDir[ "src" ].isNull() )
        {
            QMessageBox::warning( this, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fImpl->dataFile->text() ) );
            return;
        }
        auto src = currDir[ "src" ].toString();

        auto excludes = getStringList( "exclude", currDir );
        auto extras = getStringList( "extra", currDir );
        if ( !excludes.has_value() || !extras.has_value() )
        {
            QMessageBox::warning( this, "Invalid JSON file", QString( "'%1' is not a valid JSON file" ).arg( fImpl->dataFile->text() ) );
            return;
        }
        loadItem( -1, src, excludes.value(), extras.value() );
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
