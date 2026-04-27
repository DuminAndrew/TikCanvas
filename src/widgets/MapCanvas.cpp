#include "MapCanvas.h"
#include "core/PdfRenderer.h"
#include "core/Vectorizer.h"

#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
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
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
}

void MapCanvas::loadPdf(const QString &path)
{
    setLinkMode(false);
    setDrawMode(DrawMode::None);
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

void MapCanvas::rotate(int degrees)
{
    if (m_page.isNull()) return;
    QTransform t;
    t.rotate(degrees);
    m_page = m_page.transformed(t, Qt::SmoothTransformation);
    m_rotation = (m_rotation + degrees) % 360;
    update();
}

void MapCanvas::setDrawMode(DrawMode m)
{
    m_drawMode = m;
    setCursor(m == DrawMode::None ? Qt::ArrowCursor : Qt::CrossCursor);
}

void MapCanvas::clearStrokes()
{
    m_strokes.clear();
    update();
}

void MapCanvas::newBlankMap()
{
    m_page = {};
    m_pdfPath.clear();
    resetView();
    update();
}

QImage MapCanvas::currentPageImage() const
{
    return m_page.isNull() ? QImage() : m_page.toImage();
}

void MapCanvas::vectorizeCurrentPdf()
{
    if (m_page.isNull()) return;
    QImage src = m_page.toImage().convertToFormat(QImage::Format_Grayscale8);
    const int W = src.width(), H = src.height();
    const int THRESH = 170;

    QImage cleaned(W, H, QImage::Format_ARGB32_Premultiplied);
    cleaned.fill(Qt::transparent);
    QPainter pp(&cleaned);
    pp.fillRect(cleaned.rect(), QColor("#0A0A12"));
    QImage mask(W, H, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    for (int y = 0; y < H; ++y) {
        const uchar *row = src.scanLine(y);
        QRgb *dst = reinterpret_cast<QRgb *>(mask.scanLine(y));
        for (int x = 0; x < W; ++x)
            dst[x] = (row[x] < THRESH) ? qRgba(255, 255, 255, 255) : 0;
    }
    pp.drawImage(0, 0, mask);
    pp.end();

    m_page = QPixmap::fromImage(cleaned);
    m_pdfPath.clear();
    update();
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
    else {
        p.setPen(QPen(QColor("#1f2a44"), 1.0 / m_scale));
        for (int x = -2000; x <= 4000; x += 50) p.drawLine(x, -2000, x, 4000);
        for (int y = -2000; y <= 4000; y += 50) p.drawLine(-2000, y, 4000, y);
    }

    for (const auto &st : m_strokes) {
        QPen pen(QColor(st.color.isEmpty() ? "#FFFFFF" : st.color),
                 st.width / m_scale);
        pen.setCapStyle(Qt::RoundCap); pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        if (st.kind == "rect" && st.points.size() == 2) {
            p.drawRect(QRectF(st.points[0], st.points[1]).normalized());
        } else if (st.kind == "pen" && st.points.size() >= 2) {
            for (int i = 1; i < st.points.size(); ++i)
                p.drawLine(st.points[i-1], st.points[i]);
        }
    }
    if (m_drawing && !m_currentStroke.points.isEmpty()) {
        QPen pen(QColor(m_currentStroke.color), m_currentStroke.width / m_scale);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen); p.setBrush(Qt::NoBrush);
        if (m_currentStroke.kind == "rect" && m_currentStroke.points.size() == 2)
            p.drawRect(QRectF(m_currentStroke.points[0], m_currentStroke.points[1]).normalized());
        else for (int i = 1; i < m_currentStroke.points.size(); ++i)
            p.drawLine(m_currentStroke.points[i-1], m_currentStroke.points[i]);
    }

    QPen linkPen(QColor("#4FC3F7"), 2.0 / m_scale);
    p.setPen(linkPen);
    for (const auto &l : m_links) {
        if (l.from < 0 || l.to < 0 ||
            l.from >= m_devices.size() || l.to >= m_devices.size()) continue;
        p.drawLine(m_devices[l.from].pos, m_devices[l.to].pos);
    }

    for (const auto &d : m_devices) {
        if (d.coverageRadius > 0) {
            QColor c(d.coverageColor.isEmpty() ? "#4FC3F7" : d.coverageColor);
            QRadialGradient grad(d.pos, d.coverageRadius);
            QColor inner = c; inner.setAlphaF(0.35);
            QColor outer = c; outer.setAlphaF(0.0);
            grad.setColorAt(0.0, inner);
            grad.setColorAt(1.0, outer);
            p.setBrush(grad);
            p.setPen(QPen(c, 1.5 / m_scale, Qt::DashLine));
            p.drawEllipse(d.pos, d.coverageRadius, d.coverageRadius);
        }
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
        p.setBrush(QColor(255, 64, 129, 220));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRect(8, 8, 220, 26), 6, 6);
        p.setPen(Qt::white);
        p.drawText(QRect(14, 8, 220, 26), Qt::AlignVCenter,
                   tr("⚡ Режим связей — Esc чтобы выйти"));
    }
    if (m_drawMode != DrawMode::None) {
        p.setBrush(QColor(79, 195, 247, 220));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRect(8, 40, 220, 26), 6, 6);
        p.setPen(QColor("#11111B"));
        const QString name = m_drawMode == DrawMode::Pen ? "Карандаш"
                           : m_drawMode == DrawMode::Rect ? "Прямоугольник" : "Ластик";
        p.drawText(QRect(14, 40, 220, 26), Qt::AlignVCenter,
                   tr("✏ %1 — Esc чтобы выйти").arg(name));
    }
}

