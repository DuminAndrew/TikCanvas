#include "SyslogServer.h"
#include <QUdpSocket>
#include <QNetworkDatagram>

SyslogWorker::SyslogWorker(quint16 port, QObject *parent)
    : QObject(parent), m_port(port) {}

void SyslogWorker::start()
{
    m_sock = new QUdpSocket(this);
    if (!m_sock->bind(QHostAddress::AnyIPv4, m_port, QUdpSocket::ShareAddress))
        return;
    connect(m_sock, &QUdpSocket::readyRead, this, &SyslogWorker::onReadyRead);
}

void SyslogWorker::stop()
{
    if (m_sock) { m_sock->close(); m_sock->deleteLater(); m_sock = nullptr; }
}

void SyslogWorker::onReadyRead()
{
    while (m_sock && m_sock->hasPendingDatagrams()) {
        const auto dg = m_sock->receiveDatagram();
        emit messageReceived(dg.senderAddress().toString(),
                             QString::fromUtf8(dg.data()).trimmed());
    }
}

SyslogServer::SyslogServer(QObject *parent) : QObject(parent) {}

SyslogServer::~SyslogServer()
{
    m_thread.quit();
    m_thread.wait();
}

void SyslogServer::start(quint16 port)
{
    m_worker = new SyslogWorker(port);
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &SyslogServer::doStart, m_worker, &SyslogWorker::start);
    connect(m_worker, &SyslogWorker::messageReceived, this, &SyslogServer::messageReceived);
    m_thread.start();
    emit doStart();
}
