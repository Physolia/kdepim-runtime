/*
    SPDX-FileCopyrightText: 2015-2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewscreatemailjob.h"

#include <KLocalizedString>

#include <Akonadi/AgentManager>
#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <Akonadi/SpecialMailCollections>
#include <KMime/Message>

#include "ewscreateitemrequest.h"
#include "ewsmailhandler.h"
#include "ewsmoveitemrequest.h"
#include "ewspropertyfield.h"
#include "ewsresource_debug.h"

using namespace Akonadi;

static const EwsPropertyField propPidMessageFlags(0x0e07, EwsPropTypeInteger);

EwsCreateMailJob::EwsCreateMailJob(EwsClient &client,
                                   const Akonadi::Item &item,
                                   const Akonadi::Collection &collection,
                                   EwsTagStore *tagStore,
                                   EwsResource *parent)
    : EwsCreateItemJob(client, item, collection, tagStore, parent)
{
}

EwsCreateMailJob::~EwsCreateMailJob() = default;

void EwsCreateMailJob::doStart()
{
    if (!mItem.hasPayload<KMime::Message::Ptr>()) {
        setErrorMsg(QStringLiteral("Expected MIME message payload"));
        emitResult();
    }

    auto req = new EwsCreateItemRequest(mClient, this);

    auto msg = mItem.payload<KMime::Message::Ptr>();
    /* Exchange doesn't just store whatever MIME content that was sent to it - it will parse it and send
     * further the version assembled back from the parsed parts. It seems that this parsing doesn't work well
     * with the quoted-printable encoding, which KMail prefers. This results in malformed encoding, which the
     * sender doesn't even see.
     * As a workaround force encoding of the body (or in case of multipart - all parts) to Base64. */
    if (msg->contents().isEmpty()) {
        msg->changeEncoding(KMime::Headers::CEbase64);
        msg->contentTransferEncoding(true)->setEncoding(KMime::Headers::CEbase64);
    } else {
        const auto contents = msg->contents();
        for (KMime::Content *content : contents) {
            content->changeEncoding(KMime::Headers::CEbase64);
            content->contentTransferEncoding(true)->setEncoding(KMime::Headers::CEbase64);
        }
    }
    msg->assemble();
    QByteArray mimeContent = msg->encodedContent(true);
    bool sentItemsCreateWorkaround = false;
    EwsItem item;
    item.setType(EwsItemTypeMessage);
    item.setField(EwsItemFieldMimeContent, mimeContent);
    if (!mSend) {
        /* When creating items using the CreateItem request Exchange will by default mark the  message
         * as draft. Setting the extended property below causes the message to appear normally. */
        item.setProperty(propPidMessageFlags, QStringLiteral("1"));
        const Akonadi::AgentInstance &inst = Akonadi::AgentManager::self()->instance(mCollection.resource());

        /* WORKAROUND: The "Sent Items" folder is a little "special" when it comes to creating items.
         * Unlike other folders when creating items there the creation date/time is always set to the
         * current date/time instead of the value from the MIME Date header. This causes mail that
         * was copied from other folders to appear with the current date/time instead of the original one.
         * To work around this create the item in the "Drafts" folder first and then move it to "Sent Items". */
        if (mCollection == SpecialMailCollections::self()->collection(SpecialMailCollections::SentMail, inst)) {
            qCInfoNC(EWSRES_LOG) << "Move to \"Sent Items\" detected - activating workaround.";
            const Collection &draftColl = SpecialMailCollections::self()->collection(SpecialMailCollections::Drafts, inst);
            req->setSavedFolderId(EwsId(draftColl.remoteId(), draftColl.remoteRevision()));
            sentItemsCreateWorkaround = true;
        } else {
            req->setSavedFolderId(EwsId(mCollection.remoteId(), mCollection.remoteRevision()));
        }
    }
    // Set flags
    QHash<EwsPropertyField, QVariant> propertyHash = EwsMailHandler::writeFlags(mItem.flags());
    for (auto it = propertyHash.cbegin(), end = propertyHash.cend(); it != end; ++it) {
        if (!it.value().isNull()) {
            if (it.key().type() == EwsPropertyField::ExtendedField) {
                item.setProperty(it.key(), it.value());
            } else if (it.key().type() == EwsPropertyField::Field) {
                /* TODO: Currently EwsItem distinguishes between regular fields and extended fields
                 * and keeps them in separate lists. Someday it will make more sense to unify them.
                 * Until that the code below needs to manually translate the field names into
                 * EwsItemField enum items.
                 */
                if (it.key().uri() == QLatin1String("message:IsRead")) {
                    item.setField(EwsItemFieldIsRead, it.value());
                }
            }
        }
    }

    populateCommonProperties(item);

    req->setItems(EwsItem::List() << item);
    req->setMessageDisposition(mSend ? EwsDispSendOnly : EwsDispSaveOnly);
    connect(req,
            &EwsCreateItemRequest::finished,
            this,
            sentItemsCreateWorkaround ? &EwsCreateMailJob::mailCreateWorkaroundFinished : &EwsCreateMailJob::mailCreateFinished);
    addSubjob(req);
    req->start();
}

