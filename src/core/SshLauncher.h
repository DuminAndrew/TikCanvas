#pragma once
#include <QString>

class SshLauncher
{
public:
    static bool launch(const QString &host, const QString &user);
};
