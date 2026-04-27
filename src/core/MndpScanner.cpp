#include "MndpScanner.h"
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QNetworkInterface>

MndpWorker::MndpWorker(QObject *parent) : QObject(parent) {}

void MndpWorker::start()
{
    m_sock = new QUdpSocket(this);
    m_sock->bind(QHostAddress::AnyIPv4, 5678,
                 QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_sock, &QUdpSocket::readyRead, this, &MndpWorker::onReadyRead);

    m_timer = new QTimer(this);
    m_timer->setInterval(5000);
    connect(m_timer, &QTimer::timeout, this, &MndpWorker::discover);
    m_timer->start();

    QTimer::singleShot(200, this, &MndpWorker::discover);
}

void MndpWorker::discover()
{
    if (!m_sock) return;
    const QByteArray req(4, 0);
    m_sock->writeDatagram(req, QHostAddress::Broadcast, 5678);
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::CanBroadcast)) continue;
        for (const auto &entry : iface.addressEntries()) {
            if (entry.broadcast().isNull()) continue;
            m_sock->writeDatagram(req, entry.broadcast(), 5678);
        }
    }
}

void MndpWorker::onReadyRead()
{
    while (m_sock && m_sock->hasPendingDatagrams()) {
        const auto dg = m_sock->receiveDatagram();
        const QByteArray data = dg.data();
        if (data.size() < 4) continue;

        QString mac, ip = dg.senderAddress().toString();
        QString identity, version, board, platform;
        int pos = 4;
        while (pos + 4 <= data.size()) {
            quint16 type = (quint8(data[pos]) << 8) | quint8(data[pos+1]);
            quint16 len  = (quint8(data[pos+2]) << 8) | quint8(data[pos+3]);
            pos += 4;
            if (pos + len > data.size()) break;
            QByteArray val = data.mid(pos, len);
            switch (type) {
                case 0x0001:
                    if (val.size() >= 6) {
                        mac = QStringLiteral("%1:%2:%3:%4:%5:%6")
                            .arg(quint8(val[0]), 2, 16, QChar('0'))
                            .arg(quint8(val[1]), 2, 16, QChar('0'))
                            .arg(quint8(val[2]), 2, 16, QChar('0'))
                            .arg(quint8(val[3]), 2, 16, QChar('0'))
                            .arg(quint8(val[4]), 2, 16, QChar('0'))
                            .arg(quint8(val[5]), 2, 16, QChar('0'))
                            .toUpper();
                    }
                    break;
                case 0x0005: identity = QString::fromUtf8(val); break;
                case 0x0007: version  = QString::fromUtf8(val); break;
                case 0x0008: platform = QString::fromUtf8(val); break;
                case 0x000B: board    = QString::fromUtf8(val); break;
                default: break;
            }
            pos += len;
        }
        emit deviceFound(mac, ip, identity, version, board, platform);
    }
}

MndpScanner::MndpScanner(QObject *parent) : QObject(parent) {}

MndpScanner::~MndpScanner()
{
    m_thread.quit();
    m_thread.wait();
}

void MndpScanner::start()
{
    m_worker = new MndpWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MndpScanner::doStart,    m_worker, &MndpWorker::start);
    connect(this, &MndpScanner::doDiscover, m_worker, &MndpWorker::discover);
    connect(m_worker, &MndpWorker::deviceFound, this, &MndpScanner::deviceFound);
    m_thread.start();
    emit doStart();
}

void MndpScanner::triggerDiscovery()
{
    emit doDiscover();
}
