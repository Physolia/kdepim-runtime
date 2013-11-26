/*
  Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Andras Mantia <andras@kdab.net>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "incidencehandler.h"

#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>

#include <KCalCore/CalFormat>

#include <KLocale>

IncidenceHandler::IncidenceHandler( const Akonadi::Collection &imapCollection )
  : KolabHandler( imapCollection ),
    m_calendar( QString::fromLatin1( "UTC" ) )
{
}

IncidenceHandler::~IncidenceHandler()
{
}

Akonadi::Item::List IncidenceHandler::translateItems( const Akonadi::Item::List &items )
{
  Akonadi::Item::List newItems;
  Q_FOREACH ( const Akonadi::Item &item, items ) {
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
      kWarning() << "Payload is not a MessagePtr!";
      continue;
    }
    const KMime::Message::Ptr payload = item.payload<KMime::Message::Ptr>();

    KCalCore::Incidence::Ptr incidencePtr = Kolab::KolabObjectReader(payload).getIncidence();
    if ( checkForErrors( item.id() ) ) {
      continue;
    }
    if ( !incidencePtr ) {
      kWarning() << "Failed to read incidence.";
      continue;
    }
    Akonadi::Item newItem( incidencePtr->mimeType() );
    newItem.setPayload( incidencePtr );
    newItem.setRemoteId( QString::number( item.id() ) );
    newItems << newItem;
  }

  return newItems;
}

bool IncidenceHandler::toKolabFormat( const Akonadi::Item &item, Akonadi::Item &imapItem )
{
  KCalCore::Incidence::Ptr incidencePtr;
  if ( item.hasPayload<KCalCore::Incidence::Ptr>() ) {
    incidencePtr = item.payload<KCalCore::Incidence::Ptr>();
  } else {
    kWarning() << "item is not an incidence";
    return false;
  }
  incidenceToItem( incidencePtr, imapItem );
  if ( checkForErrors( item.id() ) ) {
    imapItem.setPayloadFromData( "" );
    return false;
  }
  return true;
}

void IncidenceHandler::incidenceToItem( const KCalCore::Incidence::Ptr &incidencePtr,
                                        Akonadi::Item &imapItem )
{
  if ( !incidencePtr ) {
    imapItem.setPayloadFromData( "" );
    return;
  }
  const KMime::Message::Ptr &message = incidenceToMime( incidencePtr );
  imapItem.setMimeType( QLatin1String("message/rfc822") );
  imapItem.setPayload( message );
}
