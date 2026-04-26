#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QTcpSocket>
#include <QByteArray>

class MikroTikWorker : public QObject
{
    Q_OBJECT
public:
    explicit MikroTikWorker(QObject *parent = nullptr);

public slots:
    void connectToDevice(const QString &host, quint16 port,
                         const QString &user, const QString &pass);
    void sendCommand(const QStringList &words);
    void disconnectFromDevice();

signals:
    void connected();
    void disconnected();
    void replyReceived(const QStringList &words);
    void errorOccurred(const QString &msg);

private:
    void writeWord(const QByteArray &word);
    void writeSentence(const QStringList &sentence);
    QByteArray readWord();
    QStringList readSentence();
    bool authenticate(const QString &user, const QString &pass);

    QTcpSocket *m_sock {nullptr};
};

class MikroTikManager : public QObject
{
    Q_OBJECT
public:
    explicit MikroTikManager(QObject *parent = nullptr);
    ~MikroTikManager() override;

    void connectToDevice(const QString &host, quint16 port,
                         const QString &user, const QString &pass);
    void sendCommand(const QStringList &words);

signals:
    void connected();
    void disconnected();
    void replyReceived(const QStringList &words);
    void errorOccurred(const QString &msg);

    void doConnect(const QString &host, quint16 port,
                   const QString &user, const QString &pass);
    void doSend(const QStringList &words);

private:
    QThread        m_thread;
    MikroTikWorker *m_worker {nullptr};
};
