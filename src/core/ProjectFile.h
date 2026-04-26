#pragma once
#include <QString>
#include <QVector>
#include "../widgets/MapCanvas.h"

class ProjectFile
{
public:
    static bool save(const QString &path,
                     const QString &pdfPath,
                     const QVector<DeviceNode> &devices,
                     const QVector<DeviceLink> &links);

    static bool load(const QString &path,
                     QString &pdfPath,
                     QVector<DeviceNode> &devices,
                     QVector<DeviceLink> &links);
};
