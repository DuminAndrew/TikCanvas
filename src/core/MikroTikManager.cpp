#include "MikroTikManager.h"

MikroTikWorker::MikroTikWorker(QObject *parent) : QObject(parent) {}

static QByteArray encodeLength(quint32 len)
{
    QByteArray out;
    if (len < 0x80) {
        out.append(char(len));
    } else if (len < 0x4000) {
        len |= 0x8000u;
        out.append(char((len >> 8) & 0xFF));
        out.append(char(len & 0xFF));
    } else if (len < 0x200000) {
        len |= 0xC00000u;
        out.append(char((len >> 16) & 0xFF));
        out.append(char((len >> 8) & 0xFF));
        out.append(char(len & 0xFF));
    } else if (len < 0x10000000) {
        len |= 0xE0000000u;
        out.append(char((len >> 24) & 0xFF));
        out.append(char((len >> 16) & 0xFF));
        out.append(char((len >> 8) & 0xFF));
        out.append(char(len & 0xFF));
    } else {
        out.append(char(0xF0));
        out.append(char((len >> 24) & 0xFF));
        out.append(char((len >> 16) & 0xFF));
        out.append(char((len >> 8) & 0xFF));
        out.append(char(len & 0xFF));
    }
    return out;
}

void MikroTikWorker::writeWord(const QByteArray &word)
{
    if (!m_sock) return;
    m_sock->write(encodeLength(quint32(word.size())));
    m_sock->write(word);
}

void MikroTikWorker::writeSentence(const QStringList &sentence)
{
    for (const auto &w : sentence)
        writeWord(w.toUtf8());
    writeWord(QByteArray());
    if (m_sock) m_sock->flush();
}

static bool readN(QTcpSocket *s, QByteArray &out, int n)
{
    out.clear();
    while (out.size() < n) {
        if (s->bytesAvailable() == 0 && !s->waitForReadyRead(5000))
            return false;
        out.append(s->read(n - out.size()));
    }
    return true;
}

QByteArray MikroTikWorker::readWord()
{
    if (!m_sock) return {};
    QByteArray b;
    if (!readN(m_sock, b, 1)) return {};
    quint32 len = quint8(b.at(0));
    if ((len & 0x80) == 0) {
    } else if ((len & 0xC0) == 0x80) {
        QByteArray b2;
        if (!readN(m_sock, b2, 1)) return {};
        len = ((len & 0x3F) << 8) | quint8(b2.at(0));
    } else if ((len & 0xE0) == 0xC0) {
        QByteArray b2;
        if (!readN(m_sock, b2, 2)) return {};
        len = ((len & 0x1F) << 16) | (quint8(b2.at(0)) << 8) | quint8(b2.at(1));
    } else if ((len & 0xF0) == 0xE0) {
        QByteArray b2;
        if (!readN(m_sock, b2, 3)) return {};
        len = ((len & 0x0F) << 24) | (quint8(b2.at(0)) << 16) | (quint8(b2.at(1)) << 8) | quint8(b2.at(2));
    } else {
        QByteArray b2;
        if (!readN(m_sock, b2, 4)) return {};
        len = (quint8(b2.at(0)) << 24) | (quint8(b2.at(1)) << 16) | (quint8(b2.at(2)) << 8) | quint8(b2.at(3));
    }
    if (len == 0) return {};
    QByteArray data;
    if (!readN(m_sock, data, int(len))) return {};
    return data;
}

QStringList MikroTikWorker::readSentence()
{
    QStringList out;
    while (true) {
        QByteArray w = readWord();
        if (w.isEmpty()) break;
        out << QString::fromUtf8(w);
    }
    return out;
}

bool MikroTikWorker::authenticate(const QString &user, const QString &pass)
{
    writeSentence({"/login", "=name=" + user, "=password=" + pass});
    const QStringList reply = readSentence();
    return !reply.isEmpty() && reply.first() == "!done";
}

void MikroTikWorker::connectToDevice(const QString &host, quint16 port,
                                     const QString &user, const QString &pass)
{
    if (m_sock) { m_sock->disconnectFromHost(); m_sock->deleteLater(); }
    m_sock = new QTcpSocket(this);
    m_sock->connectToHost(host, port);
    if (!m_sock->waitForConnected(5000)) {
        emit errorOccurred(QStringLiteral("Connect failed: %1").arg(m_sock->errorString()));
        return;
    }
    if (!authenticate(user, pass)) {
        emit errorOccurred(QStringLiteral("Authentication failed"));
        m_sock->disconnectFromHost();
        return;
    }
    emit connected();
}

void MikroTikWorker::sendCommand(const QStringList &words)
{
    if (!m_sock || m_sock->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred(QStringLiteral("Not connected"));
        return;
    }
    writeSentence(words);
    QStringList reply;
    while (true) {
        QStringList s = readSentence();
        if (s.isEmpty()) break;
        reply += s;
        if (s.first() == "!done" || s.first() == "!fatal") break;
    }
    emit replyReceived(reply);
}

void MikroTikWorker::disconnectFromDevice()
{
    if (m_sock) {
        m_sock->disconnectFromHost();
        m_sock->deleteLater();
        m_sock = nullptr;
        emit disconnected();
    }
}

MikroTikManager::MikroTikManager(QObject *parent) : QObject(parent)
{
    m_worker = new MikroTikWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MikroTikManager::doConnect, m_worker, &MikroTikWorker::connectToDevice);
    connect(this, &MikroTikManager::doSend,    m_worker, &MikroTikWorker::sendCommand);
    connect(m_worker, &MikroTikWorker::connected,     this, &MikroTikManager::connected);
    connect(m_worker, &MikroTikWorker::disconnected,  this, &MikroTikManager::disconnected);
    connect(m_worker, &MikroTikWorker::replyReceived, this, &MikroTikManager::replyReceived);
    connect(m_worker, &MikroTikWorker::errorOccurred, this, &MikroTikManager::errorOccurred);
    m_thread.start();
}

MikroTikManager::~MikroTikManager()
{
    m_thread.quit();
    m_thread.wait();
}

void MikroTikManager::connectToDevice(const QString &host, quint16 port,
                                      const QString &user, const QString &pass)
{
    emit doConnect(host, port, user, pass);
}

void MikroTikManager::sendCommand(const QStringList &words)
{
    emit doSend(words);
}
