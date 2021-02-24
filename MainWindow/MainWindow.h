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

class QFileSystemModel;
class QTreeWidgetItem;
class QItemSelection;
namespace Ui {class CMainWindow;};

class CMainWindow : public QDialog
{
    Q_OBJECT
public:
    CMainWindow(QWidget *parent = 0);
    ~CMainWindow();

public Q_SLOTS:
    void slotChanged();
    void slotUProdDirSelectionChanged();
    void slotLocalProdDirSelectionChanged();
    bool uProdDirChanged() const;
    bool localProdDirChanged() const;
    void slotPathLoaded();
    void slotSelectUProdDir();
    void slotSelectLocalProdDir();
    void slotSelectDataFile();
    void slotSelectRSyncExec();

    void slotDelItem();
    void slotAddItem();
    void slotEditLocalProdDirItem();
    void slotSaveDataFile();
private:
    QString getSelectedUProdDirPath() const;
    QString getSelectedLocalProdDirPath() const;
    QTreeWidgetItem * getCurrLocalItem( bool andSelect ) const;

    void selectLocalItem( QTreeWidgetItem * retVal ) const;

    QTreeWidgetItem * getTopItem( QTreeWidgetItem * item ) const;

    void loadSettings();
    void saveSettings();
    void pushDisconnected();
    void popDisconnected( bool force = false );
    void appendToLog( const QString & txt );

    void loadUProdDir();
    void loadLocalProdDir();

    void loadItem( int pos, const QString & src, const QStringList & excludes, const QStringList & extras );

    std::optional< QDir > fCurrUProdDir;
    std::optional< QFileInfo > fCurrDataFile;
    std::optional< QDir > fCurrLocalProdDir;

    QFileSystemModel * fUProdModel{ nullptr };

    int fDisconnected{ 0 };
    bool fUProdLoading{ false };
    std::unique_ptr< Ui::CMainWindow > fImpl;
};

#endif // _ALCULATOR_H
