/*
    This file is part of the kolab resource - the implementation of the
    Kolab storage format. See www.kolab.org for documentation on this.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KOLABV2_TASK_H
#define KOLABV2_TASK_H

#include "incidence.h"

#include <kcalendarcore/todo.h>
#include <kcalendarcore/incidence.h>

class QDomElement;

namespace KCal {
class ResourceKolab;
}

namespace KolabV2 {
/**
 * This class represents a task, and knows how to load/save it
 * from/to XML, and from/to a KCalendarCore::Todo.
 * The instances of this class are temporary, only used to convert
 * one to the other.
 */
class Task : public Incidence
{
public:
    /// Use this to parse an xml string to a task entry
    /// The caller is responsible for deleting the returned task
    static KCalendarCore::Todo::Ptr fromXml(const QDomDocument &xmlDoc, const QString &tz /*, KCalendarCore::ResourceKolab *res = 0,
                                const QString& subResource = QString(), quint32 sernum = 0 */);

    /// Use this to get an xml string describing this task entry
    static QString taskToXML(const KCalendarCore::Todo::Ptr &, const QString &tz);

    explicit Task( /*KCalendarCore::ResourceKolab *res, const QString& subResource, quint32 sernum,*/
        const QString &tz, const KCalendarCore::Todo::Ptr &todo = KCalendarCore::Todo::Ptr());
    ~Task() override;

    QString type() const override
    {
        return QStringLiteral("Task");
    }

    void saveTo(const KCalendarCore::Todo::Ptr &todo);

    virtual void setPercentCompleted(int percent);
    virtual int percentCompleted() const;

    virtual void setStatus(KCalendarCore::Incidence::Status status);
    virtual KCalendarCore::Incidence::Status status() const;

    virtual void setParent(const QString &parentUid);
    virtual QString parent() const;

    virtual void setHasStartDate(bool);
    virtual bool hasStartDate() const;

    virtual void setDueDate(const QDateTime &date);
    virtual void setDueDate(const QString &date);
    virtual void setDueDate(const QDate &date);
    virtual QDateTime dueDate() const;
    virtual bool hasDueDate() const;

    virtual void setCompletedDate(const QDateTime &date);
    virtual QDateTime completedDate() const;
    virtual bool hasCompletedDate() const;

    // Load the attributes of this class
    bool loadAttribute(QDomElement &) override;

    // Save the attributes of this class
    bool saveAttributes(QDomElement &) const override;

    // Load this task by reading the XML file
    bool loadXML(const QDomDocument &xml) override;

    // Serialize this task to an XML string
    QString saveXML() const override;

protected:
    // Read all known fields from this ical todo
    void setFields(const KCalendarCore::Todo::Ptr &);

    int mPercentCompleted;
    KCalendarCore::Incidence::Status mStatus;
    QString mParent;

    bool mHasStartDate;

    bool mHasDueDate;
    QDateTime mDueDate;

    bool mHasCompletedDate;
    QDateTime mCompletedDate;
};
}

#endif // KOLAB_TASK_H
