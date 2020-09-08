/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etesyncclientstate.h"

#include <KLocalizedString>

#include "etesync_debug.h"

void EteSyncClientState::init()
{
    // Load settings
    Settings::self()->load();
    mServerUrl = Settings::self()->baseUrl();
    mUsername = Settings::self()->username();
    mPassword = Settings::self()->password();
    mEncryptionPassword = Settings::self()->encryptionPassword();

    if (mServerUrl.isEmpty() || mUsername.isEmpty() || mPassword.isEmpty() || mEncryptionPassword.isEmpty()) {
        Q_EMIT clientInitialised(false);
        return;
    }

    // Initialise EteSync client state
    mClient = etesync_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCWarning(ETESYNC_LOG) << "Unable to obtain token from server" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        Q_EMIT clientInitialised(false);
        return;
    }
    etesync_set_auth_token(mClient.get(), mToken);
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);

    // Get user keypair
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = etesync_user_info_manager_fetch(userInfoManager.get(), mUsername);
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "init() - User info obtained from server is NULL";
        invalidateToken();
        Q_EMIT clientInitialised(false);
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);

    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));

    Q_EMIT clientInitialised(true);
}

bool EteSyncClientState::initToken(const QString &serverUrl, const QString &username, const QString &password)
{
    mServerUrl = serverUrl;
    mUsername = username;
    mPassword = password;

    // Initialise EteSync client state
    mClient = etesync_new(QStringLiteral("Akonadi EteSync Resource"), mServerUrl);
    if (!mClient) {
        qCDebug(ETESYNC_LOG) << "Could not initialize EteSync client";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    mToken = etesync_auth_get_token(mClient.get(), mUsername, mPassword);
    if (mToken.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Empty token";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received token" << mToken;
    etesync_set_auth_token(mClient.get(), mToken);
    return true;
}

void EteSyncClientState::refreshToken()
{
    qCDebug(ETESYNC_LOG) << "Refreshing token";
    if (initToken(mServerUrl, mUsername, mPassword)) {
        tokenRefreshed(true);
        return;
    }
    tokenRefreshed(false);
    return;
}

void EteSyncClientState::refreshUserInfo()
{
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = etesync_user_info_manager_fetch(userInfoManager.get(), mUsername);
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "refreshUserInfo() - User info obtained from server is NULL";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return;
    }
    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
}

bool EteSyncClientState::initUserInfo()
{
    mJournalManager = EteSyncJournalManagerPtr(etesync_journal_manager_new(mClient.get()));
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    mUserInfo = etesync_user_info_manager_fetch(userInfoManager.get(), mUsername);
    if (!mUserInfo) {
        qCWarning(ETESYNC_LOG) << "initUserInfo() - User info obtained from server is NULL";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    return true;
}

void EteSyncClientState::invalidateToken()
{
    etesync_auth_invalidate_token(mClient.get(), mToken);
}

bool EteSyncClientState::initKeypair(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);

    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_user_info_get_keypair(mUserInfo.get(), userInfoCryptoManager.get()));
    if (!mKeypair) {
        qCDebug(ETESYNC_LOG) << "Empty keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    qCDebug(ETESYNC_LOG) << "Received keypair";
    return true;
}

bool EteSyncClientState::initAccount(const QString &encryptionPassword)
{
    mEncryptionPassword = encryptionPassword;
    mDerived = etesync_crypto_derive_key(mClient.get(), mUsername, mEncryptionPassword);
    if (mDerived.isEmpty()) {
        qCDebug(ETESYNC_LOG) << "Could not derive key from encryption password";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    mUserInfo = etesync_user_info_new(mUsername, ETESYNC_CURRENT_VERSION);
    mKeypair = EteSyncAsymmetricKeyPairPtr(etesync_crypto_generate_keypair(mClient.get()));
    EteSyncCryptoManagerPtr userInfoCryptoManager = etesync_user_info_get_crypto_manager(mUserInfo.get(), mDerived);
    if (etesync_user_info_set_keypair(mUserInfo.get(), userInfoCryptoManager.get(), mKeypair.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create set user info keypair";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    EteSyncUserInfoManagerPtr userInfoManager(etesync_user_info_manager_new(mClient.get()));
    if (etesync_user_info_manager_create(userInfoManager.get(), mUserInfo.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create user info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }

    const auto calendarResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR), i18n("My Calendar"));
    const auto contactsResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK), i18n("My Contacts"));
    const auto tasksResult = createDefaultJournal(QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS), i18n("My Tasks"));

    return calendarResult && contactsResult && tasksResult;
}

bool EteSyncClientState::createDefaultJournal(const QString &journalType, const QString &journalName)
{
    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_gen_uid()));
    EteSyncJournalPtr journal = etesync_journal_new(journalUid, ETESYNC_CURRENT_VERSION);

    EteSyncCollectionInfoPtr info = etesync_collection_info_new(journalType, journalName, QString(), ETESYNC_COLLECTION_DEFAULT_COLOR);

    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal.get(), mDerived, mKeypair.get());

    if (etesync_journal_set_info(journal.get(), cryptoManager.get(), info.get())) {
        qCDebug(ETESYNC_LOG) << "Could not set journal info";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }

    if (etesync_journal_manager_create(mJournalManager.get(), journal.get())) {
        qCDebug(ETESYNC_LOG) << "Could not create journal";
        qCDebug(ETESYNC_LOG) << "EteSync error" << QStringFromCharPtr(CharPtr(etesync_get_error_message()));
        return false;
    }
    return true;
}

void EteSyncClientState::saveSettings()
{
    Settings::self()->setBaseUrl(mServerUrl);
    Settings::self()->setUsername(mUsername);
    Settings::self()->setPassword(mPassword);
    Settings::self()->setEncryptionPassword(mEncryptionPassword);
    Settings::self()->save();
}
