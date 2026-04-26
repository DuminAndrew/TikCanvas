#include "BackupManager.h"
#include "MikroTikManager.h"
#include <QFile>
#include <QDateTime>

BackupManager::BackupManager(MikroTikManager *api, QObject *parent)
    : QObject(parent), m_api(api)
{
    connect(m_api, &MikroTikManager::replyReceived,
            this, &BackupManager::onReply);
}

void BackupManager::requestBackup(const QString &savePath)
{
    m_pendingPath = savePath;
    m_active = true;
    m_api->sendCommand({"/export"});
}

void BackupManager::onReply(const QStringList &words)
{
    if (!m_active) return;
    m_active = false;

    QStringList lines;
    lines << "# TikCanvas backup " + QDateTime::currentDateTime().toString(Qt::ISODate);
    for (const auto &w : words) {
        if (w.startsWith("=ret="))
            lines << w.mid(5);
        else if (w.startsWith("="))
            lines << w;
    }
    QFile f(m_pendingPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit backupFailed("Cannot write " + m_pendingPath);
        return;
    }
    const QByteArray data = lines.join('\n').toUtf8();
    f.write(data);
    emit backupSaved(m_pendingPath, data.size());
}
