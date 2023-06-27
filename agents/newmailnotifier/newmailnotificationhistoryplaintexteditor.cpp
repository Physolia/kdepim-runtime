/*
    SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "newmailnotificationhistoryplaintexteditor.h"
#include <KStandardAction>
#include <QMenu>

NewMailNotificationHistoryPlainTextEditor::NewMailNotificationHistoryPlainTextEditor(QWidget *parent)
    : KPIMTextEdit::PlainTextEditor(parent)
{
}

void NewMailNotificationHistoryPlainTextEditor::addExtraMenuEntry(QMenu *menu, QPoint pos)
{
    Q_UNUSED(pos)
    QAction *clearAllAction = KStandardAction::clear(this, &NewMailNotificationHistoryPlainTextEditor::clearHistory, menu);
    menu->addAction(clearAllAction);
}

NewMailNotificationHistoryPlainTextEditor::~NewMailNotificationHistoryPlainTextEditor() = default;

#include "moc_newmailnotificationhistoryplaintexteditor.cpp"
