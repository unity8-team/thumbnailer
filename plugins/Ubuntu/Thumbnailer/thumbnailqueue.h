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

#ifndef THUMBNAILQUEUE_H
#define THUMBNAILQUEUE_H

#include <QtCore/QObject>
#include <QtCore/QThreadPool>

class ThumbnailTask;

class ThumbnailQueue : public QObject
{
    Q_OBJECT

public:
    void appendTask(ThumbnailTask* task);
    bool removeTask(ThumbnailTask* task);
    Q_SLOT void processNext();

private:
    QThreadPool m_threadPool;
    QList<ThumbnailTask*> m_queue;
};

#endif