void EwsCreateMailJob::mailCreateFinished(KJob *job)
{
    auto req = qobject_cast<EwsCreateItemRequest *>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsCreateItemRequest job object"));
        emitResult();
        return;
    }

    if (req->responses().count() != 1) {
        setErrorMsg(QStringLiteral("Invalid number of responses received from server."));
        emitResult();
        return;
    }

    EwsCreateItemRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        EwsId id = resp.itemId();
        mItem.setRemoteId(id.id());
        mItem.setRemoteRevision(id.changeKey());
        mItem.setParentCollection(mCollection);
    } else {
        setErrorMsg(i18n("Failed to create mail item"));
    }

    emitResult();
}

void EwsCreateMailJob::mailCreateWorkaroundFinished(KJob *job)
{
    auto req = qobject_cast<EwsCreateItemRequest *>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsCreateItemRequest job object"));
        emitResult();
        return;
    }

    if (req->responses().count() != 1) {
        setErrorMsg(QStringLiteral("Invalid number of responses received from server."));
        emitResult();
        return;
    }

    EwsCreateItemRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        EwsId id = resp.itemId();
        auto moveItemReq = new EwsMoveItemRequest(mClient, this);
        moveItemReq->setItemIds(EwsId::List() << id);
        moveItemReq->setDestinationFolderId(EwsId(mCollection.remoteId()));
        connect(moveItemReq, &EwsCreateItemRequest::finished, this, &EwsCreateMailJob::mailMoveWorkaroundFinished);
        addSubjob(moveItemReq);
        moveItemReq->start();
    } else {
        setErrorMsg(i18n("Failed to create mail item"));
        emitResult();
    }
}

void EwsCreateMailJob::mailMoveWorkaroundFinished(KJob *job)
{
    auto req = qobject_cast<EwsMoveItemRequest *>(job);
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsMoveItemRequest job object"));
        emitResult();
        return;
    }

    if (req->responses().count() != 1) {
        setErrorMsg(QStringLiteral("Invalid number of responses received from server."));
        emitResult();
        return;
    }

    EwsMoveItemRequest::Response resp = req->responses().first();
    if (resp.isSuccess()) {
        EwsId id = resp.itemId();
        mItem.setRemoteId(id.id());
        mItem.setRemoteRevision(id.changeKey());
        mItem.setParentCollection(mCollection);
    } else {
        setErrorMsg(i18n("Failed to create mail item"));
    }

    emitResult();
}

bool EwsCreateMailJob::setSend(bool send)
{
    mSend = send;
    return true;
}

#include "moc_ewscreatemailjob.cpp"
