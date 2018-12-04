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

#include "contactsresource.h"

#include "settings.h"
#include "contactsresourcesettingsadaptor.h"

#include <QDir>
#include <QFile>

#include <changerecorder.h>
#include <collectionfetchscope.h>
#include <entitydisplayattribute.h>
#include <itemfetchscope.h>
#include <kdbusconnectionpool.h>
#include "contacts_resources_debug.h"

#include <KLocalizedString>

using namespace Akonadi;

ContactsResource::ContactsResource(const QString &id)
    : ResourceBase(id)
{
    // setup the resource
    ContactsResourceSettings::instance(KSharedConfig::openConfig());
    new ContactsResourceSettingsAdaptor(ContactsResourceSettings::self());
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Settings"),
                                                           ContactsResourceSettings::self(), QDBusConnection::ExportAdaptors);

    changeRecorder()->fetchCollection(true);
    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    setHierarchicalRemoteIdentifiersEnabled(true);

    mSupportedMimeTypes << KContacts::Addressee::mimeType() << KContacts::ContactGroup::mimeType() << Collection::mimeType();

    if (name().startsWith(QLatin1String("akonadi_contacts_resource"))) {
        setName(i18n("Personal Contacts"));
    }

    // Make sure we have a valid directory (XDG dirs want this very much).
    initializeDirectory(ContactsResourceSettings::self()->path());

    if (ContactsResourceSettings::self()->isConfigured()) {
        synchronize();
    }
    connect(this, &ContactsResource::reloadConfiguration, this, &ContactsResource::slotReloadConfig);
}

ContactsResource::~ContactsResource()
{
    delete ContactsResourceSettings::self();
}

void ContactsResource::aboutToQuit()
{
}

void ContactsResource::slotReloadConfig()
{
    ContactsResourceSettings::self()->setIsConfigured(true);
    ContactsResourceSettings::self()->save();

    clearCache();
    initializeDirectory(baseDirectoryPath());

    synchronize();
}

Collection::List ContactsResource::createCollectionsForDirectory(const QDir &parentDirectory, const Collection &parentCollection) const
{
    Collection::List collections;

    QDir dir(parentDirectory);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    const QFileInfoList entries = dir.entryInfoList();
    collections.reserve(entries.count() * 2);

    for (const QFileInfo &entry : entries) {
        QDir subdir(entry.absoluteFilePath());

        Collection collection;
        collection.setParentCollection(parentCollection);
        collection.setRemoteId(entry.fileName());
        collection.setName(entry.fileName());
        collection.setContentMimeTypes(mSupportedMimeTypes);
        collection.setRights(supportedRights(false));

        collections << collection;
        collections << createCollectionsForDirectory(subdir, collection);
    }

    return collections;
}

void ContactsResource::retrieveCollections()
{
    // create the resource collection
    Collection resourceCollection;
    resourceCollection.setParentCollection(Collection::root());
    resourceCollection.setRemoteId(baseDirectoryPath());
    resourceCollection.setName(name());
    resourceCollection.setContentMimeTypes(mSupportedMimeTypes);
    resourceCollection.setRights(supportedRights(true));

    const QDir baseDir(baseDirectoryPath());

    Collection::List collections = createCollectionsForDirectory(baseDir, resourceCollection);
    collections.append(resourceCollection);

    collectionsRetrieved(collections);
}

void ContactsResource::retrieveItems(const Akonadi::Collection &collection)
{
    QDir directory(directoryForCollection(collection));
    if (!directory.exists()) {
        cancelTask(i18n("Directory '%1' does not exists", collection.remoteId()));
        return;
    }

    directory.setFilter(QDir::Files | QDir::Readable);

    Item::List items;

    const QFileInfoList entries = directory.entryInfoList();

    for (const QFileInfo &entry : entries) {
        if (entry.fileName() == QLatin1String("WARNING_README.txt")) {
            continue;
        }

        Item item;
        item.setRemoteId(entry.fileName());

        if (entry.fileName().endsWith(QLatin1String(".vcf"))) {
            item.setMimeType(KContacts::Addressee::mimeType());
        } else if (entry.fileName().endsWith(QLatin1String(".ctg"))) {
            item.setMimeType(KContacts::ContactGroup::mimeType());
        } else {
            cancelTask(i18n("Found file of unknown format: '%1'", entry.absoluteFilePath()));
            return;
        }

        items.append(item);
    }

    itemsRetrieved(items);
}

