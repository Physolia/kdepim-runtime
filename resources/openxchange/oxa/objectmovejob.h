/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KJob>

#include "folder.h"
#include "object.h"

namespace OXA
{
class ObjectMoveJob : public KJob
{
    Q_OBJECT

public:
    ObjectMoveJob(const Object &object, const Folder &destinationFolder, QObject *parent = nullptr);

    void start() override;

    Object object() const;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    Object mObject;
    Folder mDestinationFolder;
};
}
