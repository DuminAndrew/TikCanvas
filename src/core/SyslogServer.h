#pragma once
#include <QObject>
#include <QThread>

class QUdpSocket;

class SyslogWorker : public QObject
{
    Q_OBJECT
public:
    explicit SyslogWorker(quint16 port, QObject *parent = nullptr);
public slots:
    void start();
    void stop();
signals:
    void messageReceived(const QString &host, const QString &msg);
private slots:
    void onReadyRead();
private:
    QUdpSocket *m_sock {nullptr};
    quint16 m_port;
};

class SyslogServer : public QObject
{
    Q_OBJECT
public:
    explicit SyslogServer(QObject *parent = nullptr);
    ~SyslogServer() override;
    void start(quint16 port);
signals:
    void messageReceived(const QString &host, const QString &msg);
    void doStart();
private:
    QThread       m_thread;
    SyslogWorker *m_worker {nullptr};
};
