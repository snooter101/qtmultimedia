/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Copyright (C) 2021 The Qt Company
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
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
****************************************************************************/
#include "qqnxmediaplayer_p.h"
#include "qqnxvideorenderer_p.h"
#include "qqnxmediautil_p.h"
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/quuid.h>
#include <mm/renderer.h>
#include <qmediaplayer.h>
#include <private/qqnxaudiooutput_p.h>
#include <qaudiooutput.h>

#include <errno.h>
#include <sys/strm.h>
#include <sys/stat.h>

QT_BEGIN_NAMESPACE

static int idCounter = 0;

QQnxMediaPlayer::QQnxMediaPlayer(QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent),
      m_context(0),
      m_id(-1),
      m_connection(0),
      m_audioId(-1),
      m_rate(1),
      m_position(0),
      m_mediaStatus(QMediaPlayer::NoMedia),
      m_playAfterMediaLoaded(false),
      m_inputAttached(false),
      m_bufferLevel(0)
{
    m_loadingTimer.setSingleShot(true);
    m_loadingTimer.setInterval(0);
    connect(&m_loadingTimer, SIGNAL(timeout()), this, SLOT(continueLoadMedia()));
    QCoreApplication::eventDispatcher()->installNativeEventFilter(this);
}

void QQnxMediaPlayer::destroy()
{
    stop();
    detach();
    closeConnection();
    QCoreApplication::eventDispatcher()->removeNativeEventFilter(this);
}

void QQnxMediaPlayer::openConnection()
{
    m_connection = mmr_connect(NULL);
    if (!m_connection) {
        emitPError(QString::fromLatin1("Unable to connect to the multimedia renderer"));
        return;
    }

    m_id = idCounter++;
    m_contextName = QString::fromLatin1("QQnxMediaPlayer_%1_%2").arg(m_id)
                                                         .arg(QCoreApplication::applicationPid());
    m_context = mmr_context_create(m_connection, m_contextName.toLatin1(),
                                   0, S_IRWXU|S_IRWXG|S_IRWXO);
    if (!m_context) {
        emitPError(QString::fromLatin1("Unable to create context"));
        closeConnection();
        return;
    }

    startMonitoring();
}

void QQnxMediaPlayer::handleMmStopped()
{
    // Only react to stop events that happen when the end of the stream is reached and
    // playback is stopped because of this.
    // Ignore other stop event sources, such as calling mmr_stop() ourselves.
    if (state() != QMediaPlayer::StoppedState) {
        setMediaStatus(QMediaPlayer::EndOfMedia);
        stopInternal(IgnoreMmRenderer);
    }
}

void QQnxMediaPlayer::handleMmSuspend(const QString &reason)
{
    if (state() == QMediaPlayer::StoppedState)
        return;

    Q_UNUSED(reason);
    setMediaStatus(QMediaPlayer::StalledMedia);
}

void QQnxMediaPlayer::handleMmSuspendRemoval(const QString &bufferProgress)
{
    if (state() == QMediaPlayer::StoppedState)
        return;

    if (bufferProgress == QLatin1String("buffering"))
        setMediaStatus(QMediaPlayer::BufferingMedia);
    else
        setMediaStatus(QMediaPlayer::BufferedMedia);
}

void QQnxMediaPlayer::handleMmPause()
{
    if (state() == QMediaPlayer::PlayingState) {
        stateChanged(QMediaPlayer::PausedState);
    }
}

void QQnxMediaPlayer::handleMmPlay()
{
    if (state() == QMediaPlayer::PausedState) {
        stateChanged(QMediaPlayer::PlayingState);
    }
}

void QQnxMediaPlayer::closeConnection()
{
    stopMonitoring();

    if (m_context) {
        mmr_context_destroy(m_context);
        m_context = 0;
        m_contextName.clear();
    }

    if (m_connection) {
        mmr_disconnect(m_connection);
        m_connection = 0;
    }
}

QByteArray QQnxMediaPlayer::resourcePathForUrl(const QUrl &url)
{
    // If this is a local file, mmrenderer expects the file:// prefix and an absolute path.
    // We treat URLs without scheme as local files, most likely someone just forgot to set the
    // file:// prefix when constructing the URL.
    if (url.isLocalFile() || url.scheme().isEmpty()) {
        QString relativeFilePath;
        if (!url.scheme().isEmpty())
            relativeFilePath = url.toLocalFile();
        else
            relativeFilePath = url.path();
        const QFileInfo fileInfo(relativeFilePath);
        return QFile::encodeName(QStringLiteral("file://") + fileInfo.absoluteFilePath());

    // HTTP or similar URL
    } else {
        return url.toEncoded();
    }
}

