#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPointF>
#include <QVector>
#include <QString>

struct DeviceNode {
    QString id;
    QString name;
    QString ip;
    QPointF pos;
    double  coverageRadius {0.0};
    QString coverageColor {"#4FC3F7"};
};

struct DeviceLink {
    int from {-1};
    int to   {-1};
};

enum class DrawMode { None, Pen, Rect, Erase };

struct DrawStroke {
    QString kind;
    QVector<QPointF> points;
    QString color;
    int     width {2};
};

class MapCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit MapCanvas(QWidget *parent = nullptr);

    QVector<DeviceNode> devices() const { return m_devices; }
    QVector<DeviceLink> links()   const { return m_links;   }
    QVector<DrawStroke> strokes() const { return m_strokes; }
    QString             pdfPath() const { return m_pdfPath; }

    void setDevices(const QVector<DeviceNode> &d) { m_devices = d; update(); }
    void setLinks(const QVector<DeviceLink> &l)   { m_links = l;   update(); }
    void setStrokes(const QVector<DrawStroke> &s) { m_strokes = s; update(); }

public slots:
    void loadPdf(const QString &path);
    void resetView();
    void addDevice(const DeviceNode &node);
    void clearAll();
    void setLinkMode(bool on);
    void rotate(int degrees);
    void setDrawMode(DrawMode m);
    void clearStrokes();
    void newBlankMap();
    void vectorizeCurrentPdf();
    void setDeviceRadius(int idx, double r, const QString &color = {});
    QImage currentPageImage() const;

signals:
    void deviceDoubleClicked(const DeviceNode &node);
    void deviceCoverageRequested(int idx);

protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    int  hitTestDevice(const QPointF &widgetPos) const;
    QPointF widgetToScene(const QPointF &p) const;

    QPixmap m_page;
    QString m_pdfPath;
    qreal   m_scale {1.0};
    QPointF m_offset;
    QPointF m_lastMouse;
    bool    m_panning {false};

    QVector<DeviceNode> m_devices;
    QVector<DeviceLink> m_links;
    int     m_draggingDevice {-1};
    bool    m_linkMode {false};
    int     m_linkFrom {-1};
    int     m_rotation {0};
    DrawMode m_drawMode {DrawMode::None};
    QVector<DrawStroke> m_strokes;
    DrawStroke m_currentStroke;
    QPointF    m_rectStart;
    bool       m_drawing {false};
};
