#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

class MikroTikManager;

class BackupManager : public QObject
{
    Q_OBJECT
public:
    explicit BackupManager(MikroTikManager *api, QObject *parent = nullptr);

public slots:
    void requestBackup(const QString &savePath);

signals:
    void backupSaved(const QString &path, qint64 bytes);
    void backupFailed(const QString &reason);

private slots:
    void onReply(const QStringList &words);

private:
    MikroTikManager *m_api;
    QString m_pendingPath;
    bool    m_active {false};
};