void QQnxMediaPlayer::attach()
{
    // Should only be called in detached state
    Q_ASSERT(m_audioId == -1 && !m_inputAttached);

    if (!m_media.isValid() || !m_context) {
        setMediaStatus(QMediaPlayer::NoMedia);
        return;
    }

    resetMonitoring();

    if (m_videoRendererControl)
        m_videoRendererControl->attachDisplay(m_context);

    if (m_videoWindowControl)
        m_videoWindowControl->attachDisplay(m_context);

    const QByteArray defaultAudioDevice = qgetenv("QQNX_RENDERER_DEFAULT_AUDIO_SINK");
    m_audioId = mmr_output_attach(m_context, defaultAudioDevice.isEmpty() ? "audio:default" : defaultAudioDevice.constData(), "audio");
    if (m_audioId == -1) {
        emitMmError("mmr_output_attach() for audio failed");
        return;
    }

//    if (m_audioId != -1) {
//        QString audioType = qnxAudioType(m_role);
//        QByteArray latin1AudioType = audioType.toLatin1();
//        if (!audioType.isEmpty() && latin1AudioType == audioType) {
//            strm_dict_t *dict = strm_dict_new();
//            dict = strm_dict_set(dict, "audio_type", latin1AudioType.constData());
//            if (mmr_output_parameters(m_context, m_audioId, dict) != 0)
//                emitMmError("mmr_output_parameters: Setting audio_type failed");
//        }
//    }

    const QByteArray resourcePath = resourcePathForUrl(m_media);
    if (resourcePath.isEmpty()) {
        detach();
        return;
    }

    if (mmr_input_attach(m_context, resourcePath.constData(), "track") != 0) {
        emitMmError(QStringLiteral("mmr_input_attach() failed for ") + QString::fromUtf8(resourcePath));
        setMediaStatus(QMediaPlayer::InvalidMedia);
        detach();
        return;
    }

    m_inputAttached = true;
    setMediaStatus(QMediaPlayer::LoadedMedia);

    // mm-renderer has buffer properties "status" and "level"
    // QMediaPlayer's buffer status maps to mm-renderer's buffer level
    m_bufferLevel = 0;
    emit bufferProgressChanged(m_bufferLevel/100.);
}

void QQnxMediaPlayer::detach()
{
    if (m_context) {
        if (m_inputAttached) {
            mmr_input_detach(m_context);
            m_inputAttached = false;
        }
        if (m_videoRendererControl)
            m_videoRendererControl->detachDisplay();
        if (m_videoWindowControl)
            m_videoWindowControl->detachDisplay();
        if (m_audioId != -1 && m_context) {
            mmr_output_detach(m_context, m_audioId);
            m_audioId = -1;
        }
    }

    m_loadingTimer.stop();
}

QMediaPlayer::MediaStatus QQnxMediaPlayer::mediaStatus() const
{
    return m_mediaStatus;
}

qint64 QQnxMediaPlayer::duration() const
{
    return m_metaData.duration();
}

qint64 QQnxMediaPlayer::position() const
{
    return m_position;
}

void QQnxMediaPlayer::setPosition(qint64 position)
{
    if (m_position != position) {
        m_position = position;

        // Don't update in stopped state, it would not have any effect. Instead, the position is
        // updated in play().
        if (state() != QMediaPlayer::StoppedState)
            setPositionInternal(m_position);

        emit positionChanged(m_position);
    }
}

void QQnxMediaPlayer::setVolumeInternal(float newVolume)
{
    if (!m_context)
        return;

    newVolume = qBound(0.f, newVolume, 1.f);
    if (m_audioId != -1) {
        strm_dict_t * dict = strm_dict_new();
        dict = strm_dict_set(dict, "volume", QString::number(int(newVolume*100.)).toLatin1());
        if (mmr_output_parameters(m_context, m_audioId, dict) != 0)
            emitMmError("mmr_output_parameters: Setting volume failed");
    }
}

void QQnxMediaPlayer::setPlaybackRateInternal(qreal rate)
{
    if (!m_context)
        return;

    const int mmRate = rate * 1000;
    if (mmr_speed_set(m_context, mmRate) != 0)
        emitMmError("mmr_speed_set failed");
}

void QQnxMediaPlayer::setPositionInternal(qint64 position)
{
    if (!m_context)
        return;

    if (true /*#### m_metaData.isSeekable()*/) {
        if (mmr_seek(m_context, QString::number(position).toLatin1()) != 0)
            emitMmError("Seeking failed");
    }
}

