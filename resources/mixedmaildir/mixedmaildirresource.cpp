/*  This file is part of the KDE project
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com
    Copyright (C) 2011 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "mixedmaildirresource.h"
#include "mixedmaildir_debug.h"

#include "compactchangehelper.h"
#include "configdialog.h"
#include "mixedmaildirstore.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "retrieveitemsjob.h"
#include "createandsettagsjob.h"

#include "filestore/collectioncreatejob.h"
#include "filestore/collectiondeletejob.h"
#include "filestore/collectionfetchjob.h"
#include "filestore/collectionmodifyjob.h"
#include "filestore/collectionmovejob.h"
#include "filestore/entitycompactchangeattribute.h"
#include "filestore/itemcreatejob.h"
#include "filestore/itemdeletejob.h"
#include "filestore/itemfetchjob.h"
#include "filestore/itemmodifyjob.h"
#include "filestore/itemmovejob.h"
#include "filestore/storecompactjob.h"

#include <akonadi/kmime/messageparts.h>
#include <akonadi/kmime/messagestatus.h>

#include <AkonadiCore/changerecorder.h>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>
#include <AkonadiCore/itemmodifyjob.h>
#include <AkonadiCore/collectionfetchscope.h>

#include <kmime/kmime_message.h>

#include "mixedmaildirresource_debug.h"
#include <KLocalizedString>
#include <KWindowSystem>

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

#include <AkonadiCore/Tag>

using namespace Akonadi;

MixedMaildirResource::MixedMaildirResource(const QString &id)
    : ResourceBase(id), mStore(new MixedMaildirStore()), mCompactHelper(nullptr)
{
    new SettingsAdaptor(Settings::self());
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
            Settings::self(), QDBusConnection::ExportAdaptors);
    connect(this, &MixedMaildirResource::reloadConfiguration, this, &MixedMaildirResource::reapplyConfiguration);

    // We need to enable this here, otherwise we neither get the remote ID of the
    // parent collection when a collection changes, nor the full item when an item
    // is added.
    changeRecorder()->fetchCollection(true);
    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    setHierarchicalRemoteIdentifiersEnabled(true);

    if (ensureSaneConfiguration()) {
        const bool changeName = name().isEmpty() || name() == identifier() ||
                                name() == mStore->topLevelCollection().name();
        mStore->setPath(Settings::self()->path());
        if (changeName) {
            setName(mStore->topLevelCollection().name());
        }
    }

    const QByteArray compactHelperSessionId = id.toUtf8() + "-compacthelper";
    mCompactHelper = new CompactChangeHelper(compactHelperSessionId, this);
}

MixedMaildirResource::~MixedMaildirResource()
{
    delete mStore;
}

void MixedMaildirResource::aboutToQuit()
{
    // The settings may not have been saved if e.g. they have been modified via
    // DBus instead of the config dialog.
    Settings::self()->save();
}

void MixedMaildirResource::configure(WId windowId)
{
    ConfigDialog dlg;
    if (windowId) {
        KWindowSystem::setMainWindow(&dlg, windowId);
    }
    dlg.setWindowIcon(QIcon::fromTheme(QStringLiteral("message-rfc822")));

    bool fullSync = false;

    if (dlg.exec()) {
        const bool changeName = name().isEmpty() || name() == identifier() ||
                                name() == mStore->topLevelCollection().name();

        const QString oldPath = mStore->path();
        mStore->setPath(Settings::self()->path());

        fullSync = oldPath != mStore->path();

        if (changeName) {
            setName(mStore->topLevelCollection().name());
        }
        Q_EMIT configurationDialogAccepted();
    } else {
        Q_EMIT configurationDialogRejected();
    }

    if (ensureDirExists()) {
        if (fullSync) {
            mSynchronizedCollections.clear();
            mPendingSynchronizeCollections.clear();
        }
        synchronizeCollectionTree();
    }
}

void MixedMaildirResource::itemAdded(const Item &item, const Collection &collection)
{
    /*  qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << item.id() << "col=" << collection.remoteId();*/
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    FileStore::ItemCreateJob *job = mStore->createItem(item, collection);
    connect(job, &FileStore::ItemCreateJob::result, this, &MixedMaildirResource::itemAddedResult);
}

