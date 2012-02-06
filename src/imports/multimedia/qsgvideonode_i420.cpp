/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qsgvideonode_i420.h"
#include <QtCore/qmutex.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_I420::supportedPixelFormats(
                                        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> formats;

    if (handleType == QAbstractVideoBuffer::NoHandle)
        formats << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12;

    return formats;
}

QSGVideoNode *QSGVideoNodeFactory_I420::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_I420(format);

    return 0;
}

QStringList QSGVideoNodeFactory_I420::keys() const
{
    return QStringList() << QSGVideoNodeFactoryPluginKey;
}


class QSGVideoMaterialShader_YUV420 : public QSGMaterialShader
{
public:
    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual char const *const *attributeNames() const {
        static const char *names[] = {
            "qt_VertexPosition",
            "qt_VertexTexCoord",
            0
        };
        return names;
    }

protected:

    virtual const char *vertexShader() const {
        const char *shader =
        "uniform highp mat4 qt_Matrix;                      \n"
        "attribute highp vec4 qt_VertexPosition;            \n"
        "attribute highp vec2 qt_VertexTexCoord;            \n"
        "varying highp vec2 qt_TexCoord;                    \n"
        "void main() {                                      \n"
        "    qt_TexCoord = qt_VertexTexCoord;               \n"
        "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
        "}";
        return shader;
    }

    virtual const char *fragmentShader() const {
        static const char *shader =
        "uniform sampler2D yTexture;"
        "uniform sampler2D uTexture;"
        "uniform sampler2D vTexture;"
        "uniform mediump mat4 colorMatrix;"
        "uniform lowp float opacity;"
        ""
        "varying highp vec2 qt_TexCoord;"
        ""
        "void main()"
        "{"
        "    mediump float Y = texture2D(yTexture, qt_TexCoord).r;"
        "    mediump float U = texture2D(uTexture, qt_TexCoord).r;"
        "    mediump float V = texture2D(vTexture, qt_TexCoord).r;"
        "    mediump vec4 color = vec4(Y, U, V, 1.);"
        "    gl_FragColor = colorMatrix * color * opacity;"
        "}";
        return shader;
    }

    virtual void initialize() {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_yTexture = program()->uniformLocation("yTexture");
        m_id_uTexture = program()->uniformLocation("uTexture");
        m_id_vTexture = program()->uniformLocation("vTexture");
        m_id_colorMatrix = program()->uniformLocation("colorMatrix");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    int m_id_matrix;
    int m_id_yTexture;
    int m_id_uTexture;
    int m_id_vTexture;
    int m_id_colorMatrix;
    int m_id_opacity;
};


class QSGVideoMaterial_YUV420 : public QSGMaterial
{
public:
    QSGVideoMaterial_YUV420(const QVideoSurfaceFormat &format);
    ~QSGVideoMaterial_YUV420();

    virtual QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    virtual QSGMaterialShader *createShader() const {
        return new QSGVideoMaterialShader_YUV420;
    }

    virtual int compare(const QSGMaterial *other) const {
        const QSGVideoMaterial_YUV420 *m = static_cast<const QSGVideoMaterial_YUV420 *>(other);
        int d = m_textureIds[0] - m->m_textureIds[0];
        if (d)
            return d;
        else if ((d = m_textureIds[1] - m->m_textureIds[1]) != 0)
            return d;
        else
            return m_textureIds[2] - m->m_textureIds[2];
    }

    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true);
    }

    void setCurrentFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    void bind();
    void bindTexture(int id, int w, int h, const uchar *bits);

    QVideoSurfaceFormat m_format;
    QSize m_textureSize;
    GLuint m_textureIds[3];

    qreal m_opacity;
    QMatrix4x4 m_colorMatrix;

    QVideoFrame m_frame;
    QMutex m_frameMutex;
};

QSGVideoMaterial_YUV420::QSGVideoMaterial_YUV420(const QVideoSurfaceFormat &format) :
    m_format(format),
    m_opacity(1.0)
{
    switch (format.yCbCrColorSpace()) {
    case QVideoSurfaceFormat::YCbCr_JPEG:
        m_colorMatrix = QMatrix4x4(
                    1.0,  0.000,  1.402, -0.701,
                    1.0, -0.344, -0.714,  0.529,
                    1.0,  1.772,  0.000, -0.886,
                    0.0,  0.000,  0.000,  1.0000);
        break;
    case QVideoSurfaceFormat::YCbCr_BT709:
    case QVideoSurfaceFormat::YCbCr_xvYCC709:
        m_colorMatrix = QMatrix4x4(
                    1.164,  0.000,  1.793, -0.5727,
                    1.164, -0.534, -0.213,  0.3007,
                    1.164,  2.115,  0.000, -1.1302,
                    0.0,    0.000,  0.000,  1.0000);
        break;
    default: //BT 601:
        m_colorMatrix = QMatrix4x4(
                    1.164,  0.000,  1.596, -0.8708,
                    1.164, -0.392, -0.813,  0.5296,
                    1.164,  2.017,  0.000, -1.081,
                    0.0,    0.000,  0.000,  1.0000);
    }

    setFlag(Blending, false);
}

QSGVideoMaterial_YUV420::~QSGVideoMaterial_YUV420()
{
    if (!m_textureSize.isEmpty())
        glDeleteTextures(3, m_textureIds);
}

void QSGVideoMaterial_YUV420::bind()
{
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();

    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isValid()) {
        if (m_frame.map(QAbstractVideoBuffer::ReadOnly)) {
            int fw = m_frame.width();
            int fh = m_frame.height();

            // Frame has changed size, recreate textures...
            if (m_textureSize != m_frame.size()) {
                if (!m_textureSize.isEmpty())
                    glDeleteTextures(3, m_textureIds);
                glGenTextures(3, m_textureIds);
                m_textureSize = m_frame.size();
            }

            const uchar *bits = m_frame.bits();
            int bpl = m_frame.bytesPerLine();
            int bpl2 = (bpl / 2 + 3) & ~3;
            int offsetU = bpl * fh;
            int offsetV = bpl * fh + bpl2 * fh / 2;

            if (m_frame.pixelFormat() == QVideoFrame::Format_YV12)
                qSwap(offsetU, offsetV);


            functions->glActiveTexture(GL_TEXTURE1);
            bindTexture(m_textureIds[1], fw/2, fh / 2, bits + offsetU);
            functions->glActiveTexture(GL_TEXTURE2);
            bindTexture(m_textureIds[2], fw/2, fh / 2, bits + offsetV);
            functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
            bindTexture(m_textureIds[0], fw, fh, bits);

            m_frame.unmap();
        }

        m_frame = QVideoFrame();
    } else {
        functions->glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        functions->glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
        glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    }
}

void QSGVideoMaterial_YUV420::bindTexture(int id, int w, int h, const uchar *bits)
{
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bits);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

QSGVideoNode_I420::QSGVideoNode_I420(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    m_material = new QSGVideoMaterial_YUV420(format);
    setMaterial(m_material);
}

QSGVideoNode_I420::~QSGVideoNode_I420()
{
}

void QSGVideoNode_I420::setCurrentFrame(const QVideoFrame &frame)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}


void QSGVideoMaterialShader_YUV420::updateState(const RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    QSGVideoMaterial_YUV420 *mat = static_cast<QSGVideoMaterial_YUV420 *>(newMaterial);
    program()->setUniformValue(m_id_yTexture, 0);
    program()->setUniformValue(m_id_uTexture, 1);
    program()->setUniformValue(m_id_vTexture, 2);

    mat->bind();

    program()->setUniformValue(m_id_colorMatrix, mat->m_colorMatrix);
    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}
