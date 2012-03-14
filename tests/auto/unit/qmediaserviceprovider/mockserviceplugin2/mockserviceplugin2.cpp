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

#include <qmediaserviceproviderplugin.h>
#include <qmediaservice.h>
#include "../mockservice.h"

class MockServicePlugin2 : public QMediaServiceProviderPlugin,
                            public QMediaServiceSupportedFormatsInterface,
                            public QMediaServiceFeaturesInterface
{
    Q_OBJECT
    Q_INTERFACES(QMediaServiceSupportedFormatsInterface)
    Q_INTERFACES(QMediaServiceFeaturesInterface)
    Q_PLUGIN_METADATA(IID "com.nokia.Qt.QMediaServiceProviderFactoryInterface/1.0" FILE "mockserviceplugin2.json")
public:
    QStringList keys() const
    {
        return QStringList() << QLatin1String(Q_MEDIASERVICE_MEDIAPLAYER)
                             << QLatin1String(Q_MEDIASERVICE_RADIO);
    }

    QMediaService* create(QString const& key)
    {
        if (keys().contains(key))
            return new MockMediaService("MockServicePlugin2");
        else
            return 0;
    }

    void release(QMediaService *service)
    {
        delete service;
    }

    QtMultimedia::SupportEstimate hasSupport(const QString &mimeType, const QStringList& codecs) const
    {
        Q_UNUSED(codecs);

        if (mimeType == "audio/wav")
            return QtMultimedia::PreferredService;

        return QtMultimedia::NotSupported;
    }

    QStringList supportedMimeTypes() const
    {
        return QStringList("audio/wav");
    }

    QMediaServiceProviderHint::Features supportedFeatures(const QByteArray &service) const
    {
        if (service == QByteArray(Q_MEDIASERVICE_MEDIAPLAYER))
            return QMediaServiceProviderHint::LowLatencyPlayback;
        else
            return 0;
    }
};

#include "mockserviceplugin2.moc"

