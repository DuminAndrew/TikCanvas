#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "core/MikroTikManager.h"
#include "core/SyslogServer.h"
#include "core/MndpScanner.h"
#include "core/ProjectFile.h"
#include "core/BackupManager.h"
#include "core/SshLauncher.h"
#include "core/I18n.h"
#include "core/ErrorAdvisor.h"
#include "widgets/MapCanvas.h"
#include "widgets/DeviceConfigDialog.h"
#include "widgets/DeviceListWidget.h"

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
#include <QTimer>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);

    m_canvas = new MapCanvas(this);
    auto *lay = new QVBoxLayout(ui->canvasContainer);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_canvas);

    connect(m_canvas, &MapCanvas::deviceCoverageRequested, this, [this](int idx) {
        const auto devs = m_canvas->devices();
        if (idx < 0 || idx >= devs.size()) return;
        const double current = devs[idx].coverageRadius;

        QDialog dlg(this);
        dlg.setWindowTitle(tr("Радиус покрытия — %1").arg(devs[idx].name));
        auto *v = new QVBoxLayout(&dlg);
        v->addWidget(new QLabel(tr("<b>%1</b><br><i>Покажет зону покрытия Wi-Fi на карте.</i>").arg(devs[idx].name)));

        auto *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 1000);
        slider->setValue(int(current));
        slider->setTickPosition(QSlider::TicksBelow);
        slider->setTickInterval(100);
        auto *valueLbl = new QLabel(QStringLiteral("%1 px").arg(int(current)));
        valueLbl->setAlignment(Qt::AlignCenter);
        valueLbl->setStyleSheet("font-size:18px;font-weight:bold;color:#4FC3F7;");

        auto *colorCombo = new QComboBox;
        colorCombo->addItem("🔵 Голубой",   QString("#4FC3F7"));
        colorCombo->addItem("🟢 Зелёный",  QString("#69F0AE"));
        colorCombo->addItem("🟡 Жёлтый",   QString("#FFD54F"));
        colorCombo->addItem("🔴 Красный",  QString("#FF5252"));
        colorCombo->addItem("🟣 Фиолет",   QString("#E040FB"));
        for (int i = 0; i < colorCombo->count(); ++i)
            if (colorCombo->itemData(i).toString() == devs[idx].coverageColor) {
                colorCombo->setCurrentIndex(i); break;
            }

        auto apply = [this, idx, slider, colorCombo, valueLbl](){
            m_canvas->setDeviceRadius(idx, slider->value(), colorCombo->currentData().toString());
            valueLbl->setText(QStringLiteral("%1 px").arg(slider->value()));
        };
        connect(slider,      &QSlider::valueChanged, &dlg, apply);
        connect(colorCombo,  QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, apply);

        v->addWidget(valueLbl);
        v->addWidget(slider);
        v->addWidget(new QLabel(tr("Цвет зоны:")));
        v->addWidget(colorCombo);

        auto *row = new QHBoxLayout;
        auto *btnHide = new QPushButton(tr("Скрыть радиус"));
        auto *btnOk   = new QPushButton(tr("Готово"));
        btnOk->setDefault(true);
        connect(btnHide, &QPushButton::clicked, &dlg, [&](){ slider->setValue(0); });
        connect(btnOk,   &QPushButton::clicked, &dlg, &QDialog::accept);
        row->addWidget(btnHide);
        row->addStretch();
        row->addWidget(btnOk);
        v->addLayout(row);

        dlg.resize(420, 280);
        dlg.exec();
    });

    m_api    = new MikroTikManager(this);
    m_syslog = new SyslogServer(this);
    m_mndp   = new MndpScanner(this);
    m_backup = new BackupManager(m_api, this);

    connect(ui->editHost, &QLineEdit::textEdited, this,
            [this](const QString &) { m_hostUserEdited = true; });
    connect(ui->listDevices, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *it) {
        const QString ip = it->data(Qt::UserRole).toString().section('\t', 0, 0);
        if (!ip.isEmpty()) { ui->editHost->setText(ip); m_hostUserEdited = true; }
    });

    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->btnReboot,  &QPushButton::clicked, this, &MainWindow::onRebootClicked);
    connect(ui->btnScan,    &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(ui->btnLoadPdf, &QPushButton::clicked, this, &MainWindow::onLoadPdfClicked);
    connect(ui->btnLinkMode,&QPushButton::toggled, this, &MainWindow::onToggleLinkMode);
    connect(ui->btnBackup,  &QPushButton::clicked, this, &MainWindow::onBackup);
    connect(ui->btnSsh,     &QPushButton::clicked, this, &MainWindow::onSsh);

    connect(m_syslog, &SyslogServer::messageReceived, this, &MainWindow::onSyslogMessage);
    connect(m_mndp,   &MndpScanner::deviceFound,      this, &MainWindow::onDeviceFound);

    connect(m_api, &MikroTikManager::connected,     this, &MainWindow::onApiConnected);
    connect(m_api, &MikroTikManager::errorOccurred, this, &MainWindow::onApiError);
    connect(m_api, &MikroTikManager::replyReceived, this, &MainWindow::onApiReply);
    connect(ui->btnSendCmd,     &QPushButton::clicked, this, &MainWindow::onSendCmd);
    connect(ui->editCmd,        &QLineEdit::returnPressed, this, &MainWindow::onSendCmd);
    connect(ui->btnConfigure,   &QPushButton::clicked, this, &MainWindow::onConfigureSelected);
    connect(ui->btnPlaceOnMap,  &QPushButton::clicked, this, &MainWindow::onPlaceSelectedOnMap);
    connect(ui->btnRotateLeft,  &QPushButton::clicked, this, [this](){ m_canvas->rotate(-90); });
    connect(ui->btnRotateRight, &QPushButton::clicked, this, [this](){ m_canvas->rotate(90);  });
    connect(ui->btnBlankMap,    &QPushButton::clicked, this, [this](){ m_canvas->newBlankMap(); });
    connect(ui->btnVectorize,   &QPushButton::clicked, this, [this](){
        if (m_canvas->currentPageImage().isNull()) {
            QMessageBox::information(this, tr("Векторизация"),
                tr("Сначала загрузите PDF — затем нажмите Векторизовать."));
            return;
        }
        statusBar()->showMessage(tr("Распознаём линии..."));
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_canvas->vectorizeCurrentPdf();
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage(tr("Готово: PDF превращён в редактируемые линии. "
                                    "Можете двигать, добавлять устройства, стирать ластиком."), 6000);
    });
    connect(ui->btnClearDraw,   &QPushButton::clicked, this, [this](){ m_canvas->clearStrokes(); });
    auto setMode = [this](DrawMode m){
        m_canvas->setDrawMode(m);
        ui->btnPen  ->setChecked(m == DrawMode::Pen);
        ui->btnRect ->setChecked(m == DrawMode::Rect);
        ui->btnErase->setChecked(m == DrawMode::Erase);
        if (m != DrawMode::None) {
            ui->btnLinkMode->setChecked(false);
            m_canvas->setLinkMode(false);
        }
        m_canvas->setFocus();
    };
    connect(ui->btnPen,   &QPushButton::clicked, this, [=](){ setMode(ui->btnPen->isChecked()   ? DrawMode::Pen   : DrawMode::None); });
    connect(ui->btnRect,  &QPushButton::clicked, this, [=](){ setMode(ui->btnRect->isChecked()  ? DrawMode::Rect  : DrawMode::None); });
    connect(ui->btnErase, &QPushButton::clicked, this, [=](){ setMode(ui->btnErase->isChecked() ? DrawMode::Erase : DrawMode::None); });
    setStatus(tr("Disconnected"), "#FF5252");
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
    mHelp->addAction(tr("User &Guide"), this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(tr("User Guide"));
        box.setTextFormat(Qt::RichText);
        box.setText(tr(
            "<h2>TikCanvas — краткое руководство</h2>"
            "<p><b>● Индикатор статуса</b> — цвет показывает состояние подключения:<br>"
            "&nbsp;&nbsp;🔴 красный — не подключено &nbsp; 🟡 жёлтый — подключение &nbsp; 🟢 зелёный — подключено</p>"
            "<h3>Левая панель</h3>"
            "<ul>"
            "<li><b>Host / User / Password</b> — параметры RouterOS API (порт 8728). "
            "Поле Host заполняется автоматически при обнаружении устройства по MNDP.</li>"
            "<li><b>Connect</b> — подключение к выбранному MikroTik по API. "
            "После успеха автоматически запросит /system/identity и /system/resource.</li>"
            "<li><b>Reboot</b> — отправляет /system/reboot подключённому устройству.</li>"
            "<li><b>SSH Terminal</b> — открывает внешнее окно ssh user@host.</li>"
            "<li><b>Backup Config</b> — выполняет /export, сохраняет .rsc локально.</li>"
            "<li><b>Discover (MNDP)</b> — broadcast-поиск MikroTik в сети (автоматически каждые 5 сек).</li>"
            "<li><b>Load PDF Map</b> — загрузить чертёж объекта как фон карты.</li>"
            "<li><b>Link Mode</b> — после нажатия два клика по устройствам на карте создают связь.</li>"
            "<li><b>Devices</b> — найденные устройства. Двойной клик ставит Host. "
            "Перетаскивание мышью добавляет устройство на карту.</li>"
            "</ul>"
            "<h3>Нижние вкладки</h3>"
            "<ul>"
            "<li><b>Syslog</b> — все сообщения от роутеров на UDP/514.</li>"
            "<li><b>Terminal</b> — произвольные команды RouterOS API "
            "(например: <code>/interface/print</code>, <code>/ip/address/print</code>).</li>"
            "<li><b>Device Info</b> — авто-вывод identity и resource при подключении.</li>"
            "</ul>"
            "<h3>Меню</h3>"
            "<ul>"
            "<li><b>File</b> — New / Open / Save проекта (*.tikcanvas хранит PDF + устройства + связи).</li>"
            "<li><b>View</b> — переключение тёмной и светлой темы.</li>"
            "<li><b>Tools</b> — Link Mode, Backup, SSH.</li>"
            "<li><b>Language</b> — Русский / English (применяется мгновенно).</li>"
            "</ul>"
            "<h3>Карта (центральная область)</h3>"
            "<p>Колесо мыши — масштаб. ЛКМ по пустому месту — панорамирование. "
            "ЛКМ по ноде — перетаскивание устройства. "
            "Двойной клик по ноде — открыть свойства.</p>"));
        box.exec();
    });
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
    setStatus(tr("Connecting to %1...").arg(host), "#FFB300");
    ui->btnConnect->setEnabled(false);
    m_api->connectToDevice(host, 8728, user, pass);
}

