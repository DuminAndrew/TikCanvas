#include "MapCanvas.h"
#include "core/PdfRenderer.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QUuid>

static constexpr qreal kNodeRadius = 18.0;

MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(640, 480);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
}

void MapCanvas::loadPdf(const QString &path)
{
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, path]() {
        const QImage img = watcher->result();
        if (!img.isNull()) {
            m_page = QPixmap::fromImage(img);
            m_pdfPath = path;
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

void MapCanvas::addDevice(const DeviceNode &node)
{
    DeviceNode n = node;
    if (n.id.isEmpty()) n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (n.pos.isNull()) n.pos = QPointF(width()/2, height()/2);
    m_devices.push_back(n);
    update();
}

void MapCanvas::clearAll()
{
    m_devices.clear();
    m_links.clear();
    m_page = {};
    m_pdfPath.clear();
    resetView();
    update();
}

void MapCanvas::setLinkMode(bool on)
{
    m_linkMode = on;
    m_linkFrom = -1;
    setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
}

QPointF MapCanvas::widgetToScene(const QPointF &p) const
{
    return (p - m_offset) / m_scale;
}

int MapCanvas::hitTestDevice(const QPointF &widgetPos) const
{
    const QPointF s = widgetToScene(widgetPos);
    for (int i = m_devices.size() - 1; i >= 0; --i) {
        if (QLineF(s, m_devices[i].pos).length() <= kNodeRadius)
            return i;
    }
    return -1;
}

void MapCanvas::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.fillRect(rect(), QColor("#1E1E2E"));

    p.save();
    p.translate(m_offset);
    p.scale(m_scale, m_scale);

    if (!m_page.isNull())
        p.drawPixmap(0, 0, m_page);

    QPen linkPen(QColor("#4FC3F7"), 2.0 / m_scale);
    p.setPen(linkPen);
    for (const auto &l : m_links) {
        if (l.from < 0 || l.to < 0 ||
            l.from >= m_devices.size() || l.to >= m_devices.size()) continue;
        p.drawLine(m_devices[l.from].pos, m_devices[l.to].pos);
    }

    for (const auto &d : m_devices) {
        QRectF circ(d.pos.x() - kNodeRadius, d.pos.y() - kNodeRadius,
                    kNodeRadius * 2, kNodeRadius * 2);
        p.setBrush(QColor("#11111B"));
        p.setPen(QPen(QColor("#4FC3F7"), 2.5 / m_scale));
        p.drawEllipse(circ);
        p.setPen(QColor("#FFFFFF"));
        QFont f = p.font();
        f.setPointSizeF(8.0 / m_scale);
        f.setBold(true);
        p.setFont(f);
        p.drawText(circ, Qt::AlignCenter, "MT");
        p.setPen(QColor("#A6ADC8"));
        f.setBold(false);
        f.setPointSizeF(7.0 / m_scale);
        p.setFont(f);
        p.drawText(QRectF(d.pos.x() - 80, d.pos.y() + kNodeRadius + 2,
                          160, 20), Qt::AlignCenter,
                   d.name.isEmpty() ? d.ip : d.name);
    }
    p.restore();

    if (m_page.isNull() && m_devices.isEmpty()) {
        p.setPen(QColor("#7F8C8D"));
        p.drawText(rect(), Qt::AlignCenter,
                   tr("No map loaded\nUse 'Load PDF Map' or drag devices here"));
    }

    if (m_linkMode) {
        p.setPen(QColor("#FF4081"));
        p.drawText(QRect(8, 8, 400, 20), Qt::AlignLeft,
                   tr("LINK MODE: click two devices to connect"));
    }
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
        const int idx = hitTestDevice(e->position());
        if (m_linkMode && idx >= 0) {
            if (m_linkFrom < 0) {
                m_linkFrom = idx;
            } else if (m_linkFrom != idx) {
                m_links.push_back({m_linkFrom, idx});
                m_linkFrom = -1;
                update();
            }
            return;
        }
        if (idx >= 0) {
            m_draggingDevice = idx;
            m_lastMouse = e->position();
        } else {
            m_panning = true;
            m_lastMouse = e->position();
        }
    }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (m_draggingDevice >= 0) {
        const QPointF delta = (e->position() - m_lastMouse) / m_scale;
        m_devices[m_draggingDevice].pos += delta;
        m_lastMouse = e->position();
        update();
    } else if (m_panning) {
        m_offset += e->position() - m_lastMouse;
        m_lastMouse = e->position();
        update();
    }
}

void MapCanvas::mouseReleaseEvent(QMouseEvent *)
{
    m_draggingDevice = -1;
    m_panning = false;
}

void MapCanvas::mouseDoubleClickEvent(QMouseEvent *e)
{
    const int idx = hitTestDevice(e->position());
    if (idx >= 0) emit deviceDoubleClicked(m_devices[idx]);
}

void MapCanvas::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasText()) e->acceptProposedAction();
}

void MapCanvas::dropEvent(QDropEvent *e)
{
    const QString text = e->mimeData()->text();
    DeviceNode n;
    const auto parts = text.split('\t');
    n.ip   = parts.value(0).trimmed();
    n.name = parts.value(2, n.ip).trimmed();
    n.pos  = widgetToScene(e->position());
    addDevice(n);
    e->acceptProposedAction();
}