void MixedMaildirResource::itemChanged(const Item &item, const QSet<QByteArray> &parts)
{
    /*  qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << item.id() << "col=" << item.parentCollection().remoteId()
               << "parts=" << parts;*/
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    if (Settings::self()->readOnly()) {
        changeProcessed();
        return;
    }

    Item storeItem(item);
    storeItem.setRemoteId(mCompactHelper->currentRemoteId(item));

    FileStore::ItemModifyJob *job = mStore->modifyItem(storeItem);
    job->setIgnorePayload(!item.hasPayload<KMime::Message::Ptr>());
    job->setParts(parts);
    job->setProperty("originalRemoteId", storeItem.remoteId());
    connect(job, &FileStore::ItemModifyJob::result, this, &MixedMaildirResource::itemChangedResult);
}

void MixedMaildirResource::itemMoved(const Item &item, const Collection &source, const Collection &destination)
{
    /*  qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << item.id() << "remoteId=" << item.remoteId()
               << "source=" << source.remoteId() << "dest=" << destination.remoteId();*/
    if (source == destination) {
        changeProcessed();
        return;
    }

    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    Item moveItem = item;
    moveItem.setRemoteId(mCompactHelper->currentRemoteId(item));
    moveItem.setParentCollection(source);

    FileStore::ItemMoveJob *job = mStore->moveItem(moveItem, destination);
    job->setProperty("originalRemoteId", moveItem.remoteId());
    connect(job, &FileStore::ItemMoveJob::result, this, &MixedMaildirResource::itemMovedResult);
}

void MixedMaildirResource::itemRemoved(const Item &item)
{
    /*  qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << item.id() << "col=" << collection.remoteId()
               << "collection.remoteRevision=" << item.parentCollection().remoteRevision();*/
    Q_ASSERT(!item.remoteId().isEmpty());
    Q_ASSERT(item.parentCollection().isValid());
    if (item.parentCollection().remoteId().isEmpty()) {
        const QString message = i18nc("@info:status", "Item %1 belongs to invalid collection %2. Maybe it was deleted meanwhile?", item.id(), item.parentCollection().id());
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    Item storeItem(item);
    storeItem.setRemoteId(mCompactHelper->currentRemoteId(item));
    FileStore::ItemDeleteJob *job = mStore->deleteItem(storeItem);
    connect(job, &FileStore::ItemDeleteJob::result, this, &MixedMaildirResource::itemRemovedResult);
}

void MixedMaildirResource::retrieveCollections()
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    FileStore::CollectionFetchJob *job = mStore->fetchCollections(mStore->topLevelCollection(), FileStore::CollectionFetchJob::Recursive);
    connect(job, &FileStore::CollectionFetchJob::result, this, &MixedMaildirResource::retrieveCollectionsResult);

    status(Running, i18nc("@info:status", "Synchronizing email folders"));
}

void MixedMaildirResource::retrieveItems(const Collection &col)
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    RetrieveItemsJob *job = new RetrieveItemsJob(col, mStore, this);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::retrieveItemsResult);

    status(Running, i18nc("@info:status", "Synchronizing email folder %1", col.name()));
}

bool MixedMaildirResource::retrieveItems(const Item::List &items, const QSet<QByteArray> &parts)
{
    Q_UNUSED(parts);

    FileStore::ItemFetchJob *job = mStore->fetchItems(items);
    if (parts.contains(Item::FullPayload)) {
        job->fetchScope().fetchFullPayload(true);
    } else {
        for (const QByteArray &part : parts) {
            job->fetchScope().fetchPayloadPart(part, true);
        }
    }
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::retrieveItemResult);

    return true;
}

void MixedMaildirResource::collectionAdded(const Collection &collection, const Collection &parent)
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    FileStore::CollectionCreateJob *job = mStore->createCollection(collection, parent);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::collectionAddedResult);
}

void MixedMaildirResource::collectionChanged(const Collection &collection)
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    // when the top level collection gets renamed, we do not rename the directory
    // but rename the resource.
    if (collection.remoteId() == mStore->topLevelCollection().remoteId()) {
        if (collection.name() != name()) {
            qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "TopLevel collection name differs from resource name: collection="
                                              << collection.name() << "resource=" << name() << ". Renaming resource";
            setName(collection.name());
        }
        changeCommitted(collection);
        return;
    }

    mCompactHelper->checkCollectionChanged(collection);

    FileStore::CollectionModifyJob *job = mStore->modifyCollection(collection);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::collectionChangedResult);
}

