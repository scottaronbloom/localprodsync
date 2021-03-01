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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <optional>
#include <chrono>
#include <QPersistentModelIndex>

class QFileSystemModel;
class QTreeWidgetItem;
class QItemSelection;
namespace Ui {class CMainWindow;};
namespace NLoadProdSync
{
    class CSettings;
    class CDataFile;
    enum class ERSyncType;
}

class CMainWindow : public QDialog
{
    Q_OBJECT
public:
    CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

    NLoadProdSync::ERSyncType getDrivePrefix() const; // based on radio button settings defaults to native
    NLoadProdSync::ERSyncType autoGetDrivePrefix() const; // based on the rsync executable path default to native

public Q_SLOTS:
    void slotChanged();
    void slotUProdDirSelectionChanged();
    void slotLocalProdDirSelectionChanged();
    void slotPathLoaded( const QString & path );
    void slotSelectLocalUProdDir();
    void slotSelectLocalProdDir();
    void slotSelectDataFile();
    void slotSelectRSyncExec();
    void slotSelectBashExec();

    void slotDelItem();
    void slotAddItem();
    void slotAddAsExclude();
    void slotEditLocalProdDirItem();
    void slotSaveDataFile();
    void slotRun();
    void slotStop();
    void slotUpdateRuntimeLabel();
    void slotOpenProjectFile();
    void slotSaveProjectFile();
    void setProjectFile( const QString & projFile, bool load );
    void slotCurrentProjectChanged( const QString & projFile );
private:
    QStringList getProjects() const;
    void setCurrentProject( const QString & projFile );
    bool localUProdDirChanged() const;
    bool localProdDirChanged() const;
    void setModified( bool modified );
    bool isModified() const;
    QString getSelectedUProdDirPath() const;
    QString getSelectedLocalProdDirPath() const;
    QTreeWidgetItem * getCurrLocalItem( bool andSelect ) const;
    void selectLocalItem( QTreeWidgetItem * retVal ) const;
    QTreeWidgetItem * getTopItem( QTreeWidgetItem * item ) const;
    void setProjects( QStringList projects );

    QList< QTreeWidgetItem * > getItems( int columnNum, const QStringList & items );

    void loadSettings();
    void saveSettings();
    void pushDisconnected();
    void popDisconnected( bool force = false );
    void appendToLog( const QString & txt );

    void setLocalUProdDir();
    void loadLocalProdDir();

    void loadItem( int pos, const QString & src, const QStringList & excludes, const QStringList & extras );
    void setRunning( bool running );

    std::optional< QDir > fCurrLocalUProdDir;
    std::optional< QFileInfo > fCurrDataFile;
    std::optional< QDir > fCurrLocalProdDir;

    QFileSystemModel * fLocalUProdModel{ nullptr };

    int fDisconnected{ 0 };
    bool fUProdLoading{ false };
    bool fModified{ false };
    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::unique_ptr< NLoadProdSync::CSettings > fSettings;
    std::shared_ptr< NLoadProdSync::CDataFile > fDataFile;
    std::pair< QTimer *, std::chrono::system_clock::time_point > fRuntimeTimer;
};

#endif 
