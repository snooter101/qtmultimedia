// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QFFMPEGPLAYBACKENGINE_P_H
#define QFFMPEGPLAYBACKENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

/* Playback engine design description.
 *
 *
 * PLAYBACK ENGINE OBJECTS
 *
 * - Playback engine manages 7 objects inside, each one works in a separate thread.
 * Each object inherits PlaybackEngineObject. The objects are:
 *    Demuxer
 *    Stream Decoders: audio, video, subtitles
 *    Renderers: audio, video, subtitles
 *
 *
 * THREADS:
 *
 * - By default, each object works in a separate thread. It's easy to reconfigure
 *   to using several objects in thread.
 * - New thread is allocated if a new object is created and the engine doesn't
 *   have free threads. If it does, the thread is to be reused.
 * - If all objects for some thread are deleted, the thread becomes free and the engine
 *   postpones its termination.
 *
 * OBJECTS WEAK CONNECTIVITY
 *
 * - The objects know nothing about others and about PlaybackEngine.
 *   For any interractions the objects use slots/signals.
 *
 * - PlaybackEngine knows the objects object and is able to create/delete them and
 *   call their public methods.
 *
 */

#include "playbackengine/qffmpegplaybackenginedefs_p.h"
#include "playbackengine/qffmpegtimecontroller_p.h"
#include "playbackengine/qffmpegmediadataholder_p.h"

#include <unordered_map>

QT_BEGIN_NAMESPACE

class QAudioSink;
class QVideoSink;
class QAudioOutput;
class QFFmpegMediaPlayer;

namespace QFFmpeg
{

class PlaybackEngine : public QObject, protected PlaybackEngineInternal::MediaDataHolder
{
    Q_OBJECT
public:
    PlaybackEngine();

    ~PlaybackEngine() override;

    void setMedia(const QUrl &media, QIODevice *stream);

    void setVideoSink(QVideoSink *sink);

    void setAudioSink(QAudioOutput *output);

    void setAudioSink(QPlatformAudioOutput *output);

    void setState(QMediaPlayer::PlaybackState state);

    void play() {
        setState(QMediaPlayer::PlayingState);
    }
    void pause() {
        setState(QMediaPlayer::PausedState);
    }
    void stop() {
        setState(QMediaPlayer::StoppedState);
    }

    void seek(qint64 pos);

    void setPlaybackRate(float rate);

    float playbackRate() const;

    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

    using MediaDataHolder::activeTrack;

    bool isSeekable() const;

    qint64 currentPosition() const;

    // To be removed after aligning with Decoder
    using MediaDataHolder::m_currentAVStreamIndex;
    using MediaDataHolder::m_duration;
    using MediaDataHolder::m_metaData;
    using MediaDataHolder::m_requestedStreams;
    using MediaDataHolder::m_streamMap;

signals:
    void endOfStream();
    void errorOccured(int, const QString &);

protected: // objects managing
    using Demuxer = PlaybackEngineInternal::Demuxer;
    using PlaybackEngineObject = PlaybackEngineInternal::PlaybackEngineObject;
    using Renderer = PlaybackEngineInternal::Renderer;
    using AudioRenderer = PlaybackEngineInternal::AudioRenderer;
    using VideoRenderer = PlaybackEngineInternal::VideoRenderer;
    using SubtitleRenderer = PlaybackEngineInternal::SubtitleRenderer;
    using StreamDecoder = PlaybackEngineInternal::StreamDecoder;
    using TimeController = PlaybackEngineInternal::TimeController;

    struct ObjectDeleter
    {
        void operator()(PlaybackEngineObject *) const;

        PlaybackEngine *engine = nullptr;
    };

    template<typename T>
    using ObjectPtr = std::unique_ptr<T, ObjectDeleter>;

    using RendererPtr = ObjectPtr<Renderer>;
    using StreamPtr = ObjectPtr<StreamDecoder>;

    template<typename T, typename... Args>
    ObjectPtr<T> createPlaybackEngineObject(Args &&...args);

    virtual RendererPtr createRenderer(QPlatformMediaPlayer::TrackType trackType);

private:
    void createStreamAndRenderer(QPlatformMediaPlayer::TrackType trackType);

    void createDemuxer();

    void registerObject(PlaybackEngineObject &object);

    template<typename C, typename Action>
    void forEachExistingObject(Action &&action);

    template<typename Action>
    void forEachExistingObject(Action &&action);

    void forceUpdate();

    void recreateObjects();

    void createObjectsIfNeeded();

    void updateObjectsPausedState();

    void deleteFreeThreads();

    void onRendererSynchronized(std::chrono::steady_clock::time_point time, qint64 trackTime);

    void onRendererFinished();

    void triggerStepIfNeeded();

    static QString objectThreadName(const PlaybackEngineObject &object);

    std::optional<Codec> codecForTrack(QPlatformMediaPlayer::TrackType trackType);

private:
    TimeController m_timeController;

    std::unordered_map<QString, std::unique_ptr<QThread>> m_threads;
    bool m_threadsDirty = false;

    QPointer<QVideoSink> m_videoSink;
    QPointer<QAudioOutput> m_audioOutput;

    QMediaPlayer::PlaybackState m_state = QMediaPlayer::StoppedState;

    ObjectPtr<Demuxer> m_demuxer;
    std::array<StreamPtr, QPlatformMediaPlayer::NTrackTypes> m_streams;
    std::array<RendererPtr, QPlatformMediaPlayer::NTrackTypes> m_renderers;

    std::array<std::optional<Codec>, QPlatformMediaPlayer::NTrackTypes> m_codecs;
};

template<typename T, typename... Args>
PlaybackEngine::ObjectPtr<T> PlaybackEngine::createPlaybackEngineObject(Args &&...args)
{
    auto result = ObjectPtr<T>(new T(std::forward<Args>(args)...), { this });
    registerObject(*result);
    return result;
}
}

QT_END_NAMESPACE

#endif // QFFMPEGPLAYBACKENGINE_P_H
