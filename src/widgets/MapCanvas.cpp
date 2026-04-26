#include "MapCanvas.h"
#include "core/PdfRenderer.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtConcurrent>
#include <QFutureWatcher>

MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(640, 480);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
}

void MapCanvas::loadPdf(const QString &path)
{
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher]() {
        const QImage img = watcher->result();
        if (!img.isNull()) {
            m_page = QPixmap::fromImage(img);
            resetView();
            update();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([path]() {
        return PdfRenderer::renderFirstPage(path, 150.0);
    }));
}

void MapCanvas::resetView()
{
    m_scale = 1.0;
    m_offset = QPointF(0, 0);
}

void MapCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.fillRect(rect(), QColor("#1E1E2E"));

    if (m_page.isNull()) {
        p.setPen(QColor("#7F8C8D"));
        p.drawText(rect(), Qt::AlignCenter, tr("No map loaded\nUse 'Load PDF Map'"));
        return;
    }

    p.translate(m_offset);
    p.scale(m_scale, m_scale);
    p.drawPixmap(0, 0, m_page);
}

void MapCanvas::wheelEvent(QWheelEvent *e)
{
    const qreal factor = (e->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    m_scale = qBound(0.1, m_scale * factor, 8.0);
    update();
}

void MapCanvas::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMouse = e->position();
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        m_offset += e->position() - m_lastMouse;
        m_lastMouse = e->position();
        update();
    }
}
