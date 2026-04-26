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
};

struct DeviceLink {
    int from {-1};
    int to   {-1};
};

class MapCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit MapCanvas(QWidget *parent = nullptr);

    QVector<DeviceNode> devices() const { return m_devices; }
    QVector<DeviceLink> links()   const { return m_links;   }
    QString             pdfPath() const { return m_pdfPath; }

    void setDevices(const QVector<DeviceNode> &d) { m_devices = d; update(); }
    void setLinks(const QVector<DeviceLink> &l)   { m_links = l;   update(); }

public slots:
    void loadPdf(const QString &path);
    void resetView();
    void addDevice(const DeviceNode &node);
    void clearAll();
    void setLinkMode(bool on);

signals:
    void deviceDoubleClicked(const DeviceNode &node);

protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

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
};
