/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "imaplibresource.h"
#include "imaplib.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>
#include <kpassworddialog.h>
#include <kmessagebox.h>
#include <kwallet.h>
using KWallet::Wallet;

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kmime/kmime_message.h>

using namespace Akonadi;

ImaplibResource::ImaplibResource( const QString &id )
   :ResourceBase( id )
{
    // For now, read the mailody settings. Need to figure out how to set mailody up for settings().
    KConfig* tempConfig = new KConfig(KStandardDirs::locate("config", "mailodyrc"));
    KConfigGroup config = tempConfig->group("General");
    const QString imapServer = config.readEntry("imapServer");
    int safe = config.readEntry("safeImap",3);

    QString server = imapServer.section(":",0,0);
    int port = imapServer.section(":",1,1).toInt();

    m_imap = new Imaplib(0,"serverconnection");

    /* TODO: copy cryptoConnectionSupport or do this somewhere else ? 
    if ((safe == 1 || safe == 2) && !Global::cryptoConnectionSupported())
    {
        kDebug(50002) << "Crypto not supported!" << endl;
        slotError(i18n("You requested TLS/SSL, but your "
                       "system does not seem to be set up for that."));
        return;
    }
    */

    m_imap->startConnection(server, port, safe);
    connections();
}

ImaplibResource::~ImaplibResource()
{
    delete m_imap;
}

bool ImaplibResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  kDebug(50002) << "Implement me!";
  return false;

  /*
  KMime::Message *mail = new KMime::Message();
  mail->setContent( data );
  mail->parse();

  Item i( item );
  i.setMimeType( "message/rfc822" );
  i.setPayload( MessagePtr( mail ) );
  itemRetrieved( i );
  return true;
  */
}

void ImaplibResource::configure()
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemChanged( const Akonadi::Item& item, const QStringList& parts )
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::itemRemoved(const Akonadi::DataReference & ref)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::retrieveCollections()
{
    m_imap->getMailBoxList();
}

void ImaplibResource::slotGetMailBoxList(const QStringList& list)
{
    Collection::List collections;
    
    QStringList::ConstIterator it = list.begin();
    while (it != list.end())
    {
        Collection c; 
        c.setName( *it );
        c.setRemoteId( *it );
        kDebug(50002) << "ADDING: " << (*it) << endl;
        collections << c;
        ++it;
    }

    collectionsRetrieved( collections ); 
}

void ImaplibResource::retrieveItems(const Akonadi::Collection & col, const QStringList &parts)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionAdded(const Collection & collection, const Collection &parent)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionChanged(const Collection & collection)
{
  kDebug(50002) << "Implement me!";
}

void ImaplibResource::collectionRemoved(int id, const QString & remoteId)
{
  qDebug() << "Implement me!";
}

/******************* Slots  ***********************************************/

void ImaplibResource::slotLogin( Imaplib* connection)
{
    // kDebug(50002) << endl;

    // For now, read the mailody settings. Need to figure out how to set mailody up for settings().
    KConfig* tempConfig = new KConfig(KStandardDirs::locate("config", "mailodyrc"));
    KConfigGroup config = tempConfig->group("General");
    QString login = config.readEntry("userName");
    QString pass;

    Wallet* wallet = Wallet::openWallet(Wallet::NetworkWallet(), 0 /* TODO: anything more intelligent possible?*/);
    if (wallet && wallet->isOpen() && wallet->hasFolder("mailody"))
    {
        wallet->setFolder( "mailody" );
        wallet->readPassword("account1", pass);
    }
    delete wallet;

    if (pass.isEmpty())
    {
        manualAuth( connection, login);
    }
    else
    {
        connection->login(login, pass);
    }
}

void ImaplibResource::slotLoginFailed(Imaplib* connection)
{
    // the credentials where not ok....
    int i = KMessageBox::questionYesNoCancel(0,
                i18n("The server refused the supplied username and password. "
                     "Do you want to go to the settings, re-enter it for one "
                     "time or do nothing?"),
                i18n("Could Not Log In"),
                   KGuiItem(i18n("Settings")), KGuiItem(i18n("Single Input")));
    if (i == KMessageBox::Yes)
        configure();
    else if (i == KMessageBox::No)
    {
        // For now, read the mailody settings. Need to figure out how to set mailody up for settings().
        KConfig* tempConfig = new KConfig(KStandardDirs::locate("config", "mailodyrc"));
        KConfigGroup config = tempConfig->group("General");
        QString username = config.readEntry("userName");
        manualAuth(connection, username);
    }
    else
        connection->logout();
}

