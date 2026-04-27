#pragma once
#include <QString>

struct Diagnosis {
    bool    isError {false};
    QString severity;
    QString title;
    QString cause;
    QString fix;
};

class ErrorAdvisor
{
public:
    static Diagnosis advise(const QString &rawMessage);
};
