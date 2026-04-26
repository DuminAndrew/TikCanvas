#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QTextStream>
#include "MainWindow.h"
#include "core/I18n.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("TikCanvas");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("TikCanvas");
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    I18n::instance().setLanguage("ru");

    MainWindow w;
    w.show();
    return app.exec();
}
