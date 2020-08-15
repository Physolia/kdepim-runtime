/*
    This file is part of oxaccess.

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef OXA_FOLDERMOVEJOB_H
#define OXA_FOLDERMOVEJOB_H

#include <KJob>

#include "folder.h"

namespace OXA {
/**
 * @short A job that moves a folder on the OX server.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class FolderMoveJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new folder move job.
     *
     * @param folder The folder to move.
     * @param destinationFolder The new parent folder.
     * @param parent The parent object.
     *
     * @note The folder needs the objectId, folderId and lastModified property set, the
     *       destinationFolder the objectId property.
     */
    FolderMoveJob(const Folder &folder, const Folder &destinationFolder, QObject *parent = nullptr);

    /**
     * Starts the job.
     */
    void start() override;

    /**
     * Returns the updated folder that has been moved.
     */
    Folder folder() const;

private Q_SLOTS:
    void davJobFinished(KJob *);

private:
    Folder mFolder;
    Folder mDestinationFolder;
};
}

#endif
