#include "I18n.h"
#include <QCoreApplication>

I18n &I18n::instance()
{
    static I18n inst;
    return inst;
}

bool I18n::setLanguage(const QString &code)
{
    QCoreApplication::removeTranslator(&m_translator);
    const QString resPath = QStringLiteral(":/i18n/tikcanvas_%1.qm").arg(code);
    if (m_translator.load(resPath)) {
        QCoreApplication::installTranslator(&m_translator);
    }
    m_current = code;
    emit languageChanged(code);
    return true;
}
