#include "DeviceConfigDialog.h"
#include "core/MikroTikManager.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>

DeviceConfigDialog::DeviceConfigDialog(MikroTikManager *api,
                                       const QString &host,
                                       const QString &identity,
                                       const QString &board,
                                       QWidget *parent)
    : QDialog(parent), m_api(api), m_host(host)
{
    setWindowTitle(tr("Настройка устройства — %1").arg(host));
    resize(720, 600);

    auto *root = new QVBoxLayout(this);
    auto *header = new QLabel(QStringLiteral(
        "<b style='color:#4FC3F7;font-size:16px;'>%1</b> &nbsp; "
        "<span style='color:#A6ADC8;'>%2 &nbsp;·&nbsp; %3</span>")
        .arg(identity.isEmpty() ? "MikroTik" : identity, host, board));
    root->addWidget(header);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs, 1);

    // ---------- Identity tab ----------
    auto *tabId = new QWidget;
    auto *fId = new QFormLayout(tabId);
    m_editName = new QLineEdit(identity);
    fId->addRow(tr("Имя устройства (identity):"), m_editName);
    auto *btnId = new QPushButton(tr("Применить имя"));
    fId->addRow("", btnId);
    fId->addRow(new QLabel(tr("<i>Поможет различать роутеры в сети.</i>")));
    connect(btnId, &QPushButton::clicked, this, &DeviceConfigDialog::applyIdentity);
    m_tabs->addTab(tabId, tr("📝 Имя"));

    // ---------- Network tab ----------
    auto *tabNet = new QWidget;
    auto *fNet = new QFormLayout(tabNet);
    m_cbIface = new QComboBox; m_cbIface->setEditable(true);
    m_cbIface->addItems({"ether1", "ether2", "ether3", "bridge"});
    m_editIp  = new QLineEdit; m_editIp->setPlaceholderText("192.168.88.1/24");
    m_editGw  = new QLineEdit; m_editGw->setPlaceholderText("192.168.88.254");
    m_editDns = new QLineEdit; m_editDns->setPlaceholderText("8.8.8.8,1.1.1.1");
    fNet->addRow(tr("Интерфейс:"),  m_cbIface);
    fNet->addRow(tr("IP / маска:"), m_editIp);
    fNet->addRow(tr("Шлюз:"),       m_editGw);
    fNet->addRow(tr("DNS:"),        m_editDns);
    auto *btnNet = new QPushButton(tr("Применить настройки сети"));
    fNet->addRow("", btnNet);
    fNet->addRow(new QLabel(tr(
        "<i>Назначит статический IP, шлюз и DNS. Уже существующие записи будут обновлены.</i>")));
    connect(btnNet, &QPushButton::clicked, this, &DeviceConfigDialog::applyNetwork);
    m_tabs->addTab(tabNet, tr("🌐 Сеть"));

    // ---------- DHCP client tab ----------
    auto *tabDhcp = new QWidget;
    auto *fDhcp = new QFormLayout(tabDhcp);
    m_cbDhcpIface = new QComboBox; m_cbDhcpIface->setEditable(true);
    m_cbDhcpIface->addItems({"ether1", "ether2"});
    fDhcp->addRow(tr("WAN-интерфейс:"), m_cbDhcpIface);
    auto *btnDhcp = new QPushButton(tr("Получать интернет по DHCP"));
    fDhcp->addRow("", btnDhcp);
    fDhcp->addRow(new QLabel(tr(
        "<i>Самый частый сценарий: провайдер выдаёт IP автоматически.<br>"
        "Включите если кабель провайдера воткнут в выбранный порт.</i>")));
    connect(btnDhcp, &QPushButton::clicked, this, &DeviceConfigDialog::applyDhcpClient);
    m_tabs->addTab(tabDhcp, tr("🌍 Интернет"));

    // ---------- WiFi tab ----------
    auto *tabWifi = new QWidget;
    auto *fWifi = new QFormLayout(tabWifi);
    m_cbWifiIface = new QComboBox; m_cbWifiIface->setEditable(true);
    m_cbWifiIface->addItems({"wlan1", "wlan2", "wifi1", "wifi2"});
    m_editSsid     = new QLineEdit; m_editSsid->setPlaceholderText("MyHomeWiFi");
    m_editWifiPass = new QLineEdit; m_editWifiPass->setPlaceholderText(tr("минимум 8 символов"));
    m_editWifiPass->setEchoMode(QLineEdit::Password);
    fWifi->addRow(tr("Wi-Fi интерфейс:"), m_cbWifiIface);
    fWifi->addRow(tr("Имя сети (SSID):"), m_editSsid);
    fWifi->addRow(tr("Пароль (WPA2):"),   m_editWifiPass);
    auto *btnWifi = new QPushButton(tr("Применить Wi-Fi"));
    fWifi->addRow("", btnWifi);
    fWifi->addRow(new QLabel(tr(
        "<i>Создаст профиль безопасности WPA2-PSK и применит его к интерфейсу.<br>"
        "Если на устройстве пакет wifi (ROS 7) — может потребоваться wifi1.</i>")));
    connect(btnWifi, &QPushButton::clicked, this, &DeviceConfigDialog::applyWifi);
    m_tabs->addTab(tabWifi, tr("📶 Wi-Fi"));

    // ---------- Roaming tab ----------
    auto *tabR = new QWidget;
    auto *fR = new QFormLayout(tabR);
    m_editConnectMin = new QLineEdit("-75");
    m_editKickBelow  = new QLineEdit("-85");
    m_editStickyTime = new QLineEdit("10");
    fR->addRow(tr("Минимальный сигнал для подключения (dBm):"), m_editConnectMin);
    fR->addRow(tr("Отключать клиента слабее (dBm):"), m_editKickBelow);
    fR->addRow(tr("Sticky-таймаут перед переключением (сек):"), m_editStickyTime);
    auto *btnR = new QPushButton(tr("Применить роуминг ко всем CAPsMAN-точкам"));
    fR->addRow("", btnR);
    fR->addRow(new QLabel(tr(
        "<i>Эти настройки заставят клиентов автоматически "
        "переключаться на ближайшую точку:<br>"
        "• <b>−75 dBm</b> — клиент с более слабым сигналом не будет подключаться<br>"
        "• <b>−85 dBm</b> — если уже подключён и сигнал упал ниже, его принудительно отключат "
        "(телефон/ноут переподключится к ближайшей точке)<br>"
        "• Записи добавляются в /caps-man/access-list, применяются ко всем точкам.</i>")));
    connect(btnR, &QPushButton::clicked, this, &DeviceConfigDialog::applyRoaming);
    m_tabs->addTab(tabR, tr("📡 Роуминг"));

    // ---------- Maintenance tab ----------
    auto *tabM = new QWidget;
    auto *vM = new QVBoxLayout(tabM);
    auto *btnReboot = new QPushButton(tr("🔄 Перезагрузить роутер"));
    btnReboot->setStyleSheet("background:#FFB300;color:#11111B;font-weight:bold;padding:14px;");
    auto *btnReset  = new QPushButton(tr("⚠ Сброс к заводским настройкам"));
    btnReset->setStyleSheet("background:#FF5252;color:#fff;font-weight:bold;padding:14px;");
    vM->addWidget(btnReboot);
    vM->addWidget(btnReset);
    vM->addWidget(new QLabel(tr(
        "<p><b>Перезагрузка</b> — мягкий рестарт RouterOS, конфигурация сохранится.</p>"
        "<p><b>Сброс</b> — стирает всю конфигурацию. Используйте если запутались "
        "или хотите начать настройку с нуля.</p>")));
    vM->addStretch();
    connect(btnReboot, &QPushButton::clicked, this, &DeviceConfigDialog::doReboot);
    connect(btnReset,  &QPushButton::clicked, this, &DeviceConfigDialog::doFactoryReset);
    m_tabs->addTab(tabM, tr("⚙ Обслуживание"));

    // ---------- Log ----------
    auto *gLog = new QGroupBox(tr("Журнал команд"));
    auto *vLog = new QVBoxLayout(gLog);
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(140);
    m_log->setObjectName("terminalView");
    vLog->addWidget(m_log);
    root->addWidget(gLog);

    auto *btnClose = new QPushButton(tr("Закрыть"));
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    root->addWidget(btnClose);

    connect(m_api, &MikroTikManager::replyReceived, this, &DeviceConfigDialog::onReply);
}

