#pragma once
#include <QListWidget>
#include <QMimeData>

class DeviceListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit DeviceListWidget(QWidget *parent = nullptr) : QListWidget(parent) {
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
    }

protected:
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override {
        auto *m = new QMimeData;
        if (items.isEmpty()) return m;
        const QString payload = items.first()->data(Qt::UserRole).toString();
        m->setText(payload);
        return m;
    }
};