void MixedMaildirResource::collectionChanged(const Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    // when the top level collection gets renamed, we do not rename the directory
    // but rename the resource.
    if (collection.remoteId() == mStore->topLevelCollection().remoteId()) {
        if (collection.name() != name()) {
            qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "TopLevel collection name differs from resource name: collection="
                                              << collection.name() << "resource=" << name() << ". Renaming resource";
            setName(collection.name());
        }
        changeCommitted(collection);
        return;
    }

    mCompactHelper->checkCollectionChanged(collection);

    Q_UNUSED(changedAttributes);

    FileStore::CollectionModifyJob *job = mStore->modifyCollection(collection);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::collectionChangedResult);
}

void MixedMaildirResource::collectionMoved(const Collection &collection, const Collection &source, const Collection &dest)
{
    //qCDebug(MIXEDMAILDIR_LOG) << collection << source << dest;

    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    if (collection.parentCollection() == Collection::root()) {
        const QString message = i18nc("@info:status", "Cannot move root maildir folder '%1'.", collection.remoteId());
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    if (source == dest) {   // should not happen, but who knows...
        changeProcessed();
        return;
    }

    Collection moveCollection = collection;
    moveCollection.setParentCollection(source);

    FileStore::CollectionMoveJob *job = mStore->moveCollection(moveCollection, dest);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::collectionMovedResult);
}

void MixedMaildirResource::collectionRemoved(const Collection &collection)
{
    if (!ensureSaneConfiguration()) {
        const QString message = i18nc("@info:status", "Unusable configuration.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        cancelTask(message);
        return;
    }

    if (collection.parentCollection() == Collection::root()) {
        Q_EMIT error(i18n("Cannot delete top-level maildir folder '%1'.", Settings::self()->path()));
        changeProcessed();
        return;
    }

    FileStore::CollectionDeleteJob *job = mStore->deleteCollection(collection);
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::collectionRemovedResult);
}

bool MixedMaildirResource::ensureDirExists()
{
    QDir dir(Settings::self()->path());
    if (!dir.exists()) {
        if (!dir.mkpath(Settings::self()->path())) {
            const QString message = i18nc("@info:status", "Unable to create maildir '%1'.", Settings::self()->path());
            qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
            status(Broken, message);
            return false;
        }
    }
    return true;
}

bool MixedMaildirResource::ensureSaneConfiguration()
{
    if (Settings::self()->path().isEmpty()) {
        const QString message = i18nc("@info:status", "No usable storage location configured.");
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << message;
        status(NotConfigured, message);
        return false;
    }
    return true;
}

void MixedMaildirResource::checkForInvalidatedIndexCollections(KJob *job)
{
    // when operations invalidate the on-disk index, we need to make sure all index
    // data has been transferred into Akonadi by synchronizing the collections
    const QVariant var = job->property("onDiskIndexInvalidated");
    if (var.isValid()) {
        const Collection::List collections = var.value<Collection::List>();
        qCDebug(MIXEDMAILDIR_LOG) << "On disk index of" << collections.count()
                                  << "collections invalidated after" << job->metaObject()->className();

        for (const Collection &collection : collections) {
            const Collection::Id id = collection.id();
            if (!mSynchronizedCollections.contains(id) && !mPendingSynchronizeCollections.contains(id)) {
                qCDebug(MIXEDMAILDIR_LOG) << "Requesting sync of collection" << collection.name()
                                          << ", id=" << collection.id();
                mPendingSynchronizeCollections << id;
                synchronizeCollection(id);
            }
        }
    }
}

void MixedMaildirResource::reapplyConfiguration()
{
    if (ensureSaneConfiguration() && ensureDirExists()) {
        const QString oldPath = mStore->path();
        mStore->setPath(Settings::self()->path());

        if (oldPath != mStore->path()) {
            mSynchronizedCollections.clear();
            mPendingSynchronizeCollections.clear();
        }
        synchronizeCollectionTree();
    }
}

void MixedMaildirResource::retrieveCollectionsResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::CollectionFetchJob *fetchJob = qobject_cast<FileStore::CollectionFetchJob *>(job);
    Q_ASSERT(fetchJob != nullptr);

    Collection topLevelCollection = mStore->topLevelCollection();
    if (!name().isEmpty() && name() != identifier()) {
        topLevelCollection.setName(name());
    }

    Collection::List collections;
    collections << topLevelCollection;
    collections << fetchJob->collections();
    collectionsRetrieved(collections);
}