void MainWindow::setStatus(const QString &state, const QString &color)
{
    ui->lblStatus->setText("● " + state);
    ui->lblStatus->setStyleSheet(QStringLiteral("color:%1;padding:4px 8px;font-weight:bold;").arg(color));
    statusBar()->showMessage(state, 5000);
}

void MainWindow::onApiConnected()
{
    m_connected = true;
    ui->btnConnect->setEnabled(true);
    setStatus(tr("Connected to %1").arg(ui->editHost->text()), "#69F0AE");
    ui->infoView->clear();
    m_pendingInfoQueries = 2;
    m_api->sendCommand({"/system/identity/print"});
    m_api->sendCommand({"/system/resource/print"});

    auto pollNeighbors = [this]() {
        m_api->sendCommand({"/ip/neighbor/print"});
        m_api->sendCommand({"/caps-man/remote-cap/print"});
        m_api->sendCommand({"/caps-man/interface/print"});
        m_api->sendCommand({"/caps-man/registration-table/print"});
        m_api->sendCommand({"/ip/arp/print"});
        m_api->sendCommand({"/interface/wireless/registration-table/print"});
        m_api->sendCommand({"/log/print", "=count=30"});
    };
    pollNeighbors();
    if (!m_neighborPollTimer) {
        m_neighborPollTimer = new QTimer(this);
        m_neighborPollTimer->setInterval(15000);
        connect(m_neighborPollTimer, &QTimer::timeout, this, pollNeighbors);
    }
    m_neighborPollTimer->start();
}