void QQnxMediaPlayer::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (m_mediaStatus != status) {
        m_mediaStatus = status;
        emit mediaStatusChanged(m_mediaStatus);
    }
}

void QQnxMediaPlayer::setState(QMediaPlayer::PlaybackState state)
{
    auto oldState = this->state();
    if (oldState != state) {
        if (m_videoRendererControl) {
            if (state == QMediaPlayer::PausedState || state == QMediaPlayer::StoppedState) {
                m_videoRendererControl->pause();
            } else if ((state == QMediaPlayer::PlayingState)
                       && (oldState == QMediaPlayer::PausedState
                           || oldState == QMediaPlayer::StoppedState)) {
                m_videoRendererControl->resume();
            }
        }

        stateChanged(state);
    }
}

void QQnxMediaPlayer::stopInternal(StopCommand stopCommand)
{
    resetMonitoring();
    setPosition(0);

    if (state() != QMediaPlayer::StoppedState) {

        if (stopCommand == StopMmRenderer) {
            mmr_stop(m_context);
        }

        setState(QMediaPlayer::StoppedState);
    }
}

void QQnxMediaPlayer::setVolume(float volume)
{
    const int newVolume = qBound(0.f, volume, 1.f);
    if (m_volume != newVolume) {
        m_volume = newVolume;
        setVolumeInternal(m_muted ? 0. : m_volume);
    }
}

void QQnxMediaPlayer::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        setVolumeInternal(muted ? 0 : m_volume);
    }
}

void QQnxMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    QAudioOutput *out = output->q;
    if (m_audioOutput == out)
        return;

    if (m_audioOutput)
        disconnect(m_audioOutput.get());
    m_audioOutput = out;
    if (m_audioOutput) {
        connect(out, &QAudioOutput::volumeChanged, this, &QQnxMediaPlayer::setVolume);
        connect(out, &QAudioOutput::mutedChanged, this, &QQnxMediaPlayer::setMuted);
    }
    setVolume(out ? out->volume() : 1.);
    setMuted(out ? out->isMuted() : true);
}

float QQnxMediaPlayer::bufferProgress() const
{
    // mm-renderer has buffer properties "status" and "level"
    // QMediaPlayer's buffer status maps to mm-renderer's buffer level
    return m_bufferLevel/100.;
}

bool QQnxMediaPlayer::isAudioAvailable() const
{
    return m_metaData.hasAudio();
}

bool QQnxMediaPlayer::isVideoAvailable() const
{
    return m_metaData.hasVideo();
}

bool QQnxMediaPlayer::isSeekable() const
{
    return m_metaData.isSeekable();
}

QMediaTimeRange QQnxMediaPlayer::availablePlaybackRanges() const
{
    // We can't get this information from the mmrenderer API yet, so pretend we can seek everywhere
    return QMediaTimeRange(0, m_metaData.duration());
}

qreal QQnxMediaPlayer::playbackRate() const
{
    return m_rate;
}

void QQnxMediaPlayer::setPlaybackRate(qreal rate)
{
    if (m_rate != rate) {
        m_rate = rate;
        setPlaybackRateInternal(m_rate);
        emit playbackRateChanged(m_rate);
    }
}

QUrl QQnxMediaPlayer::media() const
{
    return m_media;
}

const QIODevice *QQnxMediaPlayer::mediaStream() const
{
    // Always 0, we don't support QIODevice streams
    return 0;
}

void QQnxMediaPlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    Q_UNUSED(stream); // not supported

    stop();
    detach();

    m_media = media;

    // Slight hack: With MediaPlayer QtQuick elements that have autoPlay set to true, playback
    // would start before the QtQuick canvas is propagated to all elements, and therefore our
    // video output would not work. Therefore, delay actually playing the media a bit so that the
    // canvas is ready.
    // The mmrenderer doesn't allow to attach video outputs after playing has started, otherwise
    // this would be unnecessary.
    if (m_media.isValid()) {
        setMediaStatus(QMediaPlayer::LoadingMedia);
        m_loadingTimer.start(); // singleshot timer to continueLoadMedia()
    } else {
        continueLoadMedia(); // still needed, as it will update the media status and clear metadata
    }
}

void QQnxMediaPlayer::continueLoadMedia()
{
    updateMetaData(nullptr);
    attach();
    if (m_playAfterMediaLoaded)
        play();
}

MmRendererVideoWindowControl *QQnxMediaPlayer::videoWindowControl() const
{
    return m_videoWindowControl;
}

