/*
 * SPDX-FileCopyrightText: 2012 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "kolabconversion.h"
#include "commonconversion.h"
#include <akonadi/notes/noteutils.h>

namespace Kolab {
namespace Conversion {
Note fromNote(const KMime::Message::Ptr &m)
{
    Akonadi::NoteUtils::NoteMessageWrapper note(m);
    Note n;
    n.setSummary(toStdString(note.title()));
    n.setDescription(toStdString(note.text()));
    QDateTime created = QDateTime(note.creationDate());
    created.setTimeSpec(Qt::UTC);
    n.setCreated(fromDate(created, false));

    n.setUid(toStdString(note.uid()));
    QDateTime lastModified = note.lastModifiedDate();
    lastModified.setTimeSpec(Qt::UTC);
    n.setLastModified(fromDate(lastModified, false));

    switch (note.classification()) {
    case Akonadi::NoteUtils::NoteMessageWrapper::Confidential:
        n.setClassification(Kolab::ClassConfidential);
        break;
    case Akonadi::NoteUtils::NoteMessageWrapper::Private:
        n.setClassification(Kolab::ClassPrivate);
        break;
    default:
        n.setClassification(Kolab::ClassPublic);
    }

    std::vector<Kolab::CustomProperty> customs;
    QMap<QString, QString> &customsMap = note.custom();
    for (QMap <QString, QString >::const_iterator it = customsMap.constBegin(), end = customsMap.constEnd(); it != end; ++it) {
        customs.push_back(Kolab::CustomProperty(toStdString(it.key()), toStdString(it.value())));
    }
    n.setCustomProperties(customs);

    std::vector<Kolab::Attachment> attachments;
    attachments.reserve(note.attachments().count());
    foreach (const Akonadi::NoteUtils::Attachment &a, note.attachments()) {
        Kolab::Attachment attachment;
        if (a.url().isValid()) {
            attachment.setUri(toStdString(a.url().toString()), toStdString(a.mimetype()));
        } else {
            attachment.setData(toStdString(QString::fromUtf8(a.data())), toStdString(a.mimetype()));
        }
        attachment.setLabel(toStdString(a.label()));
        attachments.push_back(attachment);
    }
    n.setAttachments(attachments);

    return n;
}

KMime::Message::Ptr toNote(const Note &n)
{
    Akonadi::NoteUtils::NoteMessageWrapper note;
    note.setTitle(fromStdString(n.summary()));
    note.setText(fromStdString(n.description()));
    note.setFrom(QStringLiteral("kolab@kde4"));
    note.setCreationDate(toDate(n.created()));
    note.setUid(fromStdString(n.uid()));
    note.setLastModifiedDate(toDate(n.lastModified()));
    switch (n.classification()) {
    case Kolab::ClassPrivate:
        note.setClassification(Akonadi::NoteUtils::NoteMessageWrapper::Private);
        break;
    case Kolab::ClassConfidential:
        note.setClassification(Akonadi::NoteUtils::NoteMessageWrapper::Confidential);
        break;
    default:
        note.setClassification(Akonadi::NoteUtils::NoteMessageWrapper::Public);
    }

    foreach (const Kolab::Attachment &a, n.attachments()) {
        if (!a.uri().empty()) {
            Akonadi::NoteUtils::Attachment attachment(QUrl(fromStdString(a.uri())), fromStdString(a.mimetype()));
            attachment.setLabel(fromStdString(a.label()));
            note.attachments().append(attachment);
        } else {
            Akonadi::NoteUtils::Attachment attachment(fromStdString(a.data()).toLatin1(), fromStdString(a.mimetype()));
            attachment.setLabel(fromStdString(a.label()));
            note.attachments().append(attachment);
        }
    }
    foreach (const Kolab::CustomProperty &a, n.customProperties()) {
        note.custom().insert(fromStdString(a.identifier), fromStdString(a.value));
    }
    return note.message();
}
}
}
