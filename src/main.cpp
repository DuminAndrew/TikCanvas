#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include "MainWindow.h"

static QString loadStyleSheet()
{
    QFile f(QStringLiteral(":/styles/dark.qss"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QTextStream(&f).readAll();
}

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QApplication::setApplicationName("TikCanvas");
    QApplication::setOrganizationName("TikCanvas");
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    const QString qss = loadStyleSheet();
    if (!qss.isEmpty())
        app.setStyleSheet(qss);

    MainWindow w;
    w.show();
    return app.exec();
}