void MixedMaildirResource::retrieveItemsResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    RetrieveItemsJob *retrieveJob = qobject_cast<RetrieveItemsJob *>(job);
    Q_ASSERT(retrieveJob != nullptr);

    // messages marked as deleted have been deleted from mbox files but never got purged
    // TODO FileStore could provide deleteItems() to deleted all filtered items in one go
    KJob *deleteJob = nullptr;
    qCDebug(MIXEDMAILDIR_LOG) << retrieveJob->itemsMarkedAsDeleted().count()
                              << "items marked as Deleted";
    Q_FOREACH (const Item &item, retrieveJob->itemsMarkedAsDeleted()) {
        deleteJob = mStore->deleteItem(item);
    }

    if (deleteJob != nullptr) {
        // last item delete triggers mbox purge, i.e. store compact
        const bool connected = connect(deleteJob, &KJob::result, this, &MixedMaildirResource::itemsDeleted);
        Q_ASSERT(connected);
        Q_UNUSED(connected);
    }

    // if some items have tags, we need to complete the retrieval and schedule tagging
    // to a later time so we can then fetch the items to get their Akonadi URLs
    const Item::List items = retrieveJob->availableItems();
    const QVariant var = retrieveJob->property("remoteIdToTagList");
    if (var.isValid()) {
        const QHash<QString, QVariant> tagListHash = var.value< QHash<QString, QVariant> >();
        if (!tagListHash.isEmpty()) {
            qCDebug(MIXEDMAILDIRRESOURCE_LOG) << tagListHash.count() << "of" << items.count()
                                              << "items in collection" << retrieveJob->collection().remoteId() << "have tags";

            TagContextList taggedItems;
            for (const Item &item : items) {
                const QVariant tagListVar = tagListHash[ item.remoteId() ];
                if (tagListVar.isValid()) {
                    const QStringList tagList = tagListVar.value<QStringList>();
                    if (!tagListHash.isEmpty()) {
                        TagContext tag;
                        tag.mItem = item;
                        tag.mTagList = tagList;

                        taggedItems << tag;
                    }
                }
            }

            if (!taggedItems.isEmpty()) {
                mTagContextByColId.insert(retrieveJob->collection().id(), taggedItems);

                scheduleCustomTask(this, "restoreTags", QVariant::fromValue<Collection>(retrieveJob->collection()));
            }
        }
    }

    mSynchronizedCollections << retrieveJob->collection().id();
    mPendingSynchronizeCollections.remove(retrieveJob->collection().id());

    itemsRetrievalDone();
}

void MixedMaildirResource::retrieveItemResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob *>(job);
    Q_ASSERT(fetchJob != nullptr);
    Q_ASSERT(!fetchJob->items().isEmpty());

    itemsRetrieved(fetchJob->items());
}

void MixedMaildirResource::itemAddedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::ItemCreateJob *itemJob = qobject_cast<FileStore::ItemCreateJob *>(job);
    Q_ASSERT(itemJob != nullptr);

    /*  qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << itemJob->item().id() << "remoteId=" << itemJob->item().remoteId();*/
    changeCommitted(itemJob->item());

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::itemChangedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::ItemModifyJob *itemJob = qobject_cast<FileStore::ItemModifyJob *>(job);
    Q_ASSERT(itemJob != nullptr);

    changeCommitted(itemJob->item());

    const QString remoteId = itemJob->property("originalRemoteId").toString();

    const QVariant compactStoreVar = itemJob->property("compactStore");
    if (compactStoreVar.isValid() && compactStoreVar.toBool()) {
        scheduleCustomTask(this, "compactStore", QVariant());
    }

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::itemMovedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::ItemMoveJob *itemJob = qobject_cast<FileStore::ItemMoveJob *>(job);
    Q_ASSERT(itemJob != nullptr);

    changeCommitted(itemJob->item());

    const QString remoteId = itemJob->property("originalRemoteId").toString();
//   qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "item.id=" << itemJob->item().id() << "remoteId=" << itemJob->item().remoteId()
//            << "old remoteId=" << remoteId;

    const QVariant compactStoreVar = itemJob->property("compactStore");
    if (compactStoreVar.isValid() && compactStoreVar.toBool()) {
        scheduleCustomTask(this, "compactStore", QVariant());
    }

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::itemRemovedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::ItemDeleteJob *itemJob = qobject_cast<FileStore::ItemDeleteJob *>(job);
    Q_ASSERT(itemJob != nullptr);

    changeCommitted(itemJob->item());

    const QString remoteId = itemJob->item().remoteId();

    const QVariant compactStoreVar = itemJob->property("compactStore");
    if (compactStoreVar.isValid() && compactStoreVar.toBool()) {
        scheduleCustomTask(this, "compactStore", QVariant());
    }

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::itemsDeleted(KJob *job)
{
    Q_UNUSED(job);
    scheduleCustomTask(this, "compactStore", QVariant());
}

