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

#include "mediaplayerbackend.h"
#include "logging.h"

#include <QDebug>
#include <QImage>

#ifndef QTIVI_NO_TAGLIB
#include <attachedpictureframe.h>
#include <fileref.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <tag.h>
#include <taglib.h>
#include <tstring.h>
#endif // QTIVI_NO_TAGLIB

MediaPlayerBackend::MediaPlayerBackend(QSharedPointer<mopidy::JsonRpcHandler> jsonRpcHandler,
                                       QObject *parent)
    : QIviMediaPlayerBackendInterface(parent)
{
    m_libraryHelper.setJsonRpcHandler(jsonRpcHandler);
    m_eventHandler.setJsonRpcHandler(jsonRpcHandler);
    m_mixerController.setJsonRpcHandler(jsonRpcHandler);
    m_playbackController.setJsonRpcHandler(jsonRpcHandler);
    m_tracklistController.setJsonRpcHandler(jsonRpcHandler);

    m_positionTrackerTimer.setInterval(500);

    connect(&m_positionTrackerTimer, &QTimer::timeout, [this]() {
        m_playbackController.getTimePosition();
    });

    // playback
    connect(&m_playbackController,
            &mopidy::PlaybackController::currentTlTrackReceived,
            this,
            &MediaPlayerBackend::onCurrentTlTrackReceived);
    connect(&m_playbackController,
            &mopidy::PlaybackController::stateReceived,
            this,
            &MediaPlayerBackend::onStateReceived);
    connect(&m_eventHandler,
            &mopidy::EventHandler::playbackStateChanged,
            this,
            &MediaPlayerBackend::onPlaybackStateChanged);
    connect(&m_eventHandler, &mopidy::EventHandler::seeked, this, &MediaPlayerBackend::onSeeked);
    connect(&m_eventHandler,
            &mopidy::EventHandler::trackPlaybackStarted,
            this,
            &MediaPlayerBackend::onTrackPlaybackStarted);
    connect(&m_eventHandler,
            &mopidy::EventHandler::tracklistChanged,
            this,
            &MediaPlayerBackend::onTracklistChanged);
    connect(&m_playbackController,
            &mopidy::PlaybackController::timePositionReceived,
            this,
            &MediaPlayerBackend::onTimePositionReceived);

    // tracklist
    connect(&m_tracklistController,
            &mopidy::TracklistController::indexReceived,
            this,
            &MediaPlayerBackend::onCurrentIndexReceived);
    connect(&m_tracklistController,
            &mopidy::TracklistController::tracksReceived,
            this,
            &MediaPlayerBackend::onTracksReceived);
    connect(&m_tracklistController,
            &mopidy::TracklistController::singleReceived,
            this,
            &MediaPlayerBackend::onSingleReceived);
    connect(&m_tracklistController,
            &mopidy::TracklistController::repeatReceived,
            this,
            &MediaPlayerBackend::onRepeatReceived);
    connect(&m_tracklistController,
            &mopidy::TracklistController::randomReceived,
            this,
            &MediaPlayerBackend::onRandomReceived);

    // mixer
    connect(&m_mixerController,
            &mopidy::MixerController::muteReceived,
            this,
            &MediaPlayerBackend::onMuteReceived);
    connect(&m_mixerController,
            &mopidy::MixerController::volumeReceived,
            this,
            &MediaPlayerBackend::onVolumeReceived);

    connect(&m_libraryHelper,
            &mopidy::LibraryHelper::tracksInDirectoryFetched,
            this,
            &MediaPlayerBackend::onLibraryHelperTracksInDirectoryFetched);

    m_tracklistController.index();

    m_libraryHelper.requestTracksInDirectory(MEDIA_FOLDER);
}

void MediaPlayerBackend::initialize()
{
    qDebug() << "initialize";
    emit initializationDone();
}

bool MediaPlayerBackend::canReportCount()
{
    return true;
}

void MediaPlayerBackend::fetchData(int start, int count)
{
    qDebug() << "fetchData: "
             << "from: " << start << ", count: " << count;
}

void MediaPlayerBackend::insert(int index, const QIviPlayableItem *item)
{
    qDebug() << "insert: " << index;
}

void MediaPlayerBackend::remove(int index)
{
    qDebug() << "remove: " << index;
}

void MediaPlayerBackend::move(int curIndex, int newIndex)
{
    qDebug() << "move: "
             << "current: " << curIndex << ", new: " << newIndex;
    m_tracklistController.move(curIndex, curIndex, newIndex);
}

void MediaPlayerBackend::next()
{
    qDebug() << "next";
    m_playbackController.next();
    m_tracklistController.index();
}

void MediaPlayerBackend::pause()
{
    qDebug() << "pause";
    m_playbackController.pause();
}

void MediaPlayerBackend::play()
{
    qDebug() << "play";
    m_playbackController.play();
    m_tracklistController.index();
}

void MediaPlayerBackend::previous()
{
    qDebug() << "previous";
    m_playbackController.previous();
    m_tracklistController.index();
}

