/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QMEDIAENCODER_P_H
#define QMEDIAENCODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmediaencoder.h"
#include "qcamera.h"
#include <QtCore/qurl.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QPlatformMediaRecorder;
class QMediaContainerControl;
class QAudioEncoderSettingsControl;
class QVideoEncoderSettingsControl;
class QTimer;

class QMediaEncoderPrivate
{
    Q_DECLARE_PUBLIC(QMediaEncoder)

public:
    QMediaEncoderPrivate() = default;

    void applySettingsLater();

    QMediaCaptureSession *captureSession = nullptr;

    QPlatformMediaRecorder *control = nullptr;

    bool settingsChanged = false;

    QMediaEncoder::State state = QMediaEncoder::StoppedState;
    QMediaEncoder::Error error = QMediaEncoder::NoError;
    QString errorString;
    QUrl actualLocation;
    QMediaEncoderSettings encoderSettings;

    void _q_stateChanged(QMediaEncoder::State state);
    void _q_error(int error, const QString &errorString);
    void _q_updateActualLocation(const QUrl &);
    void _q_applySettings();

    QMediaEncoder *q_ptr = nullptr;
};

#undef Q_DECLARE_NON_CONST_PUBLIC

QT_END_NAMESPACE

#endif