void MixedMaildirResource::collectionAddedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::CollectionCreateJob *colJob = qobject_cast<FileStore::CollectionCreateJob *>(job);
    Q_ASSERT(colJob != nullptr);

    changeCommitted(colJob->collection());
}

void MixedMaildirResource::collectionChangedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::CollectionModifyJob *colJob = qobject_cast<FileStore::CollectionModifyJob *>(job);
    Q_ASSERT(colJob != nullptr);

    changeCommitted(colJob->collection());

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::collectionMovedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::CollectionMoveJob *colJob = qobject_cast<FileStore::CollectionMoveJob *>(job);
    Q_ASSERT(colJob != nullptr);

    changeCommitted(colJob->collection());

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::collectionRemovedResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::CollectionDeleteJob *colJob = qobject_cast<FileStore::CollectionDeleteJob *>(job);
    Q_ASSERT(colJob != nullptr);

    changeCommitted(colJob->collection());
}

void MixedMaildirResource::compactStore(const QVariant &arg)
{
    Q_UNUSED(arg);

    FileStore::StoreCompactJob *job = mStore->compactStore();
    connect(job, &RetrieveItemsJob::result, this, &MixedMaildirResource::compactStoreResult);
}

void MixedMaildirResource::compactStoreResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        status(Broken, job->errorString());
        cancelTask(job->errorString());
        return;
    }

    FileStore::StoreCompactJob *compactJob = qobject_cast<FileStore::StoreCompactJob *>(job);
    Q_ASSERT(compactJob != nullptr);

    const Item::List items = compactJob->changedItems();
    qCDebug(MIXEDMAILDIR_LOG) << "Compacting store resulted in" << items.count() << "changed items";

    mCompactHelper->addChangedItems(items);

    taskDone();

    checkForInvalidatedIndexCollections(job);
}

void MixedMaildirResource::restoreTags(const QVariant &arg)
{
    if (!arg.isValid()) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << "Given variant is not valid";
        cancelTask();
        return;
    }

    const Collection collection = arg.value<Collection>();
    if (!collection.isValid()) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << "Given variant is not valid";
        cancelTask();
        return;
    }

    const TagContextList taggedItems = mTagContextByColId[ collection.id() ];
    mPendingTagContexts << taggedItems;

    QMetaObject::invokeMethod(this, "processNextTagContext", Qt::QueuedConnection);
    taskDone();
}

void MixedMaildirResource::processNextTagContext()
{
    qCDebug(MIXEDMAILDIRRESOURCE_LOG) << mPendingTagContexts.count() << "items to go";
    if (mPendingTagContexts.isEmpty()) {
        return;
    }

    const TagContext tagContext = mPendingTagContexts.front();
    mPendingTagContexts.pop_front();

    ItemFetchJob *fetchJob = new ItemFetchJob(tagContext.mItem);
    fetchJob->setProperty("tagList", tagContext.mTagList);
    connect(fetchJob, &ItemFetchJob::result, this, &MixedMaildirResource::tagFetchJobResult);
}

void MixedMaildirResource::tagFetchJobResult(KJob *job)
{
    if (job->error() != 0) {
        qCCritical(MIXEDMAILDIRRESOURCE_LOG) << job->errorString();
        processNextTagContext();
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob != nullptr);

    Q_ASSERT(!fetchJob->items().isEmpty());

    const Item item = fetchJob->items().at(0);
    const QStringList tagList = job->property("tagList").toStringList();
    qCDebug(MIXEDMAILDIRRESOURCE_LOG) << "Tagging item" << item.url() << "with" << tagList;

    Akonadi::Tag::List tags;
    for (const QString &tag : tagList) {
        if (tag.isEmpty()) {
            qCWarning(MIXEDMAILDIRRESOURCE_LOG) << "TagList for item" << item.url() << "contains an empty tag";
        } else {
            tags << Akonadi::Tag(tag);
        }
    }
    new CreateAndSetTagsJob(item, tags);

    processNextTagContext();
}

AKONADI_RESOURCE_MAIN(MixedMaildirResource)

