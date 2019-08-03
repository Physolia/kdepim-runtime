/*
    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef BIRTHDAYSRESOURCE_H
#define BIRTHDAYSRESOURCE_H

#include <KCalendarCore/Event>

#include <resourcebase.h>

#include <QHash>

class QDate;

class BirthdaysResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    explicit BirthdaysResource(const QString &id);
    ~BirthdaysResource() override;

protected:
    using ResourceBase::retrieveItems; // Suppress -Woverload-virtual

protected:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;

private:
    void addPendingEvent(const KCalendarCore::Event::Ptr &event, const QString &remoteId);
    void checkForUnknownCategories(const QString &categoryToCheck, KCalendarCore::Event::Ptr &event);

    KCalendarCore::Event::Ptr createBirthday(const Akonadi::Item &contactItem);
    KCalendarCore::Event::Ptr createAnniversary(const Akonadi::Item &contactItem);
    KCalendarCore::Event::Ptr createEvent(const QDate &date);

private Q_SLOTS:
    void doFullSearch();
    void listContacts(const Akonadi::Collection::List &cols);
    void createEvents(const Akonadi::Item::List &items);

    void contactChanged(const Akonadi::Item &item);
    void contactRemoved(const Akonadi::Item &item);

    void contactRetrieved(KJob *job);
private:
    void slotReloadConfig();
    QHash<QString, Akonadi::Item> mPendingItems;
    QHash<QString, Akonadi::Item> mDeletedItems;
};

#endif