void QQnxMediaPlayer::play()
{
    if (m_playAfterMediaLoaded)
        m_playAfterMediaLoaded = false;

    // No-op if we are already playing, except if we were called from continueLoadMedia(), in which
    // case m_playAfterMediaLoaded is true (hence the 'else').
    else if (state() == QMediaPlayer::PlayingState)
        return;

    if (m_mediaStatus == QMediaPlayer::LoadingMedia) {

        // State changes are supposed to be synchronous
        setState(QMediaPlayer::PlayingState);

        // Defer playing to later, when the timer triggers continueLoadMedia()
        m_playAfterMediaLoaded = true;
        return;
    }

    // Un-pause the state when it is paused
    if (state() == QMediaPlayer::PausedState) {
        setPlaybackRateInternal(m_rate);
        setState(QMediaPlayer::PlayingState);
        return;
    }

    if (!m_media.isValid() || !m_connection || !m_context || m_audioId == -1) {
        setState(QMediaPlayer::StoppedState);
        return;
    }

    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        m_position = 0;

    resetMonitoring();
    setPositionInternal(m_position);
    setVolumeInternal(m_muted ? 0 : m_volume);
    setPlaybackRateInternal(m_rate);

    if (mmr_play(m_context) != 0) {
        setState(QMediaPlayer::StoppedState);
        emitMmError("mmr_play() failed");
        return;
    }

    setState( QMediaPlayer::PlayingState);
}

void QQnxMediaPlayer::pause()
{
    if (state() == QMediaPlayer::PlayingState) {
        setPlaybackRateInternal(0);
        setState(QMediaPlayer::PausedState);
    }
}

void QQnxMediaPlayer::stop()
{
    stopInternal(StopMmRenderer);
}

MmRendererPlayerVideoRendererControl *QQnxMediaPlayer::videoRendererControl() const
{
    return m_videoRendererControl;
}

void QQnxMediaPlayer::setVideoRendererControl(MmRendererPlayerVideoRendererControl *videoControl)
{
    m_videoRendererControl = videoControl;
}

void QQnxMediaPlayer::setVideoWindowControl(MmRendererVideoWindowControl *videoControl)
{
    m_videoWindowControl = videoControl;
}

void QQnxMediaPlayer::setMmPosition(qint64 newPosition)
{
    if (newPosition != 0 && newPosition != m_position) {
        m_position = newPosition;
        emit positionChanged(m_position);
    }
}

void QQnxMediaPlayer::setMmBufferStatus(const QString &bufferProgress)
{
    if (bufferProgress == QLatin1String("buffering"))
        setMediaStatus(QMediaPlayer::BufferingMedia);
    else if (bufferProgress == QLatin1String("playing"))
        setMediaStatus(QMediaPlayer::BufferedMedia);
    // ignore "idle" buffer status
}

void QQnxMediaPlayer::setMmBufferLevel(int level, int capacity)
{
    m_bufferLevel = capacity == 0 ? 0 : level / static_cast<float>(capacity) * 100.0f;
    m_bufferLevel = qBound(0, m_bufferLevel, 100);
    emit bufferProgressChanged(m_bufferLevel/100.);
}

void QQnxMediaPlayer::updateMetaData(const strm_dict *dict)
{
    m_metaData.update(dict);

    if (m_videoWindowControl)
        m_videoWindowControl->setMetaData(m_metaData);

    // ### convert to QMediaMetaData and notify the player about metadata changes
    emit durationChanged(m_metaData.duration());
    emit audioAvailableChanged(m_metaData.hasAudio());
    emit videoAvailableChanged(m_metaData.hasVideo());
//    emit availablePlaybackRangesChanged(availablePlaybackRanges());
    emit seekableChanged(m_metaData.isSeekable());
}

void QQnxMediaPlayer::emitMmError(const char *msg)
{
    emitMmError(QString::fromUtf8(msg));
}

void QQnxMediaPlayer::emitMmError(const QString &msg)
{
    int errorCode = MMR_ERROR_NONE;
    const QString errorMessage = mmErrorMessage(msg, m_context, &errorCode);
    qDebug() << errorMessage;
    emit error(errorCode, errorMessage);
}

void QQnxMediaPlayer::emitPError(const QString &msg)
{
    const QString errorMessage = QString::fromLatin1("%1: %2").arg(msg).arg(QString::fromUtf8(strerror(errno)));
    qDebug() << errorMessage;
    emit error(errno, errorMessage);
}

QT_END_NAMESPACE
