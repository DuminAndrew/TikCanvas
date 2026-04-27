#pragma once
#include <QObject>
#include <QThread>
#include <QTimer>

class QUdpSocket;

class MndpWorker : public QObject
{
    Q_OBJECT
public:
    explicit MndpWorker(QObject *parent = nullptr);
public slots:
    void start();
    void discover();
signals:
    void deviceFound(const QString &mac, const QString &ip,
                     const QString &identity, const QString &version,
                     const QString &board, const QString &platform);
private slots:
    void onReadyRead();
private:
    QUdpSocket *m_sock {nullptr};
    QTimer     *m_timer {nullptr};
};

class MndpScanner : public QObject
{
    Q_OBJECT
public:
    explicit MndpScanner(QObject *parent = nullptr);
    ~MndpScanner() override;
    void start();
    void triggerDiscovery();
signals:
    void deviceFound(const QString &mac, const QString &ip,
                     const QString &identity, const QString &version,
                     const QString &board, const QString &platform);
    void doStart();
    void doDiscover();
private:
    QThread     m_thread;
    MndpWorker *m_worker {nullptr};
};
