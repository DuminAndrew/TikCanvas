<div align="center">

<img src="icon.svg" width="120" alt="TikCanvas"/>

# TikCanvas

**Высокопроизводительный десктопный инструмент для сетевых инженеров MikroTik**

PDF-карты объектов · RouterOS API · Syslog · Автопоиск устройств — в едином 60 FPS интерфейсе.

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C?logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

</div>

---

## О проекте

**TikCanvas** — это нативное Qt6/C++20 приложение, которое объединяет визуальную карту инфраструктуры (PDF-чертежи здания, объекта, площадки) с инструментами оперативного управления MikroTik-устройствами. Без браузерных оверхедов, без Electron — чистый компилируемый C++ с асинхронной архитектурой на `QThread` и `QtConcurrent`.

> Один экран. Карта объекта. Все ваши роутеры. Реальное время.

## Возможности

- 🗺️ **PDF Map Canvas** — отрисовка чертежей через **Poppler-Qt6**, плавный pan & zoom, рендер в фоновом потоке (`QtConcurrent`)
- 📡 **RouterOS API клиент** — собственная реализация поверх `QTcpSocket`, без внешних зависимостей: подключение, аутентификация, отправка команд (например, `/system/reboot`)
- 📥 **Syslog Server** — UDP-коллектор на порту 514 в выделенном `QThread` с лайв-выводом
- 🛰️ **MNDP Auto-discovery** — broadcast-сканер устройств MikroTik в локальной сети
- 🎨 **Современный тёмный UI** — все формы в `.ui` (Qt Designer), темизация через QSS
- ⚡ **Асинхронность** — UI всегда отзывчив на 60 FPS, сеть и тяжёлые операции вне главного потока
- 🌐 **Кросс-платформенность** — Windows, Linux, macOS

## Архитектура

```
TikCanvas
├── MainWindow              ← композиция UI (MainWindow.ui)
│
├── widgets/
│   └── MapCanvas           ← QWidget: отрисовка PDF + оверлей устройств
│
├── core/
│   ├── MikroTikManager     ← QThread + QTcpSocket — RouterOS API
│   ├── SyslogServer        ← QThread + QUdpSocket :514
│   ├── MndpScanner         ← QThread + QUdpSocket :5678
│   └── PdfRenderer         ← Poppler-Qt6 (рендер в QtConcurrent)
│
└── resources/
    ├── dark.qss            ← тема оформления
    └── resources.qrc       ← qresource манифест
```

Каждый сетевой компонент живёт в собственном потоке через паттерн **Worker + moveToThread**, общение — через сигналы Qt. Это гарантирует, что главный поток UI никогда не блокируется.

## Требования

### Для пользователей
- Windows 10/11, Linux или macOS 11+
- 100 МБ свободного места
- Сеть с доступом до управляемых устройств MikroTik

### Для разработки
- **C++20** компилятор (MSVC 2022 / GCC 11+ / Clang 14+)
- **Qt 6.5+** — модули Core, Gui, Widgets, Network, Concurrent
- **CMake 3.21+**
- **Poppler-Qt6** — для рендеринга PDF-чертежей

## Установка зависимостей

### Windows (vcpkg)
```powershell
vcpkg install qtbase:x64-windows poppler[qt]:x64-windows
```

### Linux (Debian / Ubuntu)
```bash
sudo apt install cmake build-essential qt6-base-dev qt6-tools-dev libpoppler-qt6-dev
```

### macOS (Homebrew)
```bash
brew install cmake qt poppler
```

## Сборка из исходного кода

```bash
git clone https://github.com/DuminAndrew/TikCanvas.git
cd TikCanvas

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Готовый бинарник появится в `build/src/TikCanvas` (или `build/src/Release/TikCanvas.exe` на Windows).

> 💡 Если Poppler-Qt6 не найден, проект всё равно соберётся — рендер PDF будет заглушкой, остальной функционал работает.

## Использование

1. **Запустите приложение** — `TikCanvas`
2. **Загрузите PDF-карту объекта** — кнопка `Load PDF Map`
3. **Найдите устройства** — `Discover (MNDP)` отправит broadcast
4. **Подключитесь** — введите Host / User / Password и нажмите `Connect`
5. **Управляйте** — кнопка `Reboot` отправит `/system/reboot` через RouterOS API
6. **Логи в реальном времени** — Syslog от роутеров отображается внизу экрана

## Структура проекта

```
TikCanvas/
├── CMakeLists.txt
├── README.md
├── icon.svg
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── MainWindow.{h,cpp,ui}
│   ├── widgets/
│   │   └── MapCanvas.{h,cpp}
│   └── core/
│       ├── MikroTikManager.{h,cpp}
│       ├── SyslogServer.{h,cpp}
│       ├── MndpScanner.{h,cpp}
│       └── PdfRenderer.{h,cpp}
└── resources/
    ├── resources.qrc
    └── dark.qss
```

## Скриншоты

> _placeholder_ — главная карта с устройствами

> _placeholder_ — панель discovery + syslog

## Дорожная карта

- [ ] Рисование линков между устройствами поверх карты
- [ ] Сохранение/загрузка проекта (*.tikcanvas)
- [ ] SSH-терминал внутри приложения
- [ ] Backup-менеджер конфигов
- [ ] Тёмная/светлая темы

## Лицензия

Распространяется под лицензией **MIT**. См. файл [LICENSE](LICENSE).

## Автор

### Андрей Думин

- GitHub: [@DuminAndrew](https://github.com/DuminAndrew)

## Вклад в проект

Pull-request'ы приветствуются. Для крупных изменений сначала откройте issue для обсуждения.

1. Сделайте форк проекта
2. Создайте ветку (`git checkout -b feature/awesome`)
3. Закоммитьте изменения (`git commit -m 'Add awesome feature'`)
4. Запушьте (`git push origin feature/awesome`)
5. Откройте Pull Request