bool ContactsResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    const QString filePath = directoryForCollection(item.parentCollection()) + QDir::separator() + item.remoteId();

    Item newItem(item);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        cancelTask(i18n("Unable to open file '%1'", filePath));
        return false;
    }

    if (filePath.endsWith(QLatin1String(".vcf"))) {
        KContacts::VCardConverter converter;

        const QByteArray content = file.readAll();
        const KContacts::Addressee contact = converter.parseVCard(content);
        if (contact.isEmpty()) {
            cancelTask(i18n("Found invalid contact in file '%1'", filePath));
            return false;
        }

        newItem.setPayload<KContacts::Addressee>(contact);
    } else if (filePath.endsWith(QLatin1String(".ctg"))) {
        KContacts::ContactGroup group;
        QString errorMessage;

        if (!KContacts::ContactGroupTool::convertFromXml(&file, group, &errorMessage)) {
            cancelTask(i18n("Found invalid contact group in file '%1': %2", filePath, errorMessage));
            return false;
        }

        newItem.setPayload<KContacts::ContactGroup>(group);
    } else {
        cancelTask(i18n("Found file of unknown format: '%1'", filePath));
        return false;
    }

    file.close();

    itemRetrieved(newItem);

    return true;
}

void ContactsResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only directory: '%1'", collection.remoteId()));
        return;
    }

    const QString directoryPath = directoryForCollection(collection);

    Item newItem(item);

    if (item.hasPayload<KContacts::Addressee>()) {
        const KContacts::Addressee contact = item.payload<KContacts::Addressee>();

        const QString fileName = directoryPath + QDir::separator() + contact.uid() + QLatin1String(".vcf");

        KContacts::VCardConverter converter;
        const QByteArray content = converter.createVCard(contact);

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            cancelTask(i18n("Unable to write to file '%1': %2", fileName, file.errorString()));
            return;
        }

        file.write(content);
        file.close();

        newItem.setRemoteId(contact.uid() + QLatin1String(".vcf"));
    } else if (item.hasPayload<KContacts::ContactGroup>()) {
        const KContacts::ContactGroup group = item.payload<KContacts::ContactGroup>();

        const QString fileName = directoryPath + QDir::separator() + group.id() + QLatin1String(".ctg");

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            cancelTask(i18n("Unable to write to file '%1': %2", fileName, file.errorString()));
            return;
        }

        KContacts::ContactGroupTool::convertToXml(group, &file);

        file.close();

        newItem.setRemoteId(group.id() + QLatin1String(".ctg"));
    } else {
        qCWarning(CONTACTSRESOURCES_LOG) << "got item without (usable) payload, ignoring it";
    }

    changeCommitted(newItem);
}

void ContactsResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only file: '%1'", item.remoteId()));
        return;
    }

    Item newItem(item);

    const QString fileName = directoryForCollection(item.parentCollection()) + QDir::separator() + item.remoteId();

    if (item.hasPayload<KContacts::Addressee>()) {
        const KContacts::Addressee contact = item.payload<KContacts::Addressee>();

        KContacts::VCardConverter converter;
        const QByteArray content = converter.createVCard(contact);

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            cancelTask(i18n("Unable to write to file '%1': %2", fileName, file.errorString()));
            return;
        }
        file.write(content);
        file.close();

        newItem.setRemoteId(item.remoteId());
    } else if (item.hasPayload<KContacts::ContactGroup>()) {
        const KContacts::ContactGroup group = item.payload<KContacts::ContactGroup>();

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            cancelTask(i18n("Unable to write to file '%1': %2", fileName, file.errorString()));
            return;
        }

        KContacts::ContactGroupTool::convertToXml(group, &file);

        file.close();

        newItem.setRemoteId(item.remoteId());
    } else {
        cancelTask(i18n("Received item with unknown payload %1", item.mimeType()));
        return;
    }

    changeCommitted(newItem);
}

void ContactsResource::itemRemoved(const Akonadi::Item &item)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only file: '%1'", item.remoteId()));
        return;
    }

    // If the parent collection has no valid remote id, the parent
    // collection will be removed in a second, so stop here and remove
    // all items in collectionRemoved().
    if (item.parentCollection().remoteId().isEmpty()) {
        changeProcessed();
        return;
    }

    const QString fileName = directoryForCollection(item.parentCollection()) + QDir::separator() + item.remoteId();

    if (!QFile::remove(fileName)) {
        cancelTask(i18n("Unable to remove file '%1'", fileName));
        return;
    }

    changeProcessed();
}

void ContactsResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only directory: '%1'", parent.remoteId()));
        return;
    }

    const QString dirName = directoryForCollection(parent) + QDir::separator() + collection.name();

    if (!QDir::root().mkpath(dirName)) {
        cancelTask(i18n("Unable to create folder '%1'.", dirName));
        return;
    }

    initializeDirectory(dirName);

    Collection newCollection(collection);
    newCollection.setRemoteId(collection.name());
    changeCommitted(newCollection);
}

void ContactsResource::collectionChanged(const Akonadi::Collection &collection)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only directory: '%1'", collection.remoteId()));
        return;
    }

    if (collection.parentCollection() == Collection::root()) {
        if (collection.name() != name()) {
            setName(collection.name());
        }
        changeProcessed();
        return;
    }

    if (collection.remoteId() == collection.name()) {
        changeProcessed();
        return;
    }

    const QString dirName = directoryForCollection(collection);

    QFileInfo oldDirectory(dirName);
    if (!QDir::root().rename(dirName, oldDirectory.absolutePath() + QDir::separator() + collection.name())) {
        cancelTask(i18n("Unable to rename folder '%1'.", collection.name()));
        return;
    }

    Collection newCollection(collection);
    newCollection.setRemoteId(collection.name());
    changeCommitted(newCollection);
}

void ContactsResource::collectionRemoved(const Akonadi::Collection &collection)
{
    if (ContactsResourceSettings::self()->readOnly()) {
        cancelTask(i18n("Trying to write to a read-only directory: '%1'", collection.remoteId()));
        return;
    }

    QDir collectionDir = directoryForCollection(collection);
    if (!collectionDir.removeRecursively()) {
        cancelTask(i18n("Unable to delete folder '%1'.", collection.name()));
        return;
    }

    changeProcessed();
}

void ContactsResource::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    const QString sourceFileName = directoryForCollection(collectionSource) + QDir::separator() + item.remoteId();
    const QString targetFileName = directoryForCollection(collectionDestination) + QDir::separator() + item.remoteId();

    if (QFile::rename(sourceFileName, targetFileName)) {
        changeProcessed();
    } else {
        cancelTask(i18n("Unable to move file '%1' to '%2', '%2' already exists.", sourceFileName, targetFileName));
    }
}

void ContactsResource::collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    const QString sourceDirectoryName = directoryForCollection(collectionSource) + QDir::separator() + collection.remoteId();
    const QString targetDirectoryName = directoryForCollection(collectionDestination) + QDir::separator() + collection.remoteId();

    if (QFile::rename(sourceDirectoryName, targetDirectoryName)) {
        changeProcessed();
    } else {
        cancelTask(i18n("Unable to move directory '%1' to '%2', '%2' already exists.", sourceDirectoryName, targetDirectoryName));
    }
}

QString ContactsResource::baseDirectoryPath() const
{
    return ContactsResourceSettings::self()->path();
}

void ContactsResource::initializeDirectory(const QString &path) const
{
    QDir dir(path);

    // if folder does not exists, create it
    QDir::root().mkpath(dir.absolutePath());

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QDir::separator() + QLatin1String("WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write("Important Warning!!!\n\n"
                   "Don't create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

Collection::Rights ContactsResource::supportedRights(bool isResourceCollection) const
{
    Collection::Rights rights = Collection::ReadOnly;

    if (!ContactsResourceSettings::self()->readOnly()) {
        rights |= Collection::CanChangeItem;
        rights |= Collection::CanCreateItem;
        rights |= Collection::CanDeleteItem;
        rights |= Collection::CanCreateCollection;
        rights |= Collection::CanChangeCollection;

        if (!isResourceCollection) {
            rights |= Collection::CanDeleteCollection;
        }
    }

    return rights;
}

QString ContactsResource::directoryForCollection(const Collection &collection) const
{
    if (collection.remoteId().isEmpty()) {
        qCWarning(CONTACTSRESOURCES_LOG) << "Got incomplete ancestor chain:" << collection;
        return QString();
    }

    if (collection.parentCollection() == Collection::root()) {
        if (collection.remoteId() != baseDirectoryPath()) {
            qCWarning(CONTACTSRESOURCES_LOG) << "RID mismatch, is " << collection.remoteId()
                                             << " expected " << baseDirectoryPath();
        }
        return collection.remoteId();
    }

    const QString parentDirectory = directoryForCollection(collection.parentCollection());
    if (parentDirectory.isNull()) { // invalid, != isEmpty() here!
        return QString();
    }

    QString directory = parentDirectory;
    if (!directory.endsWith(QLatin1Char('/'))) {
        directory += QDir::separator() + collection.remoteId();
    } else {
        directory += collection.remoteId();
    }

    return directory;
}

AKONADI_RESOURCE_MAIN(ContactsResource)