void MapCanvas::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        if (m_linkMode || m_drawMode != DrawMode::None) {
            setLinkMode(false);
            setDrawMode(DrawMode::None);
            update();
            return;
        }
    }
    QWidget::keyPressEvent(e);
}

void MapCanvas::wheelEvent(QWheelEvent *e)
{
    const qreal factor = (e->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    m_scale = qBound(0.1, m_scale * factor, 8.0);
    update();
}

void MapCanvas::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    const QPointF scenePt = widgetToScene(e->position());
    if (m_drawMode == DrawMode::Pen || m_drawMode == DrawMode::Rect) {
        m_drawing = true;
        m_currentStroke = {};
        m_currentStroke.kind  = (m_drawMode == DrawMode::Pen) ? "pen" : "rect";
        m_currentStroke.color = "#FFFFFF";
        m_currentStroke.width = 2;
        m_currentStroke.points.append(scenePt);
        if (m_drawMode == DrawMode::Rect) m_currentStroke.points.append(scenePt);
        return;
    }
    if (m_drawMode == DrawMode::Erase) {
        for (int i = m_strokes.size() - 1; i >= 0; --i) {
            for (const auto &pt : m_strokes[i].points) {
                if (QLineF(pt, scenePt).length() < 12.0 / m_scale) {
                    m_strokes.removeAt(i); update(); return;
                }
            }
        }
        return;
    }
    const int idx = hitTestDevice(e->position());
    if (m_linkMode && idx >= 0) {
        if (m_linkFrom < 0) m_linkFrom = idx;
        else if (m_linkFrom != idx) { m_links.push_back({m_linkFrom, idx}); m_linkFrom = -1; update(); }
        return;
    }
    if (idx >= 0) { m_draggingDevice = idx; m_lastMouse = e->position(); }
    else          { m_panning = true; m_lastMouse = e->position(); }
}

void MapCanvas::mouseMoveEvent(QMouseEvent *e)
{
    if (m_drawing) {
        const QPointF scenePt = widgetToScene(e->position());
        if (m_currentStroke.kind == "rect") m_currentStroke.points[1] = scenePt;
        else m_currentStroke.points.append(scenePt);
        update();
        return;
    }
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
    if (m_drawing) {
        if (!m_currentStroke.points.isEmpty()) m_strokes.push_back(m_currentStroke);
        m_currentStroke = {};
        m_drawing = false;
        update();
    }
    m_draggingDevice = -1;
    m_panning = false;
}

void MapCanvas::mouseDoubleClickEvent(QMouseEvent *e)
{
    const int idx = hitTestDevice(e->position());
    if (idx >= 0) emit deviceCoverageRequested(idx);
}

void MapCanvas::setDeviceRadius(int idx, double r, const QString &color)
{
    if (idx < 0 || idx >= m_devices.size()) return;
    m_devices[idx].coverageRadius = r;
    if (!color.isEmpty()) m_devices[idx].coverageColor = color;
    update();
}

void MapCanvas::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasText() ||
        e->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
        e->acceptProposedAction();
}

void MapCanvas::dropEvent(QDropEvent *e)
{
    QString text = e->mimeData()->text();
    if (text.isEmpty() && e->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        const QByteArray raw = e->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream s(raw);
        int row, col;
        QMap<int, QVariant> roles;
        while (!s.atEnd()) {
            s >> row >> col >> roles;
            if (roles.contains(Qt::UserRole)) text = roles.value(Qt::UserRole).toString();
            else if (roles.contains(Qt::DisplayRole)) text = roles.value(Qt::DisplayRole).toString();
            if (!text.isEmpty()) break;
        }
    }
    DeviceNode n;
    const auto parts = text.split('\t');
    n.ip   = parts.value(0).trimmed();
    n.name = parts.value(2, n.ip).trimmed();
    if (n.name.isEmpty()) n.name = n.ip;
    n.pos  = widgetToScene(e->position());
    addDevice(n);
    e->acceptProposedAction();
}
