/*
 *  settingsdialog.h  -  Akonadi KAlarm directory resource configuration dialog
 *  Program:  kalarm
 *  Copyright © 2011 by David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#ifndef KALARMDIR_SETTINGSDIALOG_H
#define KALARMDIR_SETTINGSDIALOG_H

#include "ui_settingsdialog.h"
#include "ui_alarmtypewidget.h"

#include <kalarmcal/kacalendar.h>

#include <QDialog>
class QPushButton;
using namespace KAlarmCal;

class KConfigDialogManager;
class AlarmTypeWidget;

namespace Akonadi_KAlarm_Dir_Resource
{

class Settings;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(WId windowId, Settings *);
    void setAlarmTypes(CalEvent::Types);
    CalEvent::Types alarmTypes() const;

private Q_SLOTS:
    void save();
    void validate();
    void textChanged();
    void readOnlyClicked(bool);

private:
    Ui::SettingsDialog    ui;
    AlarmTypeWidget      *mTypeSelector = nullptr;
    QPushButton          *mOkButton = nullptr;
    KConfigDialogManager *mManager = nullptr;
    Akonadi_KAlarm_Dir_Resource::Settings *mSettings = nullptr;
    bool                  mReadOnlySelected = false;   // read-only was set by user (not by validate())
};

}

#endif // KALARMDIR_SETTINGSDIALOG_H

