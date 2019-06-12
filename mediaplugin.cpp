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

#include "mediaplugin.h"
#include "logging.h"

#include <QtIviCore/QIviSearchAndBrowseModel>
#include <QtIviMedia/QIviMediaPlayer>

#include <QCoreApplication>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>
#include <QtDebug>

#if defined(QT_GENIVIEXTRAS_LIB)
#include <QtGeniviExtras/QtDlt>
#endif

MopidyMediaPlugin::MopidyMediaPlugin(QObject *parent)
    : QObject(parent)
{
    connect(&m_mopidyClient,
            &mopidy::MopidyClient::clientConnected,
            this,
            &MopidyMediaPlugin::onClientConnected);
    connect(&m_mopidyClient,
            &mopidy::MopidyClient::clientDisconnected,
            this,
            &MopidyMediaPlugin::onClientDisconnected);
    connect(&m_mopidyClient,
            &mopidy::MopidyClient::connectionError,
            this,
            &MopidyMediaPlugin::onConnectionError);
    connect(m_mopidyClient.jsonrpcHandler().data(),
            &mopidy::JsonRpcHandler::error,
            this,
            &MopidyMediaPlugin::onJsonRpcError);

    QSharedPointer<mopidy::JsonRpcHandler> jsonRpcHandler = m_mopidyClient.jsonrpcHandler();

    QTimer::singleShot(m_timerMsec, this, &MopidyMediaPlugin::tryToConnect);

    m_player = new MediaPlayerBackend(jsonRpcHandler);
    m_indexer = new MediaIndexerBackend(jsonRpcHandler, m_player);

#if defined(QT_GENIVIEXTRAS_LIB)
    qInstallMessageHandler(QDltRegistration::messageHandler);
#endif
}

MopidyMediaPlugin::~MopidyMediaPlugin()
{
    m_mopidyClient.disconnect();
}

void MopidyMediaPlugin::tryToConnect()
{
    qCInfo(media) << "Connecting to Mopidy...";

    m_mopidyClient.connectToServer();
}

void MopidyMediaPlugin::onClientConnected()
{
    qCInfo(media) << "Connected to Mopidy";
}

void MopidyMediaPlugin::onClientDisconnected()
{
    qCInfo(media) << "Disconnected from Mopidy";
}

void MopidyMediaPlugin::onConnectionError(int errCode, const QString &message)
{
    qCWarning(media) << Q_FUNC_INFO << QString("%1 [%2]").arg(message).arg(errCode);

    QTimer::singleShot(m_timerMsec, this, &MopidyMediaPlugin::tryToConnect);
}

void MopidyMediaPlugin::onJsonRpcError(mopidy::JsonRpcHandler::RpcErrorCode errCode,
                                       const QJsonObject &jsonRpcErrorObj)
{
    qCWarning(media) << "JSON RPC Error: " << static_cast<int>(errCode) << jsonRpcErrorObj;
}

QStringList MopidyMediaPlugin::interfaces() const
{
    QStringList list;

    list << QStringLiteral(QIviMediaPlayer_iid);
    list << QStringLiteral(QIviMediaIndexer_iid);

    return list;
}

QIviFeatureInterface *MopidyMediaPlugin::interfaceInstance(const QString &interface) const
{
    if (interface == QStringLiteral(QIviMediaPlayer_iid))
        return m_player;
    else if (interface == QStringLiteral(QIviMediaIndexer_iid))
        return m_indexer;

    return nullptr;
}
