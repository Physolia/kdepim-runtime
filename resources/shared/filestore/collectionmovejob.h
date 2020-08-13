/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_COLLECTIONMOVEJOB_H
#define AKONADI_FILESTORE_COLLECTIONMOVEJOB_H

#include "job.h"

namespace Akonadi {
class Collection;

namespace FileStore {
class AbstractJobSession;

/**
 */
class AKONADI_FILESTORE_EXPORT CollectionMoveJob : public Job
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    explicit CollectionMoveJob(const Collection &collection, const Collection &targetParent, AbstractJobSession *session = nullptr);

    ~CollectionMoveJob() override;

    Collection collection() const;

    Collection targetParent() const;

    bool accept(Visitor *visitor) override;

private:
    void handleCollectionMoved(const Collection &collection);

private:
    class Private;
    Private *const d;
};
}
}

#endif
