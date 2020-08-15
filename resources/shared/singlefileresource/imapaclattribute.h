/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_IMAPACLATTRIBUTE_H
#define AKONADI_IMAPACLATTRIBUTE_H

#include "akonadi-singlefileresource_export.h"
#include <attribute.h>

#include <QMap>

#include <kimap/acl.h>

namespace Akonadi {
class AKONADI_SINGLEFILERESOURCE_EXPORT ImapAclAttribute : public Akonadi::Attribute
{
public:
    ImapAclAttribute();
    ImapAclAttribute(const QMap<QByteArray, KIMAP::Acl::Rights> &rights, const QMap<QByteArray, KIMAP::Acl::Rights> &oldRights);
    void setRights(const QMap<QByteArray, KIMAP::Acl::Rights> &rights);
    QMap<QByteArray, KIMAP::Acl::Rights> rights() const;
    QMap<QByteArray, KIMAP::Acl::Rights> oldRights() const;
    void setMyRights(KIMAP::Acl::Rights rights);
    KIMAP::Acl::Rights myRights() const;
    QByteArray type() const override;
    ImapAclAttribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

    bool operator==(const ImapAclAttribute &other) const;
private:
    QMap<QByteArray, KIMAP::Acl::Rights> mRights;
    QMap<QByteArray, KIMAP::Acl::Rights> mOldRights;
    KIMAP::Acl::Rights mMyRights;
};
}

#endif
