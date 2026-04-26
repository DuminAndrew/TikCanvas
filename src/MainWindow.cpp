#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "core/MikroTikManager.h"
#include "core/SyslogServer.h"
#include "core/MndpScanner.h"
#include "core/ProjectFile.h"
#include "core/BackupManager.h"
#include "core/SshLauncher.h"
#include "core/I18n.h"
#include "widgets/MapCanvas.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QActionGroup>
#include <QMimeData>
#include <QDrag>
#include <QListWidget>

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
    m_backup = new BackupManager(m_api, this);

    ui->listDevices->setDragEnabled(true);
    ui->listDevices->setDragDropMode(QAbstractItemView::DragOnly);

    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->btnReboot,  &QPushButton::clicked, this, &MainWindow::onRebootClicked);
    connect(ui->btnScan,    &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(ui->btnLoadPdf, &QPushButton::clicked, this, &MainWindow::onLoadPdfClicked);
    connect(ui->btnLinkMode,&QPushButton::toggled, this, &MainWindow::onToggleLinkMode);
    connect(ui->btnBackup,  &QPushButton::clicked, this, &MainWindow::onBackup);
    connect(ui->btnSsh,     &QPushButton::clicked, this, &MainWindow::onSsh);

    connect(m_syslog, &SyslogServer::messageReceived, this, &MainWindow::onSyslogMessage);
    connect(m_mndp,   &MndpScanner::deviceFound,      this, &MainWindow::onDeviceFound);

    connect(m_api, &MikroTikManager::connected, this, [this]() {
        statusBar()->showMessage(tr("Connected"), 3000);
    });
    connect(m_api, &MikroTikManager::errorOccurred, this, [this](const QString &m) {
        statusBar()->showMessage(tr("Error: ") + m, 5000);
    });
    connect(m_backup, &BackupManager::backupSaved, this, [this](const QString &p, qint64 b) {
        QMessageBox::information(this, tr("Backup"),
            tr("Saved %1 (%2 bytes)").arg(p).arg(b));
    });
    connect(m_backup, &BackupManager::backupFailed, this, [this](const QString &r) {
        QMessageBox::warning(this, tr("Backup"), r);
    });

    buildMenus();
    applyTheme("dark");

    m_syslog->start(514);
    m_mndp->start();

    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() = default;

void MainWindow::buildMenus()
{
    auto *menubar = menuBar();

    auto *mFile = menubar->addMenu(tr("&File"));
    mFile->addAction(tr("&New Project"),  this, &MainWindow::onNewProject,    QKeySequence::New);
    mFile->addAction(tr("&Open..."),      this, &MainWindow::onOpenProject,   QKeySequence::Open);
    mFile->addAction(tr("&Save"),         this, &MainWindow::onSaveProject,   QKeySequence::Save);
    mFile->addAction(tr("Save &As..."),   this, &MainWindow::onSaveProjectAs, QKeySequence::SaveAs);
    mFile->addSeparator();
    mFile->addAction(tr("E&xit"), this, &QWidget::close, QKeySequence::Quit);

    auto *mView = menubar->addMenu(tr("&View"));
    auto *aDark  = mView->addAction(tr("Dark Theme"));
    auto *aLight = mView->addAction(tr("Light Theme"));
    aDark->setCheckable(true);  aLight->setCheckable(true);
    aDark->setChecked(true);
    auto *themeGroup = new QActionGroup(this);
    themeGroup->addAction(aDark); themeGroup->addAction(aLight);
    connect(aLight, &QAction::toggled, this, &MainWindow::onToggleTheme);

    auto *mTools = menubar->addMenu(tr("&Tools"));
    auto *aLink = mTools->addAction(tr("Toggle &Link Mode"));
    aLink->setCheckable(true);
    connect(aLink, &QAction::toggled, this, &MainWindow::onToggleLinkMode);
    connect(aLink, &QAction::toggled, ui->btnLinkMode, &QPushButton::setChecked);
    mTools->addAction(tr("&Backup Config..."), this, &MainWindow::onBackup);
    mTools->addAction(tr("Open &SSH Terminal"), this, &MainWindow::onSsh);

    auto *mLang = menubar->addMenu(tr("&Language"));
    mLang->addAction(tr("Русский"), this, [this]() { onLanguage("ru"); });
    mLang->addAction(tr("English"), this, [this]() { onLanguage("en"); });

    auto *mHelp = menubar->addMenu(tr("&Help"));
    mHelp->addAction(tr("&About"), this, &MainWindow::onAbout);
}

void MainWindow::applyTheme(const QString &which)
{
    m_currentTheme = which;
    QFile f(QStringLiteral(":/styles/%1.qss").arg(which));
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        qApp->setStyleSheet(QTextStream(&f).readAll());
}

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
    auto *item = new QListWidgetItem(QStringLiteral("%1  %2  %3").arg(ip, mac, identity));
    item->setData(Qt::UserRole, ip + "\t" + mac + "\t" + identity);
    ui->listDevices->addItem(item);
}

