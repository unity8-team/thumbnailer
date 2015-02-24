/*
 * Copyright 2015 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Florian Boucault <florian.boucault@canonical.com>
*/

#include "thumbnailqueue.h"
#include "qthumbnailer.h"

void ThumbnailQueue::appendTask(ThumbnailTask* task)
{
    connect(task, SIGNAL(thumbnailPathRetrieved(QString)), task->caller, SLOT(setThumbnail(QString)));
    connect(task, SIGNAL(thumbnailPathRetrieved(QString)), this, SLOT(processNext()));
    m_queue.append(task);
    processNext();
}

bool ThumbnailQueue::removeTask(ThumbnailTask* task)
{
    return m_queue.removeOne(task);
}

void ThumbnailQueue::processNext()
{
    if (m_queue.isEmpty()) {
        return;
    }

    ThumbnailTask* task = m_queue.takeFirst();
    if (!m_threadPool.tryStart(task)) {
        m_queue.prepend(task);
    }
}
