# TikCanvas

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6-41CD52.svg)
![CMake](https://img.shields.io/badge/CMake-3.21%2B-064F8C.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**TikCanvas** is a high-performance native desktop tool for MikroTik network operators.
Visualise infrastructure on top of PDF floor-plans, control devices through the
RouterOS API, watch live Syslog and discover routers via MNDP — all in a single,
60-FPS-smooth Qt interface.

## Features

- **PDF Map Canvas** — high-DPI rendering via Poppler-Qt6, pan & zoom.
- **MikroTik RouterOS API** — pure C++ TCP client (no external deps), login + commands (e.g. `/system/reboot`).
- **Syslog Server** — UDP/514 collector running in its own `QThread`.
- **MNDP Auto-discovery** — broadcast scanner for MikroTik devices.
- **Modern Dark UI** — defined in `.ui` files, themed with QSS.
- **Asynchronous everything** — networking on `QThread`, PDF rendering via `QtConcurrent`.

## Architecture

```
TikCanvas
├── MainWindow        Composes the UI defined in MainWindow.ui
├── widgets/
│   └── MapCanvas     Custom QWidget, paints PDF + overlays
├── core/
│   ├── MikroTikManager    QThread-based RouterOS API client
│   ├── SyslogServer       QThread + QUdpSocket :514
│   ├── MndpScanner        QThread + QUdpSocket :5678
│   └── PdfRenderer        Poppler-Qt6 wrapper
└── resources/        QSS theme, icons (.qrc)
```

## Build

### Dependencies
- CMake >= 3.21
- Qt 6 (Core, Gui, Widgets, Network, Concurrent)
- Poppler-Qt6

#### Linux (Debian/Ubuntu)
```bash
sudo apt install cmake qt6-base-dev qt6-tools-dev libpoppler-qt6-dev
```

#### macOS (Homebrew)
```bash
brew install cmake qt poppler
```

#### Windows (vcpkg)
```powershell
vcpkg install qtbase:x64-windows poppler[qt]:x64-windows
```

### Compile
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/src/TikCanvas
```

## Screenshots

> _placeholder_ — main canvas
![Canvas](docs/screenshot-canvas.png)

> _placeholder_ — discovery & syslog
![Discovery](docs/screenshot-discovery.png)

## License
MIT
