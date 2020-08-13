/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_CONNECTIONTESTJOB_H
#define OXA_CONNECTIONTESTJOB_H

#include <KJob>

namespace OXA {
class ConnectionTestJob : public KJob
{
    Q_OBJECT

public:
    ConnectionTestJob(const QString &url, const QString &user, const QString &password, QObject *parent = nullptr);

    void start() override;

private Q_SLOTS:
    void httpJobFinished(KJob *);

private:
    QString mUrl;
    QString mUser;
    QString mPassword;
};
}

#endif