void MainWindow::onNewProject()
{
    m_canvas->clearAll();
    m_currentProject.clear();
    setWindowTitle("TikCanvas");
}

void MainWindow::onOpenProject()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Project"), {},
        tr("TikCanvas Project (*.tikcanvas)"));
    if (path.isEmpty()) return;
    QString pdf;
    QVector<DeviceNode> dev;
    QVector<DeviceLink> lnk;
    if (!ProjectFile::load(path, pdf, dev, lnk)) {
        QMessageBox::warning(this, tr("Open"), tr("Failed to load project"));
        return;
    }
    m_canvas->clearAll();
    if (!pdf.isEmpty()) m_canvas->loadPdf(pdf);
    m_canvas->setDevices(dev);
    m_canvas->setLinks(lnk);
    m_currentProject = path;
    setWindowTitle("TikCanvas — " + path);
}

bool MainWindow::saveTo(const QString &path)
{
    if (!ProjectFile::save(path, m_canvas->pdfPath(),
                           m_canvas->devices(), m_canvas->links())) {
        QMessageBox::warning(this, tr("Save"), tr("Failed to save project"));
        return false;
    }
    m_currentProject = path;
    setWindowTitle("TikCanvas — " + path);
    statusBar()->showMessage(tr("Saved %1").arg(path), 3000);
    return true;
}

void MainWindow::onSaveProject()
{
    if (m_currentProject.isEmpty()) { onSaveProjectAs(); return; }
    saveTo(m_currentProject);
}

void MainWindow::onSaveProjectAs()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Project"), "untitled.tikcanvas",
        tr("TikCanvas Project (*.tikcanvas)"));
    if (path.isEmpty()) return;
    saveTo(path);
}

void MainWindow::onToggleTheme(bool light)
{
    applyTheme(light ? "light" : "dark");
}

void MainWindow::onToggleLinkMode(bool on)
{
    m_canvas->setLinkMode(on);
    statusBar()->showMessage(on ? tr("Link mode ON") : tr("Link mode OFF"), 2000);
}

void MainWindow::onBackup()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save backup"),
        QStringLiteral("backup_%1.rsc").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm")),
        tr("RouterOS Script (*.rsc)"));
    if (path.isEmpty()) return;
    m_backup->requestBackup(path);
    statusBar()->showMessage(tr("Requesting /export ..."));
}

void MainWindow::onSsh()
{
    const QString host = ui->editHost->text().trimmed();
    const QString user = ui->editUser->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, tr("SSH"), tr("Host is required"));
        return;
    }
    if (!SshLauncher::launch(host, user))
        QMessageBox::warning(this, tr("SSH"), tr("Failed to launch ssh"));
}

void MainWindow::onLanguage(const QString &code)
{
    I18n::instance().setLanguage(code);
    ui->retranslateUi(this);
    statusBar()->showMessage(tr("Language: %1").arg(code), 2000);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About TikCanvas"),
        tr("<h2>TikCanvas 1.0.0</h2>"
           "<p>High-performance MikroTik operations canvas.</p>"
           "<p>Qt 6 · C++20 · Poppler-Qt6</p>"
           "<p>© 2026 Andrew Dumin · MIT License</p>"));
}

void MainWindow::onListDeviceContextDrag() {}
