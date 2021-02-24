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

#include "AddAttribute.h"
#include "ui_AddAttribute.h"

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>

CAddAttribute::CAddAttribute( QTreeWidgetItem * item, QWidget * parent )
    : QDialog( parent ),
    fImpl( new Ui::CAddAttribute )
{
    fImpl->setupUi( this );
    loadItem( item );

    connect( fImpl->excludes, &QListWidget::itemSelectionChanged, this, &CAddAttribute::slotChanged );
    connect( fImpl->extras, &QListWidget::itemSelectionChanged, this, &CAddAttribute::slotChanged );
    connect( fImpl->dir, &QLineEdit::textChanged, this, &CAddAttribute::slotChanged );

    connect( fImpl->addExtraBtn, &QToolButton::clicked, this, &CAddAttribute::slotAddExtra );
    connect( fImpl->delExtraBtn, &QToolButton::clicked, this, &CAddAttribute::slotDelExtra );
    connect( fImpl->addExcludeBtn, &QToolButton::clicked, this, &CAddAttribute::slotAddExclude );
    connect( fImpl->delExcludeBtn, &QToolButton::clicked, this, &CAddAttribute::slotDelExclude );

    slotChanged();
}

void CAddAttribute::slotChanged()
{
    fImpl->delExcludeBtn->setEnabled( !fImpl->excludes->selectedItems().isEmpty() );
    fImpl->delExtraBtn->setEnabled( !fImpl->extras->selectedItems().isEmpty() );
}

void CAddAttribute::slotAddExtra()
{
    auto text = QInputDialog::getText( this, "RSync Extra Option", "RSync Extra Options:" );
    if ( text.isEmpty() )
        return;

    addExtra( text );
}

void CAddAttribute::slotDelExtra()
{
    auto items = fImpl->extras->selectedItems();
    for ( auto && curr : items )
        delete curr;
}

void CAddAttribute::slotAddExclude()
{
    auto text = QInputDialog::getText( this, "RSync Exclude", "RSync Exclude:" );
    if ( text.isEmpty() )
        return;

    addExclude( text );
}

void CAddAttribute::slotDelExclude()
{
    auto items = fImpl->excludes->selectedItems();
    for ( auto && curr : items )
        delete curr;
}

void CAddAttribute::loadItem( QTreeWidgetItem * item )
{
    if ( !item )
        return;

    fImpl->dir->setText( item->text( 0 ) );
    for ( int ii = 0; ii < item->childCount(); ++ii )
    {
        auto child = item->child( ii );
        auto exclude = child->text( 1 );
        addExclude( exclude );

        auto extra = child->text( 2 );
        addExtra( extra );
    }
}

void CAddAttribute::addExclude( QString & exclude )
{
    if ( !exclude.isEmpty() )
    {
        new QListWidgetItem( exclude, fImpl->excludes );
    }
}

void CAddAttribute::addExtra( QString & extra )
{
    if ( !extra.isEmpty() )
    {
        new QListWidgetItem( extra, fImpl->extras );
    }
}

CAddAttribute::~CAddAttribute()
{
}

QString CAddAttribute::dir() const
{
    return fImpl->dir->text();
}

QStringList CAddAttribute::excludes() const
{
    QStringList retVal;
    for ( int ii = 0; ii < fImpl->excludes->count(); ++ii )
    {
        auto curr = fImpl->excludes->item( ii );
        if ( !curr )
            continue;
        retVal << curr->text();
    }
    return retVal;
}

QStringList CAddAttribute::extras() const
{
    QStringList retVal;
    for ( int ii = 0; ii < fImpl->extras->count(); ++ii )
    {
        auto curr = fImpl->extras->item( ii );
        if ( !curr )
            continue;
        retVal << curr->text();
    }
    return retVal;
}

