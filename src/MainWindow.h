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
    void onDeviceFound(const QString &mac, const QString &ip, const QString &identity);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    MikroTikManager *m_api {nullptr};
    SyslogServer    *m_syslog {nullptr};
    MndpScanner     *m_mndp {nullptr};
    MapCanvas       *m_canvas {nullptr};
};
