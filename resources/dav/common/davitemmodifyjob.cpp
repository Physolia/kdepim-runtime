/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "davitemmodifyjob.h"

#include "davitemfetchjob.h"
#include "davmanager.h"

#include <kio/job.h>
#include <KLocalizedString>

DavItemModifyJob::DavItemModifyJob(const DavUtils::DavUrl &url, const DavItem &item, QObject *parent)
    : DavJobBase(parent), mUrl(url), mItem(item), mFreshResponseCode(0)
{
}

void DavItemModifyJob::start()
{
    QString headers = QLatin1String("Content-Type: ");
    headers += mItem.contentType();
    headers += QLatin1String("\r\n");
    headers += QLatin1String("If-Match: ") + mItem.etag();

    KIO::StoredTransferJob *job = KIO::storedPut(mItem.data(), mUrl.url(), -1, KIO::HideProgressInfo | KIO::DefaultFlags);
    job->addMetaData(QStringLiteral("PropagateHttpHeader"), QStringLiteral("true"));
    job->addMetaData(QStringLiteral("customHTTPHeader"), headers);
    job->addMetaData(QStringLiteral("cookies"), QStringLiteral("none"));
    job->addMetaData(QStringLiteral("no-auth-prompt"), QStringLiteral("true"));

    connect(job, &KIO::StoredTransferJob::result, this, &DavItemModifyJob::davJobFinished);
}

DavItem DavItemModifyJob::item() const
{
    return mItem;
}

DavItem DavItemModifyJob::freshItem() const
{
    return mFreshItem;
}

int DavItemModifyJob::freshResponseCode() const
{
    return mFreshResponseCode;
}

void DavItemModifyJob::davJobFinished(KJob *job)
{
    KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob *>(job);

    if (storedJob->error()) {
        const int responseCode = storedJob->queryMetaData(QLatin1String("responsecode")).isEmpty() ?
                                 0 :
                                 storedJob->queryMetaData(QLatin1String("responsecode")).toInt();

        QString err;
        if (storedJob->error() != KIO::ERR_SLAVE_DEFINED) {
            err = KIO::buildErrorString(storedJob->error(), storedJob->errorText());
        } else {
            err = storedJob->errorText();
        }

        setLatestResponseCode(responseCode);
        setError(UserDefinedError + responseCode);
        setErrorText(i18n("There was a problem with the request. The item was not modified on the server.\n"
                          "%1 (%2).", err, responseCode));

        if (hasConflict()) {
            DavItemFetchJob *fetchJob = new DavItemFetchJob(mUrl, mItem);
            connect(fetchJob, &DavItemFetchJob::result, this, &DavItemModifyJob::conflictingItemFetched);
            fetchJob->start();
        } else {
            emitResult();
        }

        return;
    }

    // The 'Location:' HTTP header is used to indicate the new URL
    const QStringList allHeaders = storedJob->queryMetaData(QLatin1String("HTTP-Headers")).split(QLatin1Char('\n'));
    QString location;
    foreach (const QString &header, allHeaders) {
        if (header.startsWith(QLatin1String("location:"), Qt::CaseInsensitive)) {
            location = header.section(QLatin1Char(' '), 1);
        }
    }

    KUrl url;
    if (location.isEmpty()) {
        url = storedJob->url();
    } else if (location.startsWith(QLatin1Char('/'))) {
        url = storedJob->url();
        url.setEncodedPath(location.toLatin1());
    } else {
        url = location;
    }

    url.setUser(QString());
    mItem.setUrl(url.prettyUrl());

    DavItemFetchJob *fetchJob = new DavItemFetchJob(mUrl, mItem);
    connect(fetchJob, &DavItemFetchJob::result, this, &DavItemModifyJob::itemRefreshed);
    fetchJob->start();
}

void DavItemModifyJob::itemRefreshed(KJob *job)
{
    if (!job->error()) {
        DavItemFetchJob *fetchJob = qobject_cast<DavItemFetchJob *>(job);
        mItem.setEtag(fetchJob->item().etag());
    } else {
        mItem.setEtag(QString());
    }
    emitResult();
}

void DavItemModifyJob::conflictingItemFetched(KJob *job)
{
    DavItemFetchJob *fetchJob = qobject_cast<DavItemFetchJob *>(job);
    mFreshResponseCode = fetchJob->latestResponseCode();

    if (!job->error()) {
        mFreshItem = fetchJob->item();
    }

    emitResult();
}