void DeviceConfigDialog::log(const QString &line)
{
    m_log->appendPlainText(QDateTime::currentDateTime().toString("hh:mm:ss") + "  " + line);
}

void DeviceConfigDialog::onReply(const QStringList &words)
{
    if (!isVisible()) return;
    for (const auto &w : words)
        if (w == "!done" || w == "!fatal" || w == "!trap" || w.startsWith("=message="))
            log(w);
}

void DeviceConfigDialog::applyIdentity()
{
    const QString name = m_editName->text().trimmed();
    if (name.isEmpty()) return;
    log("> /system/identity/set name=" + name);
    m_api->sendCommand({"/system/identity/set", "=name=" + name});
}

void DeviceConfigDialog::applyNetwork()
{
    const QString iface = m_cbIface->currentText().trimmed();
    const QString ip    = m_editIp->text().trimmed();
    const QString gw    = m_editGw->text().trimmed();
    const QString dns   = m_editDns->text().trimmed();
    if (!ip.isEmpty()) {
        log("> /ip/address/add address=" + ip + " interface=" + iface);
        m_api->sendCommand({"/ip/address/add", "=address=" + ip, "=interface=" + iface});
    }
    if (!gw.isEmpty()) {
        log("> /ip/route/add gateway=" + gw);
        m_api->sendCommand({"/ip/route/add", "=dst-address=0.0.0.0/0", "=gateway=" + gw});
    }
    if (!dns.isEmpty()) {
        log("> /ip/dns/set servers=" + dns);
        m_api->sendCommand({"/ip/dns/set", "=servers=" + dns});
    }
}

