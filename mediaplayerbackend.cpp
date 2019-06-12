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
    connect(&m_eventHandler,
            &mopidy::EventHandler::volumeChanged,
            this,
            &MediaPlayerBackend::onVolumeChanged);

    // library
    connect(&m_libraryHelper,
            &mopidy::LibraryHelper::tracksInDirectoryFetched,
            this,
            &MediaPlayerBackend::onLibraryHelperTracksInDirectoryFetched);

    m_tracklistController.index();

    m_libraryHelper.requestTracksInDirectory(MEDIA_FOLDER);
}

void MediaPlayerBackend::initialize()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.getState();
    m_tracklistController.getTracks();

    emit initializationDone();
}

bool MediaPlayerBackend::canReportCount()
{
    return true;
}

void MediaPlayerBackend::fetchData(int start, int count)
{
    qCDebug(media) << Q_FUNC_INFO << "from:" << start << "count:" << count;
}

void MediaPlayerBackend::insert(int index, const QIviPlayableItem *item)
{
    qCDebug(media) << Q_FUNC_INFO << index;
}

void MediaPlayerBackend::remove(int index)
{
    qCDebug(media) << Q_FUNC_INFO << index;
}

void MediaPlayerBackend::move(int curIndex, int newIndex)
{
    qCDebug(media) << Q_FUNC_INFO << "current:" << curIndex << "new:" << newIndex;

    m_tracklistController.move(curIndex, curIndex, newIndex);
}

void MediaPlayerBackend::next()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.next();
    m_tracklistController.index();
}

void MediaPlayerBackend::pause()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.pause();
}

void MediaPlayerBackend::play()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.play();
    m_tracklistController.index();
}

void MediaPlayerBackend::previous()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.previous();
    m_tracklistController.index();
}

void MediaPlayerBackend::seek(qint64 offset)
{
    qCDebug(media) << Q_FUNC_INFO << offset;

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
    qCDebug(media) << Q_FUNC_INFO << index;

    //    m_tracklistController.index(index);
}

void MediaPlayerBackend::setMuted(bool muted)
{
    qCDebug(media) << Q_FUNC_INFO << muted;

    m_mixerController.setMute(muted);
}

void MediaPlayerBackend::setPlayMode(QIviMediaPlayer::PlayMode playMode)
{
    switch (playMode) {
    case QIviMediaPlayer::Normal:
        qCDebug(media) << Q_FUNC_INFO << "Normal";

        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(false);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::RepeatTrack:
        qCDebug(media) << Q_FUNC_INFO << "RepeatTrack";

        m_tracklistController.setSingle(true);
        m_tracklistController.setRepeat(true);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::RepeatAll:
        qCDebug(media) << Q_FUNC_INFO << "RepeatAll";

        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(true);
        m_tracklistController.setRandom(false);
        break;

    case QIviMediaPlayer::Shuffle:
        qCDebug(media) << Q_FUNC_INFO << "Shuffle";

        m_tracklistController.setSingle(false);
        m_tracklistController.setRepeat(false);
        m_tracklistController.setRandom(true);
        break;

    default:
        qCWarning(media) << Q_FUNC_INFO << "unsupported mode";
        break;
    };
}

void MediaPlayerBackend::setPosition(qint64 position)
{
    qCDebug(media) << Q_FUNC_INFO << position;

    m_playbackController.seek(position);
}

void MediaPlayerBackend::setVolume(int volume)
{
    qCDebug(media) << Q_FUNC_INFO << volume;

    m_mixerController.setVolume(volume);
}

void MediaPlayerBackend::stop()
{
    qCDebug(media) << Q_FUNC_INFO;

    m_playbackController.stop();
}

/*
 * Mopidy playback calls
*/

void MediaPlayerBackend::onCurrentTlTrackReceived(const mopidy::TlTrack &tlTrack)
{
    qCDebug(media) << Q_FUNC_INFO;

    m_tracklistController.index();
    emit durationChanged(tlTrack.track.length);
}

void MediaPlayerBackend::onStateReceived(mopidy::PlaybackState state)
{
    switch (state) {
    case mopidy::PlaybackState::Paused:
        qCDebug(media) << Q_FUNC_INFO << "Paused";

        m_playbackState = QIviMediaPlayer::Paused;
        m_playbackController.getCurrentTlTrack();
        m_positionTrackerTimer.stop();
        break;

    case mopidy::PlaybackState::Playing:
        qCDebug(media) << Q_FUNC_INFO << "Playing";

        m_playbackState = QIviMediaPlayer::Playing;
        m_playbackController.getCurrentTlTrack();
        m_playbackController.getTimePosition();
        break;

    case mopidy::PlaybackState::Stopped:
        qCDebug(media) << Q_FUNC_INFO << "Stopped";

        m_playbackState = QIviMediaPlayer::Stopped;
        emit positionChanged(0);
        m_positionTrackerTimer.stop();
        break;

    default:
        qCWarning(media) << Q_FUNC_INFO << "unsupported state";
        break;
    }

    emit playStateChanged(m_playbackState);
}