void MediaPlayerBackend::seek(qint64 offset)
{
    qDebug() << "seek: " << offset;
    connect(&m_playbackController,
            &mopidy::PlaybackController::timePositionReceived,
            [this, offset](int currentPosition) {
                m_playbackController.seek(currentPosition + offset);
            });
}

// TODO doesn't work
// prbly coz TLID is unique for mopidy
void MediaPlayerBackend::setCurrentIndex(int index)
{
    qDebug() << "setCurrentIndex: " << index;
    //    m_tracklistController.index(index);
}

void MediaPlayerBackend::setMuted(bool muted)
{
    qDebug() << "setMuted: " << muted;
    m_mixerController.setMute(muted);
}

void MediaPlayerBackend::setPlayMode(QIviMediaPlayer::PlayMode playMode)
{
    switch (playMode) {
    case QIviMediaPlayer::Normal:
        qDebug() << "setPlayMode: Normal";
        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(false);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::RepeatTrack:
        qDebug() << "setPlayMode: RepeatTrack";
        m_tracklistController.setSingle(true);
        m_tracklistController.setRepeat(true);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::RepeatAll:
        qDebug() << "setPlayMode: RepeatAll";
        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(true);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::Shuffle:
        qDebug() << "setPlayMode: Shuffle";
        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(false);
        m_tracklistController.setRandom(true);
        break;

    default:
        qWarning() << "setPlayMode: unsupported mode";
        break;
    };
}

void MediaPlayerBackend::setPosition(qint64 position)
{
    qDebug() << "setPosition: " << position;
    m_playbackController.seek(position);
}

void MediaPlayerBackend::setVolume(int volume)
{
    qDebug() << "setVolume: " << volume;
    m_mixerController.setVolume(volume);
}

void MediaPlayerBackend::stop()
{
    qDebug() << "stop";
    m_playbackController.stop();
}

/*
 * Mopidy playback calls
*/

void MediaPlayerBackend::onCurrentTlTrackReceived(const mopidy::TlTrack &tlTrack)
{
    qDebug() << "onCurrentTlTrackReceived";
    m_tracklistController.index();
}

void MediaPlayerBackend::onStateReceived(mopidy::PlaybackState state)
{
    switch (state) {
    case mopidy::PlaybackState::Paused:
        qDebug() << "onStateReceived: Paused";
        m_playbackState = QIviMediaPlayer::Paused;
        m_playbackController.getCurrentTlTrack();
        m_positionTrackerTimer.stop();
        break;

    case mopidy::PlaybackState::Playing:
        qDebug() << "onStateReceived: Playing";
        m_playbackState = QIviMediaPlayer::Playing;
        m_playbackController.getCurrentTlTrack();
        m_playbackController.getTimePosition();
        break;

    case mopidy::PlaybackState::Stopped:
        qDebug() << "onStateReceived: Stopped";
        m_playbackState = QIviMediaPlayer::Stopped;
        emit positionChanged(0);
        m_positionTrackerTimer.stop();
        break;

    default:
        qWarning() << "onStateReceived: unsupported state";
        break;
    }

    emit playStateChanged(m_playbackState);
}

void MediaPlayerBackend::onPlaybackStateChanged(mopidy::PlaybackState oldState,
                                                mopidy::PlaybackState newState)
{
    switch (newState) {
    case mopidy::PlaybackState::Paused:
        qDebug() << "onPlaybackStateChanged: Paused";
        m_playbackState = QIviMediaPlayer::Paused;
        m_playbackController.getCurrentTlTrack();
        break;

    case mopidy::PlaybackState::Playing:
        qDebug() << "onPlaybackStateChanged: Playing";
        m_playbackState = QIviMediaPlayer::Playing;
        m_playbackController.getCurrentTlTrack();
        m_playbackController.getTimePosition();
        break;

    case mopidy::PlaybackState::Stopped:
        qDebug() << "onPlaybackStateChanged: Stopped";
        m_playbackState = QIviMediaPlayer::Stopped;
        emit positionChanged(0);
        break;

    default:
        qWarning() << "onPlaybackStateChanged: unsupported new state";
        break;
    }

    emit playStateChanged(m_playbackState);
}

void MediaPlayerBackend::onSeeked(int timePosition)
{
    qDebug() << "onSeeked: " << timePosition;
    emit positionChanged(timePosition);
}

void MediaPlayerBackend::onTrackPlaybackStarted(const mopidy::TlTrack &tlTrack)
{
    qDebug() << "onEhTrackPlaybackStarted";
    m_tracklistController.index();
}

void MediaPlayerBackend::onTracklistChanged()
{
    qDebug() << "onTracklistChanged";
}

void MediaPlayerBackend::onTimePositionReceived(int timePosition)
{
    qDebug() << "onTimePositionReceived: " << timePosition;

    if (!m_positionTrackerTimer.isActive())
        m_positionTrackerTimer.start();

    emit positionChanged(timePosition);
}

/*
 * Mopidy tacklist callbacks
*/

