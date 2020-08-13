/*
   SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
   SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QDebug>
#include <QApplication>

#include "imapaccount.h"
#include "subscriptiondialog.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (app.arguments().size() < 5) {
        qWarning("Not enough parameters, expecting: <server> <port> <user> <password>");
        return 1;
    }

    QString server = app.arguments().at(1);
    int port = app.arguments().at(2).toInt();
    QString user = app.arguments().at(3);
    QString password = app.arguments().at(4);

    qDebug() << "Querying:" << server << port << user << password;
    qDebug();

    ImapAccount account;
    account.setServer(server);
    account.setPort(port);
    account.setUserName(user);

    SubscriptionDialog *dialog = new SubscriptionDialog();
    dialog->connectAccount(account, password);

    dialog->show();

    int retcode = app.exec();

    qDebug() << "Subscription changed?" << dialog->isSubscriptionChanged();

    return retcode;
}