void MainWindow::onApiError(const QString &msg)
{
    m_connected = false;
    ui->btnConnect->setEnabled(true);
    setStatus(tr("Error: %1").arg(msg), "#FF5252");
    QMessageBox::warning(this, tr("Connection error"), msg);
}

void MainWindow::onSendCmd()
{
    const QString text = ui->editCmd->text().trimmed();
    if (text.isEmpty()) return;
    if (!m_connected) {
        QMessageBox::information(this, tr("Terminal"), tr("Not connected. Press Connect first."));
        return;
    }
    const QStringList words = text.split(' ', Qt::SkipEmptyParts);
    ui->terminalView->appendPlainText("> " + text);
    m_api->sendCommand(words);
    ui->editCmd->clear();
}

void MainWindow::onApiReply(const QStringList &words)
{
    for (const auto &w : words) {
        if (!w.startsWith("=message=")) continue;
        const QString msg = w.mid(9);
        const auto d = ErrorAdvisor::advise(msg);
        if (d.isError) {
            const QString src = ui->editHost->text().trimmed();
            ui->logView->appendPlainText(QStringLiteral("[%1] [API:%2] ⚠ %3")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), src, msg));
            ui->logView->appendPlainText(QStringLiteral("    💡 %1 → %2").arg(d.title, d.fix));
        } else {
            const QString src = ui->editHost->text().trimmed();
            ui->logView->appendPlainText(QStringLiteral("[%1] [API:%2] %3")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), src, msg));
        }
    }
    bool hasLogTopics = false;
    QString logTime, logTopics, logMsg;
    for (const auto &w : words) {
        if (w == "!re") { logTime.clear(); logTopics.clear(); logMsg.clear(); }
        else if (w.startsWith("=time="))    logTime  = w.mid(6);
        else if (w.startsWith("=topics="))  { logTopics = w.mid(8); hasLogTopics = true; }
        else if (w.startsWith("=message=")) logMsg   = w.mid(9);
        if (w == "!done" && hasLogTopics && !logMsg.isEmpty()) {
            const auto d = ErrorAdvisor::advise(logMsg);
            const QString src = ui->editHost->text().trimmed();
            const QString icon = (logTopics.contains("error") || logTopics.contains("critical")) ? "❌"
                               : (logTopics.contains("warning")) ? "⚠" : "•";
            ui->logView->appendPlainText(QStringLiteral("[%1] [%2] %3 %4 [%5]")
                .arg(logTime, src, icon, logMsg, logTopics));
            if (d.isError)
                ui->logView->appendPlainText(QStringLiteral("    💡 %1 — %2").arg(d.title, d.fix));
        }
    }
    if (hasLogTopics) return;

    QString addr, mac, ident, board, version;
    bool isDiscovery = false;
    int  recordsFound = 0;

    auto flush = [&]() {
        if (mac.isEmpty()) return;
        if (addr.isEmpty()) addr = "0.0.0.0";
        onDeviceFound(mac, addr, ident, version, board, "");
        ++recordsFound;
        addr.clear(); mac.clear(); ident.clear(); board.clear(); version.clear();
    };

    for (const auto &w : words) {
        if (w == "!re") { flush(); continue; }
        if (w == "!done") { flush(); continue; }
        if (!w.startsWith("=")) continue;

        const int eq = w.indexOf('=', 1);
        if (eq < 0) continue;
        const QString k = w.mid(1, eq - 1);
        const QString v = w.mid(eq + 1);

        if (k == "address" || k == "remote-address" || k == "ip-address") {
            addr = v; isDiscovery = true;
        } else if (k == "mac-address" || k == "base-mac" || k == "radio-mac") {
            mac = v.toUpper(); isDiscovery = true;
        } else if (k == "identity" || k == "name" || k == "host-name") {
            if (ident.isEmpty()) ident = v;
        } else if (k == "board" || k == "common-name" || k == "board-name") {
            if (board.isEmpty()) board = v;
        } else if (k == "version") {
            version = v;
        }
    }

    if (isDiscovery) {
        statusBar()->showMessage(tr("Найдено устройств в сети: %1 (всего)").arg(ui->listDevices->count()), 3000);
        return;
    }

    QString text;
    for (const auto &w : words) text += w + "\n";
    if (m_pendingInfoQueries > 0) {
        ui->infoView->appendPlainText(text);
        ui->infoView->appendPlainText(QString(60, '-'));
        if (--m_pendingInfoQueries == 0) ui->bottomTabs->setCurrentWidget(ui->tabInfo);
    } else {
        ui->terminalView->appendPlainText(text);
    }
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

