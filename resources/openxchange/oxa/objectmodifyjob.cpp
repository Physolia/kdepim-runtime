/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "objectmodifyjob.h"

#include "davmanager.h"
#include "davutils.h"
#include "objectutils.h"
#include "oxutils.h"

#include <KIO/DavJob>
#include <QDomDocument>

using namespace OXA;

ObjectModifyJob::ObjectModifyJob(const Object &object, QObject *parent)
    : KJob(parent)
    , mObject(object)
{
}

void ObjectModifyJob::start()
{
    if (ObjectUtils::needsPreloading(mObject)) {
        KJob *job = ObjectUtils::preloadJob(mObject);
        connect(job, &KJob::result, this, &ObjectModifyJob::preloadingJobFinished);
        job->start();
    } else {
        QDomDocument document;
        QDomElement propertyupdate = DAVUtils::addDavElement(document, document, QStringLiteral("propertyupdate"));
        QDomElement set = DAVUtils::addDavElement(document, propertyupdate, QStringLiteral("set"));
        QDomElement prop = DAVUtils::addDavElement(document, set, QStringLiteral("prop"));

        ObjectUtils::addObjectElements(document, prop, mObject);

        const QString path = ObjectUtils::davPath(mObject.module());

        KIO::DavJob *job = DavManager::self()->createPatchJob(path, document);
        connect(job, &KJob::result, this, &ObjectModifyJob::davJobFinished);
    }
}

Object ObjectModifyJob::object() const
{
    return mObject;
}

void ObjectModifyJob::preloadingJobFinished(KJob *job)
{
    void *preloadedData = ObjectUtils::preloadData(mObject, job);

    QDomDocument document;
    QDomElement propertyupdate = DAVUtils::addDavElement(document, document, QStringLiteral("propertyupdate"));
    QDomElement set = DAVUtils::addDavElement(document, propertyupdate, QStringLiteral("set"));
    QDomElement prop = DAVUtils::addDavElement(document, set, QStringLiteral("prop"));

    ObjectUtils::addObjectElements(document, prop, mObject, preloadedData);

    const QString path = ObjectUtils::davPath(mObject.module());

    KIO::DavJob *davJob = DavManager::self()->createPatchJob(path, document);
    connect(davJob, &KIO::DavJob::result, this, &ObjectModifyJob::davJobFinished);
}

void ObjectModifyJob::davJobFinished(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    auto davJob = qobject_cast<KIO::DavJob *>(job);

    const QByteArray ba = davJob->responseData();
    QDomDocument document;
    document.setContent(ba);

    QString errorText, errorStatus;
    if (DAVUtils::davErrorOccurred(document, errorText, errorStatus)) {
        setError(UserDefinedError);
        setErrorText(errorText);
        emitResult();
        return;
    }

    QDomElement multistatus = document.documentElement();
    QDomElement response = multistatus.firstChildElement(QStringLiteral("response"));
    const QDomNodeList props = response.elementsByTagName(QStringLiteral("prop"));
    const QDomElement prop = props.at(0).toElement();

    QDomElement element = prop.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("last_modified")) {
            mObject.setLastModified(OXUtils::readString(element.text()));
        }

        element = element.nextSiblingElement();
    }

    emitResult();
}

#include "moc_objectmodifyjob.cpp"
