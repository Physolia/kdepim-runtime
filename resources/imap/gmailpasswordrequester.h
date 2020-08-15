/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GMAILPASSWORDREQUESTER_H_
#define GMAILPASSWORDREQUESTER_H_

#include "passwordrequesterinterface.h"

#include <QPointer>

class ImapResourceBase;

namespace KGAPI2 {
class AccountPromise;
}

class GmailPasswordRequester : public PasswordRequesterInterface
{
    Q_OBJECT
public:
    explicit GmailPasswordRequester(ImapResourceBase *resource, QObject *parent = nullptr);
    ~GmailPasswordRequester() override;

    void requestPassword(RequestType request, const QString &serverError) override;
    void cancelPasswordRequests() override;

private Q_SLOTS:
    void onTokenRequestFinished(KGAPI2::AccountPromise *promise);

private:
    ImapResourceBase *mResource = nullptr;
    QPointer<KGAPI2::AccountPromise> mPendingPromise;
};

#endif
