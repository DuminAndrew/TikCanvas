#pragma once
#include <QImage>
#include <QString>

class PdfRenderer
{
public:
    static QImage renderFirstPage(const QString &path, double dpi = 150.0);
};
