/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Poboiko <igor.poboiko@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef GOOGLESETTINGSDIALOG_H
#define GOOGLESETTINGSDIALOG_H

#include <QDialog>
#include <KGAPI/Types>

namespace Ui {
class GoogleSettingsDialog;
}
namespace KGAPI2 {
class Job;
}
class GoogleResource;
class GoogleSettings;

class GoogleSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GoogleSettingsDialog(GoogleResource *resource, GoogleSettings *settings, WId wId);
    ~GoogleSettingsDialog();
protected:
    bool handleError(KGAPI2::Job *job);
    void accountChanged();
private:
    GoogleResource *m_resource;
    GoogleSettings *m_settings;
    Ui::GoogleSettingsDialog *m_ui = nullptr;
    KGAPI2::AccountPtr m_account;
private Q_SLOTS:
    void slotConfigure();
    void slotAuthJobFinished(KGAPI2::Job *job);
    void slotSaveSettings();
    void slotReloadCalendars();
    void slotReloadTaskLists();
};

#endif // GOOGLESETTINGSDIALOG_H
