#pragma once
#include <QObject>
#include <QTranslator>
#include <QString>

class I18n : public QObject
{
    Q_OBJECT
public:
    static I18n &instance();
    bool setLanguage(const QString &code);
    QString currentLanguage() const { return m_current; }

signals:
    void languageChanged(const QString &code);

private:
    I18n() = default;
    QTranslator m_translator;
    QString     m_current {"ru"};
};
