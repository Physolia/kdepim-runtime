/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewssyncfolderitemsrequest.h"

#include <memory>

#include <QXmlStreamWriter>

#include "ewsclient_debug.h"
#include "ewsxml.h"

enum SyncFolderItemsResponseElementType {
    SyncFolderItemsResponseElementInvalid = -1,
    SyncState,
    IncludesLastItemInRange,
    Changes,
};

enum SyncFolderItemsChangeElementType {
    SyncFolderITemsChangeElemnentInvalid = -1,
    Item,
    ItemId,
    IsRead,
};

class EwsSyncFolderItemsRequest::Response : public EwsRequest::Response
{
public:
    Response(QXmlStreamReader &reader);

    static bool changeReader(QXmlStreamReader &reader, QVariant &val);

    EwsSyncFolderItemsRequest::Change::List mChanges;
    bool mIncludesLastItem;
    QString mSyncState;
};

EwsSyncFolderItemsRequest::EwsSyncFolderItemsRequest(EwsClient &client, QObject *parent)
    : EwsRequest(client, parent)
    , mMaxChanges(100)
    , mIncludesLastItem(false)
{
    qRegisterMetaType<EwsSyncFolderItemsRequest::Change::List>();
    qRegisterMetaType<EwsItem>();
}

EwsSyncFolderItemsRequest::~EwsSyncFolderItemsRequest() = default;

void EwsSyncFolderItemsRequest::setFolderId(const EwsId &id)
{
    mFolderId = id;
}

void EwsSyncFolderItemsRequest::setItemShape(const EwsItemShape &shape)
{
    mShape = shape;
}

void EwsSyncFolderItemsRequest::setSyncState(const QString &state)
{
    mSyncState = state;
}

void EwsSyncFolderItemsRequest::setMaxChanges(uint max)
{
    mMaxChanges = max;
}

void EwsSyncFolderItemsRequest::start()
{
    QString reqString;
    QXmlStreamWriter writer(&reqString);

    startSoapDocument(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderItems"));

    mShape.write(writer);

    writer.writeStartElement(ewsMsgNsUri, QStringLiteral("SyncFolderId"));
    mFolderId.writeFolderIds(writer);
    writer.writeEndElement();

    if (!mSyncState.isNull()) {
        writer.writeTextElement(ewsMsgNsUri, QStringLiteral("SyncState"), mSyncState);
    }

    writer.writeTextElement(ewsMsgNsUri, QStringLiteral("MaxChangesReturned"), QString::number(mMaxChanges));

    writer.writeEndElement();

    endSoapDocument(writer);

    qCDebug(EWSCLI_PROTO_LOG) << reqString;

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        QString st = mSyncState.isNull() ? QStringLiteral("none") : ewsHash(mSyncState);
        qCDebugNCS(EWSCLI_REQUEST_LOG) << QStringLiteral("Starting SyncFolderItems request (folder: ") << mFolderId << QStringLiteral(", state: %1").arg(st);
    }

    prepare(reqString);

    doSend();
}

bool EwsSyncFolderItemsRequest::parseResult(QXmlStreamReader &reader)
{
    return parseResponseMessage(reader, QStringLiteral("SyncFolderItems"), [this](QXmlStreamReader &reader) {
        return parseItemsResponse(reader);
    });
}

bool EwsSyncFolderItemsRequest::parseItemsResponse(QXmlStreamReader &reader)
{
    QScopedPointer<EwsSyncFolderItemsRequest::Response> resp(new EwsSyncFolderItemsRequest::Response(reader));
    if (resp->responseClass() == EwsResponseUnknown) {
        return false;
    }

    mChanges = resp->mChanges;
    mIncludesLastItem = resp->mIncludesLastItem;
    mSyncState = resp->mSyncState;

    if (EWSCLI_REQUEST_LOG().isDebugEnabled()) {
        if (resp->isSuccess()) {
            qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got SyncFolderItems response (%1 changes, last included: %2, state: %3)")
                                                 .arg(mChanges.size())
                                                 .arg(mIncludesLastItem ? QStringLiteral("true") : QStringLiteral("false"))
                                                 .arg(qHash(mSyncState), 0, 36);
        } else {
            qCDebugNC(EWSCLI_REQUEST_LOG) << QStringLiteral("Got SyncFolderItems response - %1").arg(resp->responseMessage());
        }
    }

    return true;
}

