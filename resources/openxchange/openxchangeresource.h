/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef OPENXCHANGERESOURCE_H
#define OPENXCHANGERESOURCE_H

#include <akonadi/resourcebase.h>

class OpenXchangeResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    OpenXchangeResource( const QString &id );
    ~OpenXchangeResource();

  public Q_SLOTS:
    virtual void configure( WId windowId );
    virtual void aboutToQuit();

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &collection );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );

  protected:
    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );

    virtual void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    virtual void collectionChanged( const Akonadi::Collection &collection );
    virtual void collectionRemoved( const Akonadi::Collection &collection );

  private Q_SLOTS:
    void onUserIdRequestJobFinished( KJob* );
    void onFoldersRequestJobFinished( KJob* );

  private:

};

#endif
