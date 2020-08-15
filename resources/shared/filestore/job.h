/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_FILESTORE_JOB_H
#define AKONADI_FILESTORE_JOB_H

#include "akonadi-filestore_export.h"

#include <KJob>

namespace Akonadi {
namespace FileStore {
class AbstractJobSession;
class CollectionCreateJob;
class CollectionDeleteJob;
class CollectionFetchJob;
class CollectionModifyJob;
class CollectionMoveJob;
class ItemCreateJob;
class ItemDeleteJob;
class ItemFetchJob;
class ItemModifyJob;
class ItemMoveJob;
class StoreCompactJob;

/**
 */
class AKONADI_FILESTORE_EXPORT Job : public KJob
{
    friend class AbstractJobSession;

    Q_OBJECT

public:
    class Visitor
    {
    public:
        virtual ~Visitor()
        {
        }

        virtual bool visit(Job *job) = 0;

        virtual bool visit(CollectionCreateJob *job) = 0;

        virtual bool visit(CollectionDeleteJob *job) = 0;

        virtual bool visit(CollectionFetchJob *job) = 0;

        virtual bool visit(CollectionModifyJob *job) = 0;

        virtual bool visit(CollectionMoveJob *job) = 0;

        virtual bool visit(ItemCreateJob *job) = 0;

        virtual bool visit(ItemDeleteJob *job) = 0;

        virtual bool visit(ItemFetchJob *job) = 0;

        virtual bool visit(ItemModifyJob *job) = 0;

        virtual bool visit(ItemMoveJob *job) = 0;

        virtual bool visit(StoreCompactJob *job) = 0;
    };

    enum ErrorCodes {
        InvalidStoreState = KJob::UserDefinedError + 1,
        InvalidJobContext
    };

    explicit Job(AbstractJobSession *session = nullptr);

    ~Job() override;

    void start() override;

    virtual bool accept(Visitor *visitor);

private:
    class Private;
    Private *d;
};
}
}

#endif
