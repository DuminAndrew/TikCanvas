#include "PdfRenderer.h"
#include <QPainter>
#include <QPdfDocument>
#include <QSizeF>

QImage PdfRenderer::renderFirstPage(const QString &path, double dpi)
{
    QPdfDocument doc;
    if (doc.load(path) != QPdfDocument::Error::None || doc.pageCount() == 0) {
        QImage img(800, 600, QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter p(&img);
        p.setPen(Qt::darkRed);
        p.drawText(img.rect(), Qt::AlignCenter,
                   QStringLiteral("Failed to load PDF:\n%1").arg(path));
        return img;
    }
    const QSizeF pts = doc.pagePointSize(0);
    const QSize pix(int(pts.width()  * dpi / 72.0),
                    int(pts.height() * dpi / 72.0));
    return doc.render(0, pix);
}
