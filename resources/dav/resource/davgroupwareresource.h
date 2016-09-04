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

#ifndef DAVGROUPWARERESOURCE_H
#define DAVGROUPWARERESOURCE_H

#include "etagcache.h"

#include <resourcebase.h>
#include <akonadi/calendar/freebusyproviderbase.h>

class DavFreeBusyHandler;
class DavItem;
class KDateTime;

#include <QtCore/QSet>
#include <QtCore/QStringList>

class DavGroupwareResource : public Akonadi::ResourceBase,
    public Akonadi::AgentBase::Observer,
    public Akonadi::FreeBusyProviderBase
{
    Q_OBJECT

public:
    explicit DavGroupwareResource(const QString &id);
    ~DavGroupwareResource();

    void collectionRemoved(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void cleanup() Q_DECL_OVERRIDE;

    KDateTime lastCacheUpdate() const Q_DECL_OVERRIDE;
    void canHandleFreeBusy(const QString &email) const Q_DECL_OVERRIDE;
    void retrieveFreeBusy(const QString &email, const KDateTime &start, const KDateTime &end) Q_DECL_OVERRIDE;

public Q_SLOTS:
    void configure(WId windowId) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void createInitialCache();
    void onCreateInitialCacheReady(KJob *);

protected Q_SLOTS:
    void retrieveCollections() Q_DECL_OVERRIDE;
    void retrieveItems(const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;

protected:
    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) Q_DECL_OVERRIDE;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) Q_DECL_OVERRIDE;
    void itemRemoved(const Akonadi::Item &item) Q_DECL_OVERRIDE;
    void doSetOnline(bool online) Q_DECL_OVERRIDE;

private:
    enum ItemFetchUpdateType {
        ItemUpdateNone,
        ItemUpdateAdd,
        ItemUpdateChange
    };

    void onReloadConfig();
    void onCollectionRemovedFinished(KJob *);

    void onHandlesFreeBusy(const QString &email, bool handles);
    void onFreeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText);

    void onRetrieveCollectionsFinished(KJob *);
    void onRetrieveItemsFinished(KJob *);
    void onMultigetFinished(KJob *);
    void onRetrieveItemFinished(KJob *);
    /**
      * Called when a new item has been fetched from the backend.
      *
      * @param job The job that fetched the item
      * @param updateType The type of update that triggered this call. The task notification sent
      *        sent to Akonadi will depend on this flag.
      */
    void onItemFetched(KJob *job, ItemFetchUpdateType updateType);
    void onItemRefreshed(KJob *job);

    void onItemAddedFinished(KJob *);
    void onItemChangePrepared(KJob *);
    void onItemChangedFinished(KJob *);
    void onItemRemovalPrepared(KJob *);
    void onItemRemovedFinished(KJob *);

    void onCollectionDiscovered(int protocol, const QString &collectionUrl, const QString &configuredUrl);
    void onConflictModifyJobFinished(KJob *job);
    void onDeletedItemRecreated(KJob *job);

    void doItemChange(const Akonadi::Item &item, const Akonadi::Item::List &dependentItems = Akonadi::Item::List());
    void doItemRemoval(const Akonadi::Item &item);
    void handleConflict(const Akonadi::Item &localItem,
                        const Akonadi::Item::List &localDependentItems,
                        const DavItem &remoteItem,
                        bool isLocalRemoval,
                        int responseCode
                       );

    bool configurationIsValid();
    void retryAfterFailure(const QString &errorMessage);

    /**
     * Collections which only support one mime type have an icon indicating what they support.
     */
    static void setCollectionIcon(Akonadi::Collection &collection);

    Akonadi::Collection mDavCollectionRoot;
    QMap<QString, EtagCache *> mEtagCaches;
    QMap<QString, QString> mCTagCache;
    DavFreeBusyHandler *mFreeBusyHandler;
    bool mSyncErrorNotified;
};

#endif
