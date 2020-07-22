/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "etesyncresource.h"

#include <kwindowsystem.h>

#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>
#include <KLocalizedString>
#include <QDBusConnection>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncadapter.h"
#include "journalsfetchjob.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "setupwizard.h"

using namespace EteSyncAPI;
using namespace Akonadi;

#define ROOT_COLLECTION_REMOTEID QStringLiteral("EteSyncRootCollection")

EteSyncResource::EteSyncResource(const QString &id)
    : ResourceBase(id)
{
    Settings::instance(KSharedConfig::openConfig());
    new SettingsAdaptor(Settings::self());

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(),
                                                 QDBusConnection::ExportAdaptors);

    setName(i18n("EteSync Resource"));

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    // Make local contacts directory
    initialiseDirectory(baseDirectoryPath());

    mClientState = new EteSyncClientState();
    connect(mClientState, &EteSyncClientState::clientInitialised, this, &EteSyncResource::initialiseDone);
    mClientState->init();

    mContactHandler = ContactHandler::Ptr(new ContactHandler(this));
    mCalendarHandler = CalendarHandler::Ptr(new CalendarHandler(this));
    mTaskHandler = TaskHandler::Ptr(new TaskHandler(this));

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &EteSyncResource::onReloadConfiguration);

    qCDebug(ETESYNC_LOG) << "Resource started";
}

EteSyncResource::~EteSyncResource()
{
}

void EteSyncResource::configure(WId windowId)
{
    SetupWizard wizard(mClientState);

    if (windowId) {
        wizard.setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(wizard.windowHandle(), windowId);
    }
    const int result = wizard.exec();
    if (result == QDialog::Accepted) {
        synchronize();
        mClientState->saveSettings();
        Q_EMIT configurationDialogAccepted();
    } else {
        Q_EMIT configurationDialogRejected();
    }
}

void EteSyncResource::retrieveCollections()
{
    // Set up root collection for resource
    mResourceCollection = Collection();
    mResourceCollection.setContentMimeTypes({Collection::mimeType()});
    mResourceCollection.setName(mClientState->username());
    mResourceCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    mResourceCollection.setParentCollection(Collection::root());
    mResourceCollection.setRights(Collection::CanCreateCollection);

    Akonadi::CachePolicy cachePolicy;
    cachePolicy.setInheritFromParent(false);
    cachePolicy.setSyncOnDemand(false);
    cachePolicy.setCacheTimeout(-1);
    cachePolicy.setIntervalCheckTime(5);
    mResourceCollection.setCachePolicy(cachePolicy);

    EntityDisplayAttribute *attr = mResourceCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(mClientState->username());
    attr->setIconName(QStringLiteral("akonadi-etesync"));

    auto job = new JournalsFetchJob(mClientState->client(), this);
    connect(job, &JournalsFetchJob::finished, this, [this](KJob *job) {
        if (job->error()) {
            qCWarning(ETESYNC_LOG) << "Error in fetching journals";
            return;
        }
        qCDebug(ETESYNC_LOG) << "Retrieving collections";
        EteSyncJournal **journals = qobject_cast<JournalsFetchJob *>(job)->journals();
        Collection::List list;
        list << mResourceCollection;
        for (EteSyncJournal **iter = journals; *iter; iter++) {
            EteSyncJournalPtr journal(*iter);

            Collection collection;
            collection.setParentCollection(mResourceCollection);
            setupCollection(collection, journal.get());

            list << collection;
        }
        free(journals);
        collectionsRetrieved(list);
    });
    job->start();
}

void EteSyncResource::setupCollection(Collection &collection, EteSyncJournal *journal)
{
    EteSyncCryptoManagerPtr cryptoManager(etesync_journal_get_crypto_manager(journal, mClientState->derived(), mClientState->keypair()));

    EteSyncCollectionInfoPtr info(etesync_journal_get_info(journal, cryptoManager.get()));

    const QString type = QStringFromCharPtr(CharPtr(etesync_collection_info_get_type(info.get())));

    QStringList mimeTypes;

    auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);

    if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK)) {
        mimeTypes << KContacts::Addressee::mimeType();
        attr->setIconName(QStringLiteral("view-pim-contacts"));
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR)) {
        mimeTypes << KCalendarCore::Event::eventMimeType();
        attr->setIconName(QStringLiteral("view-calendar"));
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS)) {
        mimeTypes << KCalendarCore::Todo::todoMimeType();
        attr->setIconName(QStringLiteral("view-pim-tasks"));
    } else {
        qCWarning(ETESYNC_LOG) << "Unknown journal type. Cannot set collection mime type.";
    }

    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_journal_get_uid(journal)));
    const QString displayName = QStringFromCharPtr(CharPtr(etesync_collection_info_get_display_name(info.get())));
    auto collectionColor = etesync_collection_info_get_color(info.get());
    auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>(Collection::AddIfMissing);
    colorAttr->setColor(collectionColor);

    collection.setRemoteId(journalUid);
    collection.setName(displayName);
    collection.setContentMimeTypes(mimeTypes);
}

void EteSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    QString journalUid = collection.remoteId();
    QString prevUid = collection.remoteRevision();

    auto job = new EntriesFetchJob(mClientState->client(), journalUid, prevUid, this);

    connect(job, &EntriesFetchJob::finished, this, &EteSyncResource::slotItemsRetrieved);

    job->start();
}

void EteSyncResource::slotItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qCWarning(ETESYNC_LOG) << job->errorText();
        return;
    }

    qCDebug(ETESYNC_LOG) << "Retrieving entries";
    EteSyncEntry **entries = qobject_cast<EntriesFetchJob *>(job)->entries();

    Collection collection = currentCollection();

    if (collection.contentMimeTypes().contains(KContacts::Addressee::mimeType())) {
        mContactHandler->setupItems(entries, collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Event::eventMimeType())) {
        mCalendarHandler->setupItems(entries, collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Todo::todoMimeType())) {
        mTaskHandler->setupItems(entries, collection);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        itemsRetrieved({});
    }
}

void EteSyncResource::aboutToQuit()
{
    delete mClientState;
}

void EteSyncResource::onReloadConfiguration()
{
    qCDebug(ETESYNC_LOG) << "Resource config reload";
    synchronize();
}

void EteSyncResource::initialiseDone(bool successful)
{
    qCDebug(ETESYNC_LOG) << "Resource intialised";
    if (successful) {
        synchronize();
    }
}

QString EteSyncResource::baseDirectoryPath() const
{
    return Settings::self()->basePath();
}

void EteSyncResource::initialiseDirectory(const QString &path) const
{
    QDir dir(path);

    // if folder does not exists, create it
    QDir::root().mkpath(dir.absolutePath());

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QStringLiteral("/WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write(
            "Important warning!\n\n"
            "Do not create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

void EteSyncResource::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Item added" << item.mimeType();
    if (collection.contentMimeTypes().contains(KContacts::Addressee::mimeType())) {
        mContactHandler->itemAdded(item, collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Event::eventMimeType())) {
        mCalendarHandler->itemAdded(item, collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Todo::todoMimeType())) {
        mTaskHandler->itemAdded(item, collection);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        changeCommitted(item);
    }
}

void EteSyncResource::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    qCDebug(ETESYNC_LOG) << "Item changed" << item.mimeType();
    if (item.mimeType() == KContacts::Addressee::mimeType()) {
        mContactHandler->itemChanged(item, parts);
    } else if (item.mimeType() == KCalendarCore::Event::eventMimeType()) {
        mCalendarHandler->itemChanged(item, parts);
    } else if (item.mimeType() == KCalendarCore::Todo::todoMimeType()) {
        mTaskHandler->itemChanged(item, parts);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        changeCommitted(item);
    }
}

void EteSyncResource::itemRemoved(const Akonadi::Item &item)
{
    qCDebug(ETESYNC_LOG) << "Item removed" << item.mimeType();
    if (item.mimeType() == KContacts::Addressee::mimeType()) {
        mContactHandler->itemRemoved(item);
    } else if (item.mimeType() == KCalendarCore::Event::eventMimeType()) {
        mCalendarHandler->itemRemoved(item);
    } else if (item.mimeType() == KCalendarCore::Todo::todoMimeType()) {
        mTaskHandler->itemRemoved(item);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        changeCommitted(item);
    }
}

void EteSyncResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    qCDebug(ETESYNC_LOG) << "Collection added" << collection.mimeType();
    if (collection.contentMimeTypes().contains(KContacts::Addressee::mimeType())) {
        mContactHandler->collectionAdded(collection, parent);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Event::eventMimeType())) {
        mCalendarHandler->collectionAdded(collection, parent);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Todo::todoMimeType())) {
        mTaskHandler->collectionAdded(collection, parent);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        changeCommitted(collection);
    }
}

void EteSyncResource::collectionChanged(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Collection changed" << collection.mimeType();
    if (collection.contentMimeTypes().contains(KContacts::Addressee::mimeType())) {
        mContactHandler->collectionChanged(collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Event::eventMimeType())) {
        mCalendarHandler->collectionChanged(collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Todo::todoMimeType())) {
        mTaskHandler->collectionChanged(collection);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
        changeCommitted(collection);
    }
}

void EteSyncResource::collectionRemoved(const Akonadi::Collection &collection)
{
    /// TODO: Remove local contacts for this collection
    qCDebug(ETESYNC_LOG) << "Collection removed" << collection.mimeType();
    if (collection.contentMimeTypes().contains(KContacts::Addressee::mimeType())) {
        mContactHandler->collectionRemoved(collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Event::eventMimeType())) {
        mCalendarHandler->collectionRemoved(collection);
    } else if (collection.contentMimeTypes().contains(KCalendarCore::Todo::todoMimeType())) {
        mTaskHandler->collectionRemoved(collection);
    } else {
        qCDebug(ETESYNC_LOG) << "Unknown MIME type";
    }
}

AKONADI_RESOURCE_MAIN(EteSyncResource)
