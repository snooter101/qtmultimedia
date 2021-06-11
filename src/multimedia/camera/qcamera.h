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

#ifndef QCAMERA_H
#define QCAMERA_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

#include <QtCore/qobject.h>

#include <QtMultimedia/qcameradevice.h>

#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QCameraDevice;
class QPlatformMediaCaptureSession;
class QMediaCaptureSession;

class QCameraPrivate;
class Q_MULTIMEDIA_EXPORT QCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QCamera::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QCameraDevice cameraDevice READ cameraDevice WRITE setCameraDevice NOTIFY cameraDeviceChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(QCameraFormat cameraFormat READ cameraFormat WRITE setCameraFormat NOTIFY cameraFormatChanged)

    Q_PROPERTY(FocusMode focusMode READ focusMode WRITE setFocusMode)
    Q_PROPERTY(QPointF customFocusPoint READ customFocusPoint WRITE setCustomFocusPoint NOTIFY customFocusPointChanged)
    Q_PROPERTY(float focusDistance READ focusDistance WRITE setFocusDistance NOTIFY focusDistanceChanged)

    Q_PROPERTY(float minimumZoomFactor READ minimumZoomFactor NOTIFY minimumZoomFactorChanged)
    Q_PROPERTY(float maximumZoomFactor READ maximumZoomFactor NOTIFY maximumZoomFactorChanged)
    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(qreal exposureTime READ exposureTime NOTIFY exposureTimeChanged)
    Q_PROPERTY(int isoSensitivity READ isoSensitivity NOTIFY isoSensitivityChanged)
    Q_PROPERTY(qreal exposureCompensation READ exposureCompensation WRITE setExposureCompensation NOTIFY exposureCompensationChanged)
    Q_PROPERTY(QCamera::ExposureMode exposureMode READ exposureMode WRITE setExposureMode NOTIFY exposureModeChanged)
    Q_PROPERTY(bool flashReady READ isFlashReady NOTIFY flashReady)
    Q_PROPERTY(QCamera::FlashMode flashMode READ flashMode WRITE setFlashMode NOTIFY flashModeChanged)
    Q_PROPERTY(QCamera::TorchMode torchMode READ torchMode WRITE setTorchMode NOTIFY torchModeChanged)

    Q_PROPERTY(WhiteBalanceMode whiteBalanceMode READ whiteBalanceMode WRITE setWhiteBalanceMode NOTIFY whiteBalanceModeChanged)
    Q_PROPERTY(int colorTemperature READ colorTemperature WRITE setColorTemperature NOTIFY colorTemperatureChanged)
    Q_PROPERTY(Features supportedFeatures READ supportedFeatures NOTIFY supportedFeaturesChanged)

