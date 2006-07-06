/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#ifndef PIM_RESOURCE_H
#define PIM_RESOURCE_H

#include <QObject>

#include <kdepim_export.h>

namespace PIM {

/**
 * Abstract interface for all resource agent classes.
 * You should however use @see ResourceBase as base class, as it
 * provides a lot of convenience methods and abstracts parts
 * of the protocol.
 */
class AKONADI_RESOURCES_EXPORT Resource : public QObject
{
  Q_OBJECT

  public:
    typedef QList<Resource*> List;

    /**
     * Destroys the resource.
     */
    virtual ~Resource() { };

  public Q_SLOTS:
    /**
     * This method is called whenever an external query for putting data in the
     * storage is received.
     *
     * @param uid The uid of the item that is requested.
     * @param collection The id of the collection where the data should be put.
     * @param type The type of the data that shall be put, either a full object or
     *             just a lightweight version.
     */
    virtual bool requestItemDelivery( const QString & uid, const QString & collection, int type ) = 0;
};

}

#endif
