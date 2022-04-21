/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Spatial Audio module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-NOGPL2$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qspatialaudiolistener.h"
#include "qspatialaudioengine_p.h"
#include "api/resonance_audio_api.h"
#include <qaudiosink.h>
#include <qurl.h>
#include <qdebug.h>
#include <qaudiodecoder.h>

QT_BEGIN_NAMESPACE

class QSpatialAudioListenerPrivate
{
public:
    QSpatialAudioEngine *engine = nullptr;
    QVector3D pos;
    QQuaternion rotation;
};

QSpatialAudioListener::QSpatialAudioListener(QSpatialAudioEngine *engine)
    : d(new QSpatialAudioListenerPrivate)
{
    setEngine(engine);
}

QSpatialAudioListener::~QSpatialAudioListener()
{
    delete d;
}

void QSpatialAudioListener::setPosition(QVector3D pos)
{
    d->pos = pos;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep && ep->api)
        ep->api->SetHeadPosition(pos.x(), pos.y(), pos.z());
}

QVector3D QSpatialAudioListener::position() const
{
    return d->pos;
}

void QSpatialAudioListener::setRotation(const QQuaternion &q)
{
    d->rotation = q;
    auto *ep = QSpatialAudioEnginePrivate::get(d->engine);
    if (ep && ep->api)
        ep->api->SetHeadRotation(d->rotation.x(), d->rotation.y(), d->rotation.z(), d->rotation.scalar());
}

QQuaternion QSpatialAudioListener::rotation() const
{
    return d->rotation;
}

void QSpatialAudioListener::setEngine(QSpatialAudioEngine *engine)
{
    d->engine = engine;
}

QSpatialAudioEngine *QSpatialAudioListener::engine() const
{
    return d->engine;
}

QT_END_NAMESPACE
