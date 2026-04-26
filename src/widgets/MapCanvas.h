#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPointF>

class MapCanvas : public QWidget
{
    Q_OBJECT
public:
    explicit MapCanvas(QWidget *parent = nullptr);

public slots:
    void loadPdf(const QString &path);
    void resetView();

protected:
    void paintEvent(QPaintEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;

private:
    QPixmap m_page;
    qreal   m_scale {1.0};
    QPointF m_offset;
    QPointF m_lastMouse;
    bool    m_dragging {false};
};
