#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "core/MikroTikManager.h"
#include "core/SyslogServer.h"
#include "core/MndpScanner.h"
#include "widgets/MapCanvas.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QVBoxLayout>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);

    m_canvas = new MapCanvas(this);
    auto *lay = new QVBoxLayout(ui->canvasContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_canvas);

    m_api    = new MikroTikManager(this);
    m_syslog = new SyslogServer(this);
    m_mndp   = new MndpScanner(this);

    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->btnReboot,  &QPushButton::clicked, this, &MainWindow::onRebootClicked);
    connect(ui->btnScan,    &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(ui->btnLoadPdf, &QPushButton::clicked, this, &MainWindow::onLoadPdfClicked);

    connect(m_syslog, &SyslogServer::messageReceived, this, &MainWindow::onSyslogMessage);
    connect(m_mndp,   &MndpScanner::deviceFound,      this, &MainWindow::onDeviceFound);

    m_syslog->start(514);
    m_mndp->start();

    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnectClicked()
{
    const QString host = ui->editHost->text().trimmed();
    const QString user = ui->editUser->text().trimmed();
    const QString pass = ui->editPass->text();
    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("TikCanvas"), tr("Host is required"));
        return;
    }
    m_api->connectToDevice(host, 8728, user, pass);
    statusBar()->showMessage(tr("Connecting to %1 ...").arg(host));
}

void MainWindow::onRebootClicked()
{
    if (QMessageBox::question(this, tr("Reboot"), tr("Reboot device?"))
        == QMessageBox::Yes) {
        m_api->sendCommand({"/system/reboot"});
    }
}

void MainWindow::onScanClicked()
{
    ui->listDevices->clear();
    m_mndp->triggerDiscovery();
    statusBar()->showMessage(tr("MNDP discovery sent..."));
}

void MainWindow::onLoadPdfClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open PDF"), {}, tr("PDF (*.pdf)"));
    if (path.isEmpty()) return;
    m_canvas->loadPdf(path);
}

void MainWindow::onSyslogMessage(const QString &host, const QString &msg)
{
    const QString line = QStringLiteral("[%1] %2  %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), host, msg);
    ui->logView->appendPlainText(line);
}

void MainWindow::onDeviceFound(const QString &mac, const QString &ip, const QString &identity)
{
    ui->listDevices->addItem(QStringLiteral("%1  %2  %3").arg(ip, mac, identity));
}