void MainWindow::onDeviceFound(const QString &mac, const QString &ip, const QString &identity,
                               const QString &version, const QString &board, const QString &)
{
    for (int i = 0; i < ui->listDevices->count(); ++i) {
        if (ui->listDevices->item(i)->data(Qt::UserRole + 1).toString() == mac)
            return;
    }
    const QString role = classifyBoard(board);
    const QString label = QStringLiteral("%1  %2  •  %3\n      %4  •  %5  %6")
        .arg(iconForRole(role),
             identity.isEmpty() ? "(без имени)" : identity,
             ip, role, board, version);
    auto *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, ip + "\t" + mac + "\t" + identity);
    item->setData(Qt::UserRole + 1, mac);
    item->setData(Qt::UserRole + 2, board);
    item->setData(Qt::UserRole + 3, identity);
    item->setToolTip(QStringLiteral("MAC: %1\nIP: %2\nИмя: %3\nМодель: %4\nВерсия: %5\nКатегория: %6")
                     .arg(mac, ip, identity, board, version, role));
    ui->listDevices->addItem(item);

    if (m_hostUserEdited) return;
    int score = 0;
    if (!identity.isEmpty()) score += 4;
    if (!board.isEmpty())    score += 2;
    if (!version.isEmpty())  score += 1;
    if (score > m_autoHostScore) {
        m_autoHostScore = score;
        ui->editHost->setText(ip);
        statusBar()->showMessage(
            tr("Auto-detected MikroTik: %1 (%2)").arg(ip, identity.isEmpty() ? board : identity),
            5000);
    }
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
    if (on) {
        ui->btnPen->setChecked(false);
        ui->btnRect->setChecked(false);
        ui->btnErase->setChecked(false);
        m_canvas->setDrawMode(DrawMode::None);
        ui->btnLinkMode->setStyleSheet("background:#FF4081;color:white;font-weight:bold;");
    } else {
        ui->btnLinkMode->setStyleSheet("");
    }
    m_canvas->setFocus();
    statusBar()->showMessage(on ? tr("Link mode ON — кликните по двум устройствам")
                                : tr("Link mode OFF"), 3000);
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