void DeviceConfigDialog::applyDhcpClient()
{
    const QString iface = m_cbDhcpIface->currentText().trimmed();
    if (iface.isEmpty()) return;
    log("> /ip/dhcp-client/add interface=" + iface);
    m_api->sendCommand({"/ip/dhcp-client/add",
                        "=interface=" + iface,
                        "=disabled=no",
                        "=add-default-route=yes",
                        "=use-peer-dns=yes"});
}

void DeviceConfigDialog::applyWifi()
{
    const QString iface = m_cbWifiIface->currentText().trimmed();
    const QString ssid  = m_editSsid->text().trimmed();
    const QString pass  = m_editWifiPass->text();
    if (ssid.isEmpty() || iface.isEmpty()) {
        QMessageBox::warning(this, tr("Wi-Fi"), tr("Укажите SSID и интерфейс"));
        return;
    }
    if (!pass.isEmpty() && pass.size() < 8) {
        QMessageBox::warning(this, tr("Wi-Fi"), tr("Пароль должен быть не короче 8 символов"));
        return;
    }
    const QString prof = "tikcanvas-" + ssid;
    log("> /interface/wireless/security-profiles/add name=" + prof);
    m_api->sendCommand({"/interface/wireless/security-profiles/add",
                        "=name=" + prof, "=mode=dynamic-keys",
                        "=authentication-types=wpa2-psk",
                        "=wpa2-pre-shared-key=" + pass});
    log("> /interface/wireless/set " + iface + " ssid=" + ssid);
    m_api->sendCommand({"/interface/wireless/set",
                        "=numbers=" + iface,
                        "=ssid=" + ssid,
                        "=security-profile=" + prof,
                        "=disabled=no"});
}

void DeviceConfigDialog::applyRoaming()
{
    bool okMin, okKick, okSticky;
    int connectMin = m_editConnectMin->text().toInt(&okMin);
    int kickBelow  = m_editKickBelow->text().toInt(&okKick);
    int sticky     = m_editStickyTime->text().toInt(&okSticky);
    if (!okMin || !okKick || !okSticky) {
        QMessageBox::warning(this, tr("Роуминг"), tr("Введите числовые значения"));
        return;
    }
    log(QStringLiteral("> /caps-man/access-list/add signal-range=%1..120 action=accept").arg(connectMin));
    m_api->sendCommand({"/caps-man/access-list/add",
                        QStringLiteral("=signal-range=%1..120").arg(connectMin),
                        "=action=accept",
                        "=comment=tikcanvas-min-signal"});
    log(QStringLiteral("> /caps-man/access-list/add signal-range=-120..%1 action=reject").arg(kickBelow));
    m_api->sendCommand({"/caps-man/access-list/add",
                        QStringLiteral("=signal-range=-120..%1").arg(kickBelow),
                        "=action=reject",
                        "=comment=tikcanvas-kick-below"});
    log(QStringLiteral("> /caps-man/access-list/add ap-tx-limit/sticky=%1s").arg(sticky));
    m_api->sendCommand({"/caps-man/access-list/add",
                        QStringLiteral("=time=%1s").arg(sticky),
                        "=action=accept",
                        "=comment=tikcanvas-sticky-window"});
}

void DeviceConfigDialog::doReboot()
{
    if (QMessageBox::question(this, tr("Перезагрузка"),
        tr("Точно перезагрузить %1?").arg(m_host)) == QMessageBox::Yes) {
        log("> /system/reboot");
        m_api->sendCommand({"/system/reboot"});
    }
}

void DeviceConfigDialog::doFactoryReset()
{
    const auto r = QMessageBox::warning(this, tr("Сброс"),
        tr("Это сотрёт ВСЮ конфигурацию устройства %1.\nПродолжить?").arg(m_host),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (r == QMessageBox::Yes) {
        log("> /system/reset-configuration no-defaults=yes skip-backup=yes");
        m_api->sendCommand({"/system/reset-configuration",
                            "=no-defaults=yes",
                            "=skip-backup=yes"});
    }
}
