// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegencoderthread_p.h"
#include "qmetaobject.h"

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

EncoderThread::EncoderThread(RecordingEngine &recordingEngine) : m_recordingEngine(recordingEngine)
{
}

void EncoderThread::setPaused(bool paused)
{
    auto guard = lockLoopData();
    m_paused = paused;
}

void EncoderThread::setAutoStop(bool autoStop)
{
    auto guard = lockLoopData();
    m_autoStop = autoStop;
}

void EncoderThread::setEndOfSourceStream()
{
    {
        auto guard = lockLoopData();
        m_endOfSourceStream = true;
    }

    emit endOfSourceStream();
}

} // namespace QFFmpeg

QT_END_NAMESPACE

#include "moc_qffmpegencoderthread_p.cpp"