void ImaplibResource::slotAlert(Imaplib*, const QString& message)
{
    KMessageBox::information(0, i18n("Server reported: %1",message));
}

/******************* Private ***********************************************/

void ImaplibResource::connections()
{
    connect(m_imap,
            SIGNAL(login( Imaplib* )),
            SLOT( slotLogin( Imaplib* ) ));
    connect(m_imap,
            SIGNAL(loginOk( Imaplib* )),
            SIGNAL( loginOk() ));
    connect(m_imap,
            SIGNAL(status( const QString& )),
            SIGNAL(status( const QString& )));
    connect(m_imap,
            SIGNAL(statusReady()),
            SIGNAL(statusReady()));
    connect(m_imap,
            SIGNAL(statusError( const QString& )),
            SIGNAL(statusError( const QString& )));
    connect(m_imap,
            SIGNAL(saveDone()),
            SIGNAL(saveDone()));
    connect(m_imap,
            SIGNAL(error(const QString&)),
            SLOT(slotError(const QString&)));
    connect(m_imap,
            SIGNAL(unexpectedDisconnect()),
            SLOT(slotDisconnected()));
    connect(m_imap,
            SIGNAL(disconnected()),
            SIGNAL(disconnected()));
    connect(m_imap,
            SIGNAL(loginFailed( Imaplib* )),
            SLOT( slotLoginFailed( Imaplib* ) ));
    connect(m_imap,
            SIGNAL(alert( Imaplib*, const QString& )),
            SLOT( slotAlert( Imaplib*, const QString& ) ));
    connect(m_imap,
            SIGNAL(mailBoxList(const QStringList&)),
            SLOT( slotGetMailBoxList(const QStringList& ) ));
    connect(m_imap,
            SIGNAL(mailBox( Imaplib*, const QString&, const QStringList& )),
            SLOT( slotGetMailBox( Imaplib*, const QString&, const QStringList& ) ));
    connect(m_imap,
            SIGNAL(message( Imaplib*, const QString&, int, const QString& )),
            SLOT( slotGetMessage( Imaplib*, const QString&, int, const QString& ) ));
    connect(m_imap,
            SIGNAL(messageCount(Imaplib*, const QString&, int)),
            SLOT(slotMessagesInMailbox(Imaplib*, const QString&, int) ));
    connect(m_imap,
            SIGNAL(unseenCount(Imaplib*, const QString&, int)),
            SLOT(slotUnseenMessagesInMailbox(Imaplib*, const QString& , int ) ));
    connect(m_imap,
            SIGNAL(mailBoxAdded(const QString&)),
            SLOT(slotMailBoxAdded(const QString&)));
    connect(m_imap,
            SIGNAL(mailBoxDeleted(const QString&)),
            SLOT(slotMailBoxRemoved(const QString&)));
    connect(m_imap,
            SIGNAL(mailBoxRenamed(const QString&, const QString&)),
            SLOT(slotMailBoxRenamed(const QString&, const QString&)));
    connect(m_imap,
            SIGNAL(expungeCompleted(Imaplib*, const QString&)),
            SLOT(slotMailBoxExpunged(Imaplib*, const QString&)));
    connect(m_imap,
            SIGNAL(itemsInMailBox(Imaplib*,const QString&,const QStringList&)),
            SLOT(slotMailBoxItems(Imaplib*,const QString&,const QStringList&)));
    connect(m_imap,
            SIGNAL(integrity(const QString&, int, const QString&,
                   const QString&)),
            SLOT(slotIntegrity(const QString&, int, const QString&,
                 const QString&)));
}

void ImaplibResource::manualAuth(Imaplib* connection, const QString& username)
{
    // kDebug(50002) << endl;

    KPasswordDialog dlg( 0 /* todo: sane? */);
    dlg.setPrompt( i18n("Could not find a valid password, please enter it here") );
    if (dlg.exec() == QDialog::Accepted && !dlg.password().isEmpty())
            connection->login(username, QString(dlg.password()));
    else
        connection->logout();
}

#include "imaplibresource.moc"
