/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

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

#ifndef RESOURCETASK_H
#define RESOURCETASK_H

#include <QtCore/QObject>

#include <Akonadi/Collection>
#include <Akonadi/Item>

#include "resourcestateinterface.h"

namespace KIMAP
{
  class Session;
}

class SessionPool;

class ResourceTask : public QObject
{
  Q_OBJECT

public:
  explicit ResourceTask( ResourceStateInterface::Ptr resource, QObject *parent = 0 );
  virtual ~ResourceTask();

  void start( SessionPool *pool );

protected:
  virtual void doStart( KIMAP::Session *session ) = 0;

protected:
  Akonadi::Collection collection() const;
  Akonadi::Item item() const;

  Akonadi::Collection parentCollection() const;

  Akonadi::Collection sourceCollection() const;
  Akonadi::Collection targetCollection() const;

  QSet<QByteArray> parts() const;

  QString rootRemoteId() const;
  QString mailBoxForCollection( const Akonadi::Collection &collection ) const;

  void itemRetrieved( const Akonadi::Item &item );
  void cancelTask( const QString &errorString );
  void deferTask();

private slots:
  void onSessionRequested( qint64 requestId, KIMAP::Session *session,
                           int errorCode, const QString &errorString );


private:
  SessionPool *m_pool;
  qint64 m_sessionRequestId;

  KIMAP::Session *m_session;
  ResourceStateInterface::Ptr m_resource;
};

#endif
