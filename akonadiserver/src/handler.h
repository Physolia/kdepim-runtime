/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#ifndef AKONADIHANDLER_H
#define AKONADIHANDLER_H

#include <QObject>

#include "global.h"

namespace Akonadi {
    class Response;
    class AkonadiConnection;

/**
The handler interfaces describes an entity capable of handling an AkonadiIMAP command.*/
class Handler : public QObject {
    Q_OBJECT
public:
    Handler();

    virtual ~Handler();

    /**
     * Set the tag of the command to be processed, and thus of the response
     * generated by this handler.
     * @param tag The command tag, an alphanumerical string, normally.
     */
    void setTag( const QByteArray& tag );

    /**
     * The tag of the command associated with this handler.
     */
    QByteArray tag() const;
    
    /**
     * Process one line of input.
     * @param line The input.
     * @return false if the handler expects to read more data from the client, true otherwise.
     */
    virtual bool handleLine( const QByteArray & line );

    static Handler* findHandlerForCommandAlwaysAllowed( const QByteArray& line );
    static Handler* findHandlerForCommandNonAuthenticated( const QByteArray& line );
    static Handler* findHandlerForCommandAuthenticated( const QByteArray& line );
    static Handler* findHandlerForCommandSelected( const QByteArray& line );

    void setConnection( AkonadiConnection* connection );
    AkonadiConnection* connection();
    
signals:
    
    /**
     * Emitted whenever the handler has a response ready for output. There can
     * be several responses per command.
     * @param response The response to be sent to the client.
     */
    void responseAvailable( const Response& response );

    /**
     * Emitted whenever a handler wants the connection to change into a
     * different state. The connection usually honors such requests, but
     * the decision is up to it.
     * @param state The new state the handler suggests to enter.
     */
    void connectionStateChange( ConnectionState state );
private:
    QByteArray m_tag;
    AkonadiConnection* m_connection;
};

}

#endif