void MainWindow::onPlaceSelectedOnMap()
{
    auto *item = ui->listDevices->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Поставить на карту"),
            tr("Выберите устройство в списке слева."));
        return;
    }
    const QString payload = item->data(Qt::UserRole).toString();
    const auto parts = payload.split('\t');
    DeviceNode n;
    n.ip   = parts.value(0).trimmed();
    n.name = parts.value(2).trimmed();
    if (n.name.isEmpty()) n.name = n.ip;
    n.pos = QPointF();
    m_canvas->addDevice(n);
    statusBar()->showMessage(tr("Устройство %1 добавлено на карту. "
                                "Двигайте мышью — переместить, двойной клик — свойства.").arg(n.name), 5000);
}

QString MainWindow::classifyBoard(const QString &b)
{
    const QString s = b.toLower();
    if (s.contains("cap") || s.contains("wap") || s.contains("hap ac") || s.contains("audience"))
        return "Точка доступа";
    if (s.contains("crs") || s.contains("css") || s.contains("netfiber") || s.startsWith("hex s"))
        return "Коммутатор";
    if (s.startsWith("rb") || s.startsWith("hex") || s.contains("ccr") || s.contains("hap"))
        return "Маршрутизатор";
    return "Устройство";
}

QString MainWindow::iconForRole(const QString &role)
{
    if (role == "Точка доступа") return "📡";
    if (role == "Коммутатор")    return "🔀";
    if (role == "Маршрутизатор") return "🛜";
    return "🔧";
}

void MainWindow::onConfigureSelected()
{
    auto *item = ui->listDevices->currentItem();
    if (!item) {
        QMessageBox::information(this, tr("Configure"),
            tr("Сначала выберите устройство в списке слева."));
        return;
    }
    const QString ip       = item->data(Qt::UserRole).toString().section('\t', 0, 0);
    const QString board    = item->data(Qt::UserRole + 2).toString();
    const QString identity = item->data(Qt::UserRole + 3).toString();

    if (!m_connected || ui->editHost->text().trimmed() != ip) {
        const auto r = QMessageBox::question(this, tr("Подключение"),
            tr("Чтобы настроить устройство %1, нужно подключиться к нему.\nПодключиться сейчас?").arg(ip));
        if (r != QMessageBox::Yes) return;
        ui->editHost->setText(ip);
        onConnectClicked();
        QTimer::singleShot(1500, this, [this, ip, identity, board]() {
            if (m_connected) {
                auto *dlg = new DeviceConfigDialog(m_api, ip, identity, board, this);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->show();
            }
        });
        return;
    }
    auto *dlg = new DeviceConfigDialog(m_api, ip, identity, board, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}