EwsSyncFolderItemsRequest::Response::Response(QXmlStreamReader &reader)
    : EwsRequest::Response(reader)
{
    if (mClass == EwsResponseParseError) {
        return;
    }

    static const QList<EwsXml<SyncFolderItemsResponseElementType>::Item> items = {
        {SyncState, QStringLiteral("SyncState"), &ewsXmlTextReader},
        {IncludesLastItemInRange, QStringLiteral("IncludesLastItemInRange"), &ewsXmlBoolReader},
        {Changes, QStringLiteral("Changes"), &EwsSyncFolderItemsRequest::Response::changeReader},
    };
    static const EwsXml<SyncFolderItemsResponseElementType> staticReader(items);

    EwsXml<SyncFolderItemsResponseElementType> ewsReader(staticReader);

    if (!ewsReader.readItems(reader, ewsMsgNsUri, [this](QXmlStreamReader &reader, const QString &) {
            if (!readResponseElement(reader)) {
                setErrorMsg(QStringLiteral("Failed to read EWS request - invalid response element."));
                return false;
            }
            return true;
        })) {
        mClass = EwsResponseParseError;
        return;
    }

    QHash<SyncFolderItemsResponseElementType, QVariant> values = ewsReader.values();

    mSyncState = values[SyncState].toString();
    mIncludesLastItem = values[IncludesLastItemInRange].toBool();
    mChanges = values[Changes].value<Change::List>();
}

bool EwsSyncFolderItemsRequest::Response::changeReader(QXmlStreamReader &reader, QVariant &val)
{
    Change::List changes;
    QString elmName(reader.name().toString());

    while (reader.readNextStartElement()) {
        Change change(reader);
        if (!change.isValid()) {
            qCWarningNC(EWSCLI_LOG) << QStringLiteral("Failed to read %1 element").arg(elmName);
            return false;
        }
        changes.append(change);
    }

    val = QVariant::fromValue<Change::List>(changes);
    return true;
}

EwsSyncFolderItemsRequest::Change::Change(QXmlStreamReader &reader)
{
    static const QList<EwsXml<SyncFolderItemsChangeElementType>::Item> items = {
        {Item, QStringLiteral("Item"), &ewsXmlItemReader},
        {Item, QStringLiteral("Message"), &ewsXmlItemReader},
        {Item, QStringLiteral("CalendarItem"), &ewsXmlItemReader},
        {Item, QStringLiteral("Contact"), &ewsXmlItemReader},
        {Item, QStringLiteral("DistributionList"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingMessage"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingRequest"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingResponse"), &ewsXmlItemReader},
        {Item, QStringLiteral("MeetingCancellation"), &ewsXmlItemReader},
        {Item, QStringLiteral("Task"), &ewsXmlItemReader},
        {ItemId, QStringLiteral("ItemId"), &ewsXmlIdReader},
        {IsRead, QStringLiteral("IsRead"), &ewsXmlBoolReader},
    };
    static const EwsXml<SyncFolderItemsChangeElementType> staticReader(items);

    EwsXml<SyncFolderItemsChangeElementType> ewsReader(staticReader);

    if (reader.name() == QLatin1String("Create")) {
        mType = Create;
    } else if (reader.name() == QLatin1String("Update")) {
        mType = Update;
    } else if (reader.name() == QLatin1String("Delete")) {
        mType = Delete;
    } else if (reader.name() == QLatin1String("ReadFlagChange")) {
        mType = ReadFlagChange;
    }
    if (!ewsReader.readItems(reader, ewsTypeNsUri)) {
        return;
    }

    QHash<SyncFolderItemsChangeElementType, QVariant> values = ewsReader.values();

    switch (mType) {
    case Create:
    case Update:
        mItem = values[Item].value<EwsItem>();
        mId = mItem[EwsItemFieldItemId].value<EwsId>();
        break;
    case ReadFlagChange:
        mIsRead = values[IsRead].toBool();
        /* fall through */
        [[fallthrough]];
    case Delete:
        mId = values[ItemId].value<EwsId>();
        break;
    default:
        break;
    }
}

#include "moc_ewssyncfolderitemsrequest.cpp"
