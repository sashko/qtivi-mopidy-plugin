/****************************************************************************
**
** Copyright (C) 2019 Luxoft Sweden AB
** Copyright (C) 2018 Pelagicore AG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtIvi module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-QTAS$
** Commercial License Usage
** Licensees holding valid commercial Qt Automotive Suite licenses may use
** this file in accordance with the commercial license agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and The Qt Company.  For
** licensing terms and conditions see https://www.qt.io/terms-conditions.
** For further information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
** SPDX-License-Identifier: LGPL-3.0
**
****************************************************************************/

#include "mediaindexerbackend.h"
#include "logging.h"

#include "mopidy/libraryhelper.h"

#include <QtConcurrent/QtConcurrent>

#include <QDirIterator>
#include <QImage>
#include <QStandardPaths>
#include <QThreadPool>
#include <QtDebug>

MediaIndexerBackend::MediaIndexerBackend(QSharedPointer<mopidy::JsonRpcHandler> jsonRpcHandler,
                                         QObject *parent)
    : QIviMediaIndexerControlBackendInterface(parent)
    , m_state(QIviMediaIndexerControl::Idle)
    , m_threadPool(new QThreadPool(this))
{
    m_libraryHelper.setJsonRpcHandler(jsonRpcHandler);

    connect(&m_libraryHelper,
            &mopidy::LibraryHelper::tracksInDirectoryFetched,
            this,
            &MediaIndexerBackend::onLibraryHelperTracksInDirectoryFetched);

    m_libraryHelper.requestTracksInDirectory("file:///root/Music/");
}

void MediaIndexerBackend::initialize()
{
    emit stateChanged(m_state);
    emit initializationDone();
}

void MediaIndexerBackend::pause()
{
    static const QLatin1String error("SIMULATION: Pausing the indexing is not supported");
    qCWarning(media) << error;
    emit errorChanged(QIviAbstractFeature::InvalidOperation, error);
}

void MediaIndexerBackend::resume()
{
    static const QLatin1String error("SIMULATION: Resuming the indexing is not supported");
    qCWarning(media) << error;
    emit errorChanged(QIviAbstractFeature::InvalidOperation, error);
}

void MediaIndexerBackend::addMediaFolder(const QString &path)
{
    ScanData data;
    data.remove = false;
    data.folder = path;
    m_folderQueue.append(data);

    scanNext();
}

void MediaIndexerBackend::removeMediaFolder(const QString &path)
{
    ScanData data;
    data.remove = true;
    data.folder = path;
    m_folderQueue.append(data);

    scanNext();
}

bool MediaIndexerBackend::scanWorker(const QString &mediaDir, bool removeData)
{
    qDebug() << "scanWorker";
}

void MediaIndexerBackend::onScanFinished()
{
    if (!m_folderQueue.isEmpty()) {
        scanNext();
        return;
    }

    qCInfo(media) << "Scanning done";
    emit progressChanged(1);
    emit indexingDone();

    //If the last run didn't succeed we will stay in the Error state
    if (m_watcher.future().result())
        setState(QIviMediaIndexerControl::Idle);
}

void MediaIndexerBackend::scanNext()
{
    if (m_watcher.isRunning())
        return;

    ScanData data = m_folderQueue.dequeue();
    m_currentFolder = data.folder;
    m_watcher.setFuture(
        QtConcurrent::run(this, &MediaIndexerBackend::scanWorker, m_currentFolder, data.remove));
}

void MediaIndexerBackend::setState(QIviMediaIndexerControl::State state)
{
    m_state = state;
    emit stateChanged(state);
}

void MediaIndexerBackend::onLibraryHelperTracksInDirectoryFetched(const QString &uri,
                                                                  const mopidy::Refs &refs)
{
    qDebug() << "onLibraryHelperTracksInDirectoryFetched";

    QStringList uris;
    int currentFileIndex = 0;
    int totalFileCount = uri.length();

    m_tracklistController.clear();

    for (auto ref : refs) {
        qDebug() << ref.uri;
        uris.append(ref.uri);
    }

    emit progressChanged(qreal(++currentFileIndex) / qreal(totalFileCount));
    m_tracklistController.add(uris, 0);
}
