// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTVIDEOBUFFER_H
#define QABSTRACTVIDEOBUFFER_H

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

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qvideoframe.h>

#include <QtCore/qmetatype.h>
#include <QtGui/qmatrix4x4.h>
#include <QtCore/private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE


class QVariant;
class QRhi;
class QRhiTexture;

class Q_MULTIMEDIA_EXPORT QVideoFrameTextures
{
public:
    virtual ~QVideoFrameTextures() {}
    virtual QRhiTexture *texture(uint plane) const = 0;
};

class Q_MULTIMEDIA_EXPORT QAbstractVideoBuffer
{
public:
    struct MapData
    {
        int nPlanes = 0;
        int bytesPerLine[4] = {};
        uchar *data[4] = {};
        int size[4] = {};
    };

    virtual ~QAbstractVideoBuffer();
    virtual MapData map(QtVideo::MapMode mode) = 0;
    virtual void unmap() = 0;
};

class Q_MULTIMEDIA_EXPORT QHwVideoBuffer : public QAbstractVideoBuffer
{
public:
    QHwVideoBuffer(QVideoFrame::HandleType type, QRhi *rhi = nullptr);

    ~QHwVideoBuffer() override;

    QVideoFrame::HandleType handleType() const { return m_type; }
    QRhi *rhi() const { return m_rhi; }

    virtual std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *) { return {}; }
    virtual quint64 textureHandle(QRhi *, int /*plane*/) const { return 0; }

    virtual QMatrix4x4 externalTextureMatrix() const { return {}; }

protected:
    QVideoFrame::HandleType m_type;
    QRhi *m_rhi = nullptr;
};

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug, QtVideo::MapMode);
#endif

QT_END_NAMESPACE

#endif
