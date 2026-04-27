#pragma once
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MikroTikManager;
class SyslogServer;
class MndpScanner;
class MapCanvas;
class BackupManager;
class DeviceListWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onRebootClicked();
    void onScanClicked();
    void onLoadPdfClicked();
    void onSyslogMessage(const QString &host, const QString &msg);
    void onDeviceFound(const QString &mac, const QString &ip, const QString &identity,
                       const QString &version, const QString &board, const QString &platform);

    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();

    void onToggleTheme(bool light);
    void onToggleLinkMode(bool on);
    void onBackup();
    void onSsh();
    void onLanguage(const QString &code);
    void onAbout();

    void onListDeviceContextDrag();
    void onSendCmd();
    void onApiReply(const QStringList &words);
    void onApiConnected();
    void onApiError(const QString &msg);
    void onConfigureSelected();
    void onPlaceSelectedOnMap();

private:
    void setStatus(const QString &state, const QString &color);
    static QString classifyBoard(const QString &board);
    static QString iconForRole(const QString &role);

private:
    void applyTheme(const QString &which);
    void buildMenus();
    bool saveTo(const QString &path);

    std::unique_ptr<Ui::MainWindow> ui;
    MikroTikManager *m_api {nullptr};
    SyslogServer    *m_syslog {nullptr};
    MndpScanner     *m_mndp {nullptr};
    MapCanvas       *m_canvas {nullptr};
    BackupManager   *m_backup {nullptr};

    QString m_currentProject;
    QString m_currentTheme {"dark"};
    int     m_autoHostScore {-1};
    bool    m_hostUserEdited {false};
    bool    m_connected {false};
    int     m_pendingInfoQueries {0};
    QTimer *m_neighborPollTimer {nullptr};
};
