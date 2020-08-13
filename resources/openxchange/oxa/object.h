/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_OBJECT_H
#define OXA_OBJECT_H

#include "folder.h"

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <KCalendarCore/Incidence>

#include <QVector>
#include <QString>

namespace OXA {
class Object
{
public:
    /**
     * Describes a list of objects.
     */
    typedef QVector<Object> List;

    /**
     * Describes the status of the object.
     */
    enum ObjectStatus {
        Created, ///< The object has been created or modified.
        Deleted  ///< The object has been deleted.
    };

    Object();

    void setObjectStatus(ObjectStatus status);
    ObjectStatus objectStatus() const;

    void setObjectId(qlonglong id);
    qlonglong objectId() const;

    void setFolderId(qlonglong id);
    qlonglong folderId() const;

    void setLastModified(const QString &timeStamp);
    QString lastModified() const;

    void setModule(Folder::Module module);
    Folder::Module module() const;

    void setContact(const KContacts::Addressee &contact);
    KContacts::Addressee contact() const;

    void setContactGroup(const KContacts::ContactGroup &group);
    KContacts::ContactGroup contactGroup() const;

    void setEvent(const KCalendarCore::Incidence::Ptr &event);
    KCalendarCore::Incidence::Ptr event() const;

    void setTask(const KCalendarCore::Incidence::Ptr &task);
    KCalendarCore::Incidence::Ptr task() const;

private:
    ObjectStatus mObjectStatus;
    qlonglong mObjectId;
    qlonglong mFolderId;
    QString mLastModified;
    Folder::Module mModule;
    KContacts::Addressee mContact;
    KContacts::ContactGroup mContactGroup;
    KCalendarCore::Incidence::Ptr mEvent;
    KCalendarCore::Incidence::Ptr mTask;
};
}

Q_DECLARE_TYPEINFO(OXA::Object, Q_MOVABLE_TYPE);

#endif