public:
    enum Status {
        UnavailableStatus,
        InactiveStatus,
        StartingStatus,
        StoppingStatus,
        ActiveStatus
    };
    Q_ENUM(Status)

    enum Error
    {
        NoError,
        CameraError
    };
    Q_ENUM(Error)

    enum FocusMode {
        FocusModeAuto,
        FocusModeAutoNear,
        FocusModeAutoFar,
        FocusModeHyperfocal,
        FocusModeInfinity,
        FocusModeManual
    };
    Q_ENUM(FocusMode)

    enum FlashMode {
        FlashOff,
        FlashOn,
        FlashAuto
    };
    Q_ENUM(FlashMode)

    enum TorchMode {
        TorchOff,
        TorchOn,
        TorchAuto
    };
    Q_ENUM(TorchMode)

    enum ExposureMode {
        ExposureAuto,
        ExposureManual,
        ExposurePortrait,
        ExposureNight,
        ExposureSports,
        ExposureSnow,
        ExposureBeach,
        ExposureAction,
        ExposureLandscape,
        ExposureNightPortrait,
        ExposureTheatre,
        ExposureSunset,
        ExposureSteadyPhoto,
        ExposureFireworks,
        ExposureParty,
        ExposureCandlelight,
        ExposureBarcode
    };
    Q_ENUM(ExposureMode)

    enum WhiteBalanceMode {
        WhiteBalanceAuto = 0,
        WhiteBalanceManual = 1,
        WhiteBalanceSunlight = 2,
        WhiteBalanceCloudy = 3,
        WhiteBalanceShade = 4,
        WhiteBalanceTungsten = 5,
        WhiteBalanceFluorescent = 6,
        WhiteBalanceFlash = 7,
        WhiteBalanceSunset = 8
    };
    Q_ENUM(WhiteBalanceMode)

    enum class Feature {
        ColorTemperature = 0x1,
        ExposureCompensation = 0x2,
        IsoSensitivity = 0x4,
        ManualExposureTime = 0x8,
        CustomFocusPoint = 0x10,
        FocusDistance = 0x20
    };
    Q_DECLARE_FLAGS(Features, Feature)

    explicit QCamera(QObject *parent = nullptr);
    explicit QCamera(const QCameraDevice& cameraDevice, QObject *parent = nullptr);
    explicit QCamera(QCameraDevice::Position position, QObject *parent = nullptr);
    ~QCamera();

    bool isAvailable() const;
    bool isActive() const;

    Status status() const;

    QMediaCaptureSession *captureSession() const;

    QCameraDevice cameraDevice() const;
    void setCameraDevice(const QCameraDevice &cameraDevice);

    QCameraFormat cameraFormat() const;
    void setCameraFormat(const QCameraFormat &format);

    Error error() const;
    QString errorString() const;

    Features supportedFeatures() const;

    FocusMode focusMode() const;
    void setFocusMode(FocusMode mode);
    bool isFocusModeSupported(FocusMode mode) const;

    QPointF focusPoint() const;

    QPointF customFocusPoint() const;
    void setCustomFocusPoint(const QPointF &point);

    void setFocusDistance(float d);
    float focusDistance() const;

    float minimumZoomFactor() const;
    float maximumZoomFactor() const;
    float zoomFactor() const;
    void setZoomFactor(float factor);

    FlashMode flashMode() const;
    bool isFlashModeSupported(FlashMode mode) const;
    bool isFlashReady() const;

    TorchMode torchMode() const;
    bool isTorchModeSupported(TorchMode mode) const;

    ExposureMode exposureMode() const;
    bool isExposureModeSupported(ExposureMode mode) const;

    qreal exposureCompensation() const;

    int isoSensitivity() const;
    int manualIsoSensitivity() const;

    float exposureTime() const;
    float manualExposureTime() const;

    int minimumIsoSensitivity() const;
    int maximumIsoSensitivity() const;

    float minimumShutterSpeed() const;
    float maximumShutterSpeed() const;

    WhiteBalanceMode whiteBalanceMode() const;
    Q_INVOKABLE bool isWhiteBalanceModeSupported(WhiteBalanceMode mode) const;

    int colorTemperature() const;

public Q_SLOTS:
    void setActive(bool active);
    void start() { setActive(true); }
    void stop() { setActive(false); }

    void zoomTo(float zoom, float rate);

    void setFlashMode(FlashMode mode);
    void setTorchMode(TorchMode mode);
    void setExposureMode(ExposureMode mode);

    void setExposureCompensation(qreal ev);

    void setManualIsoSensitivity(int iso);
    void setAutoIsoSensitivity();

    void setManualExposureTime(float seconds);
    void setAutoExposureTime();

    void setWhiteBalanceMode(WhiteBalanceMode mode);
    void setColorTemperature(int colorTemperature);

Q_SIGNALS:
    void activeChanged(bool);
    void statusChanged(QCamera::Status status);
    void errorChanged();
    void errorOccurred(QCamera::Error error, const QString &errorString);
    void cameraDeviceChanged();
    void cameraFormatChanged();
    void supportedFeaturesChanged();

    void focusModeChanged();
    void zoomFactorChanged(float);
    void minimumZoomFactorChanged(float);
    void maximumZoomFactorChanged(float);
    void focusDistanceChanged(float);
    void customFocusPointChanged();

    void flashReady(bool);
    void flashModeChanged();
    void torchModeChanged();

    void exposureTimeChanged(qreal speed);
    void isoSensitivityChanged(int);
    void exposureCompensationChanged(qreal);
    void exposureModeChanged();

    void whiteBalanceModeChanged() const;
    void colorTemperatureChanged() const;

    void brightnessChanged();
    void contrastChanged();
    void saturationChanged();
    void hueChanged();

private:
    void setCaptureSession(QMediaCaptureSession *session);
    friend class QMediaCaptureSession;
    Q_DISABLE_COPY(QCamera)
    Q_DECLARE_PRIVATE(QCamera)
    Q_PRIVATE_SLOT(d_func(), void _q_error(int, const QString &))
    friend class QCameraDevice;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QCamera::Features)

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QCamera, Status)
Q_MEDIA_ENUM_DEBUG(QCamera, Error)

#endif  // QCAMERA_H
