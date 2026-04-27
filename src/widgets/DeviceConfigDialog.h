#pragma once
#include <QDialog>
#include <QString>

class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QTabWidget;
class MikroTikManager;

class DeviceConfigDialog : public QDialog
{
    Q_OBJECT
public:
    DeviceConfigDialog(MikroTikManager *api,
                       const QString &host,
                       const QString &identity,
                       const QString &board,
                       QWidget *parent = nullptr);

private slots:
    void applyIdentity();
    void applyNetwork();
    void applyWifi();
    void applyDhcpClient();
    void applyRoaming();
    void doReboot();
    void doFactoryReset();
    void onReply(const QStringList &words);

private:
    void log(const QString &line);

    MikroTikManager *m_api;
    QString m_host;

    QLineEdit *m_editName;
    QLineEdit *m_editIp;
    QComboBox *m_cbIface;
    QLineEdit *m_editGw;
    QLineEdit *m_editDns;
    QLineEdit *m_editSsid;
    QLineEdit *m_editWifiPass;
    QComboBox *m_cbWifiIface;
    QComboBox *m_cbDhcpIface;
    QLineEdit *m_editConnectMin;
    QLineEdit *m_editKickBelow;
    QLineEdit *m_editStickyTime;
    QPlainTextEdit *m_log;
    QTabWidget *m_tabs;
};