void MediaPlayerBackend::onCurrentIndexReceived(int index)
{
    qDebug() << "onCurrentIndexReceived: " << index;
    m_currentIndex = index;

    emit currentIndexChanged(m_currentIndex);
    emit currentTrackChanged(QVariant());
    emit durationChanged(0);
}

void MediaPlayerBackend::onTracksReceived(const mopidy::Tracks &tracks)
{
    qDebug() << "OnTracksReceived";
    int id = 0;

    m_trackList.clear();
    emit dataFetched(m_trackList, 0, false);

    // TODO: from and until count
    for (auto track : tracks) {
#ifndef QTIVI_NO_TAGLIB
        if (!track.uri.endsWith(QLatin1String("mp3")))
            break;

        // Extract cover art
        // Cover art is stored in PNG file with an identical name to the one of
        // the corresponding MP3 file
        // If PNG file with an equivalent name is not present, we extract it from
        // the MP3 file and save on the filesystem alongside the MP3 file
        QString fileName = track.uri.right(track.uri.length() - 7); // remove "file://"
        QString defaultCoverArtUrl = fileName + QStringLiteral(".png");
        QString coverArtUrl;

        TagLib::FileRef f(TagLib::FileName(QFile::encodeName(fileName)));

        if (fileName.endsWith(QLatin1String("mp3"))) {
            auto *file = static_cast<TagLib::MPEG::File *>(f.file());
            TagLib::ID3v2::Tag *tag = file->ID3v2Tag(true);
            TagLib::ID3v2::FrameList frameList = tag->frameList("APIC");

            if (frameList.isEmpty()) {
                qCWarning(media) << "No cover art was found";
            } else if (!QFile::exists(defaultCoverArtUrl)) {
                auto *coverImage = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(
                    frameList.front());

                if (NULL == coverImage) {
                    qWarning(media) << "Could not extract cover art";
                    continue;
                }

                QImage coverQImg;
                coverArtUrl = defaultCoverArtUrl;

                coverQImg.loadFromData((const uchar *) coverImage->picture().data(),
                                       coverImage->picture().size());
                coverQImg.save(coverArtUrl, "PNG");
            } else {
                coverArtUrl = defaultCoverArtUrl;
            }
        }
#endif // QTIVI_NO_TAGLIB

        QIviAudioTrackItem item;
        qDebug() << "Adding track: " << track.name;

        item.setId(QString(id++));
        item.setTitle(track.name);
        if (track.artists.size())
            item.setArtist(track.artists.first().name);
#ifndef QTIVI_NO_TAGLIB
        item.setCoverArtUrl(coverArtUrl);
#endif
        item.setAlbum(track.album.name);
        item.setDuration(track.length);
        item.setGenre(track.genre);
        item.setTrackNumber(track.track_no);
        item.setYear(track.date.year());
        item.setUrl(track.uri);

        m_trackList.append(QVariant::fromValue(item));
    }

    emit countChanged(m_trackList.count());
    emit dataFetched(m_trackList, 0, false);
}

void MediaPlayerBackend::changePlayMode()
{
    QIviMediaPlayer::PlayMode playMode = QIviMediaPlayer::Normal;

    if (!m_singleMode && !m_repeatMode && m_randomMode)
        playMode = QIviMediaPlayer::Shuffle;
    else if (m_singleMode && m_repeatMode && !m_randomMode)
        playMode = QIviMediaPlayer::RepeatTrack;
    else if (!m_singleMode && m_repeatMode && !m_randomMode)
        playMode = QIviMediaPlayer::RepeatAll;

    emit playModeChanged(playMode);
}

void MediaPlayerBackend::onSingleReceived(bool isSingle)
{
    qDebug() << "onSingleReceived: " << isSingle;
    m_singleMode = isSingle;
    changePlayMode();
}

void MediaPlayerBackend::onRepeatReceived(bool isRepeat)
{
    qDebug() << "onRepeatReceived: " << isRepeat;
    m_repeatMode = isRepeat;
    changePlayMode();
}

void MediaPlayerBackend::onRandomReceived(bool isRandom)
{
    qDebug() << "onRandomReceived:" << isRandom;
    m_randomMode = isRandom;
    changePlayMode();
}

/*
 * Mixer callbacks
*/

void MediaPlayerBackend::onMuteReceived(bool isMuted)
{
    qDebug() << "onMuteReceive: " << isMuted;
    emit setMuted(isMuted);
}

void MediaPlayerBackend::onVolumeReceived(int volume)
{
    qDebug() << "onVolumeReceived: " << volume;
    emit setVolume(volume);
}

void MediaPlayerBackend::onLibraryHelperTracksInDirectoryFetched(const QString &uri,
                                                                 const mopidy::Refs &refs)
{
    qDebug() << "onLibraryHelperTracksInDirectoryFetched";
    QStringList uris;

    m_tracklistController.clear();

    for (auto ref : refs) {
        qDebug() << ref.uri;
        uris.append(ref.uri);
    }

    m_tracklistController.add(uris, 0);

    m_tracklistController.getTracks();
}