void MediaPlayerBackend::onPlaybackStateChanged(mopidy::PlaybackState oldState,
                                                mopidy::PlaybackState newState)
{
    switch (newState) {
    case mopidy::PlaybackState::Paused:
        qCDebug(media) << Q_FUNC_INFO << "Paused";

        m_playbackState = QIviMediaPlayer::Paused;
        m_playbackController.getCurrentTlTrack();
        break;

    case mopidy::PlaybackState::Playing:
        qCDebug(media) << Q_FUNC_INFO << "Playing";

        m_playbackState = QIviMediaPlayer::Playing;
        m_playbackController.getCurrentTlTrack();
        m_playbackController.getTimePosition();
        break;

    case mopidy::PlaybackState::Stopped:
        qCDebug(media) << Q_FUNC_INFO << "Stopped";

        m_playbackState = QIviMediaPlayer::Stopped;
        emit positionChanged(0);
        break;

    default:
        qCWarning(media) << Q_FUNC_INFO << "unsupported new state";
        break;
    }

    emit playStateChanged(m_playbackState);
}

void MediaPlayerBackend::onSeeked(int timePosition)
{
    qCDebug(media) << Q_FUNC_INFO << timePosition;

    emit positionChanged(timePosition);
}

void MediaPlayerBackend::onTrackPlaybackStarted(const mopidy::TlTrack &tlTrack)
{
    qCDebug(media) << Q_FUNC_INFO;

    m_tracklistController.index();
}

void MediaPlayerBackend::onTracklistChanged()
{
    qCDebug(media) << Q_FUNC_INFO;
}

void MediaPlayerBackend::onTimePositionReceived(int timePosition)
{
    qCDebug(media) << Q_FUNC_INFO << timePosition;

    if (!m_positionTrackerTimer.isActive())
        m_positionTrackerTimer.start();

    emit positionChanged(timePosition);
}

/*
 * Mopidy tacklist callbacks
*/

void MediaPlayerBackend::onCurrentIndexReceived(int index)
{
    qCDebug(media) << Q_FUNC_INFO << index;

    m_currentIndex = index;

    emit currentIndexChanged(m_currentIndex);
    emit currentTrackChanged(QVariant());
}

void MediaPlayerBackend::onTracksReceived(const mopidy::Tracks &tracks)
{
    qCDebug(media) << Q_FUNC_INFO;

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
                qCWarning(media) << Q_FUNC_INFO << "No cover art was found";
            } else if (!QFile::exists(defaultCoverArtUrl)) {
                auto *coverImage = static_cast<TagLib::ID3v2::AttachedPictureFrame *>(
                    frameList.front());

                if (NULL == coverImage) {
                    qCWarning(media) << "Could not extract cover art";
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
        qCDebug(media) << Q_FUNC_INFO << "Adding track: " << track.name;

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
    qCDebug(media) << Q_FUNC_INFO << isSingle;

    m_singleMode = isSingle;
    changePlayMode();
}

void MediaPlayerBackend::onRepeatReceived(bool isRepeat)
{
    qCDebug(media) << Q_FUNC_INFO << isRepeat;

    m_repeatMode = isRepeat;
    changePlayMode();
}

void MediaPlayerBackend::onRandomReceived(bool isRandom)
{
    qCDebug(media) << Q_FUNC_INFO << isRandom;

    m_randomMode = isRandom;
    changePlayMode();
}

/*
 * Mixer callbacks
*/

void MediaPlayerBackend::onMuteReceived(bool isMuted)
{
    qCDebug(media) << Q_FUNC_INFO << isMuted;

    emit setMuted(isMuted);
}

void MediaPlayerBackend::onVolumeChanged(int volume)
{
    qCDebug(media) << Q_FUNC_INFO << volume;

    emit volumeChanged(volume);
}

void MediaPlayerBackend::onLibraryHelperTracksInDirectoryFetched(const QString &uri,
                                                                 const mopidy::Refs &refs)
{
    qCDebug(media) << Q_FUNC_INFO;

    QStringList uris;

    m_tracklistController.clear();

    for (auto ref : refs) {
        uris.append(ref.uri);
    }

    m_tracklistController.add(uris, 0);

//    m_tracklistController.getTracks();
}
