/*
    SPDX-FileCopyrightText: 2011-2013 Dan Vratil <dan@progdan.cz>
    SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "googlesettings.h"
#include "googleresource_debug.h"

#include <KGAPI/Account>

#ifdef __linux__
#include <QDBusMetaType>
#endif
#include <QDataStream>

#include <qt5keychain/keychain.h>
using namespace QKeychain;

using namespace KGAPI2;

static const QString googleWalletFolder = QStringLiteral("Akonadi Google");

#ifdef __linux__ // Use same serialization as KWallet to keep compatibility
typedef QMap<QString, QByteArray> StringByteArrayMap;
Q_DECLARE_METATYPE(StringByteArrayMap)
#endif

GoogleSettings::GoogleSettings()
    : m_winId(0)
{
#ifdef __linux__
    qDBusRegisterMetaType<StringByteArrayMap>();
#endif

    // First read from QtKeyChain
    auto job = new QKeychain::ReadPasswordJob(googleWalletFolder);
    connect(job, &QKeychain::Job::finished, this, [this, job]() {
        if (job->error() != QKeychain::Error::NoError) {
            Q_EMIT accountReady(false);
            return;
        }

        // Found something with QtKeyChain
        if (!account().isEmpty()) {
            m_account = fetchAccountFromKeychain(account(), job);
        }
        m_isReady = true;
        Q_EMIT accountReady(true);
    });
}

KGAPI2::AccountPtr GoogleSettings::fetchAccountFromKeychain(const QString &accountName, QKeychain::ReadPasswordJob *job)
{
    QMap<QString, QString> map;
    auto value = job->binaryData();
    if (!value.isEmpty()) {
        qCDebug(GOOGLE_LOG) << "Account" << accountName << "not found in KWallet";
        return AccountPtr();
    }

    QDataStream ds(&value, QIODevice::ReadOnly);
    ds >> map;

    const QStringList scopes = map[QStringLiteral("scopes")].split(QLatin1Char(','), Qt::SkipEmptyParts);
    QList<QUrl> scopeUrls;
    scopeUrls.reserve(scopes.count());
    for (const QString &scope : scopes) {
        scopeUrls << QUrl(scope);
    }
    AccountPtr account(new Account(accountName, map[QStringLiteral("accessToken")], map[QStringLiteral("refreshToken")], scopeUrls));
    return account;
}

bool GoogleSettings::storeAccount(AccountPtr account)
{
    // Removing the old one (if present)
    if (m_account && (account->accountName() != m_account->accountName())) {
        cleanup();
    }
    // Populating the new one
    m_account = account;

    QStringList scopes;
    const QList<QUrl> urlScopes = m_account->scopes();
    scopes.reserve(urlScopes.count());
    for (const QUrl &url : urlScopes) {
        scopes << url.toString();
    }

    const QMap<QString, QString> map = {{QStringLiteral("accessToken"), m_account->accessToken()},
                                        {QStringLiteral("refreshToken"), m_account->refreshToken()},
                                        {QStringLiteral("scopes"), scopes.join(QLatin1Char(','))}};

    // Legacy: store the map exactly like Kwallet is doing it
    QByteArray mapData;
    QDataStream ds(&mapData, QIODevice::WriteOnly);
    ds << map;

    auto writeJob = new WritePasswordJob(googleWalletFolder, this);
    writeJob->setKey(m_account->accountName());
    writeJob->setBinaryData(mapData);
    writeJob->start();
    SettingsBase::setAccount(m_account->accountName());
    m_isReady = true;
    return true;
}

void GoogleSettings::cleanup()
{
    if (m_account) {
        auto deleteJob = new DeletePasswordJob(googleWalletFolder, this);
        deleteJob->setKey(m_account->accountName());
        deleteJob->start();
    }
}

void GoogleSettings::addCalendar(const QString &calendar)
{
    if (calendars().isEmpty() || calendars().contains(calendar)) {
        return;
    }
    setCalendars(calendars() << calendar);
    save();
}

void GoogleSettings::addTaskList(const QString &taskList)
{
    if (calendars().isEmpty() || taskLists().contains(taskList)) {
        return;
    }
    setTaskLists(taskLists() << taskList);
    save();
}

QString GoogleSettings::clientId() const
{
    return QStringLiteral("554041944266.apps.googleusercontent.com");
}

QString GoogleSettings::clientSecret() const
{
    return QStringLiteral("mdT1DjzohxN3npUUzkENT0gO");
}

bool GoogleSettings::isReady() const
{
    return m_isReady;
}

AccountPtr GoogleSettings::accountPtr()
{
    return m_account;
}

void GoogleSettings::setWindowId(WId id)
{
    m_winId = id;
}

void GoogleSettings::setResourceId(const QString &resourceIdentificator)
{
    m_resourceId = resourceIdentificator;
}
