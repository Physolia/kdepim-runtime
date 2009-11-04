/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DAVCALENDARRESOURCE_H
#define DAVCALENDARRESOURCE_H

#include <akonadi/resourcebase.h>

#include "../common/davaccessor.h"

namespace Akonadi {
  class KCalMimeTypeVisitor;
}

class davCalendarResource : public Akonadi::ResourceBase,
                            public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    davCalendarResource( const QString &id );
    ~davCalendarResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    
    void accessorStatus( const QString &status );
    void accessorError( const QString &err, bool cancelRequest );
    void accessorRetrievedCollection( const QString &url, const QString &name );
    void accessorRetrievedCollections();
    void accessorRetrievedItem( const davItem &item );
    void accessorRetrievedItemFromCache( const davItem &item );
    void accessorRetrievedItems();
    void accessorRemovedItem();
    void accessorPutItem( const KUrl &oldUrl, const KUrl &newUrl );
    
    void backendItemChanged( const davItem &item );
    void backendItemsRemoved( const QList<davItem> &items);

  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    
  private:
    void createAccessor();
    Akonadi::Item createItem( const QByteArray &data );
    Akonadi::KCalMimeTypeVisitor *mMimeVisitor;
    davAccessor *accessor;
    
    QMap<QString, Akonadi::Item> putItems;
};

#endif
