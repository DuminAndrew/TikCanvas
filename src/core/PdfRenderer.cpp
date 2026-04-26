#include "PdfRenderer.h"
#include <QPainter>

#ifndef TIKCANVAS_NO_POPPLER
#include <poppler-qt6.h>
#include <memory>
#endif

QImage PdfRenderer::renderFirstPage(const QString &path, double dpi)
{
#ifndef TIKCANVAS_NO_POPPLER
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc || doc->isLocked() || doc->numPages() == 0)
        return {};
    doc->setRenderHint(Poppler::Document::Antialiasing, true);
    doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
    std::unique_ptr<Poppler::Page> page(doc->page(0));
    if (!page) return {};
    return page->renderToImage(dpi, dpi);
#else
    Q_UNUSED(path);
    Q_UNUSED(dpi);
    QImage img(800, 600, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    p.setPen(Qt::darkGray);
    p.drawText(img.rect(), Qt::AlignCenter, "Poppler-Qt6 not available\n(stub renderer)");
    return img;
#endif
}
