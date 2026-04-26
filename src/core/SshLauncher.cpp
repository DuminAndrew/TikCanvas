#include "SshLauncher.h"
#include <QProcess>

bool SshLauncher::launch(const QString &host, const QString &user)
{
    const QString target = user.isEmpty() ? host : (user + "@" + host);
#ifdef Q_OS_WIN
    return QProcess::startDetached("cmd",
        {"/c", "start", "TikCanvas SSH", "ssh", target});
#elif defined(Q_OS_MACOS)
    return QProcess::startDetached("osascript",
        {"-e", QString("tell app \"Terminal\" to do script \"ssh %1\"").arg(target)});
#else
    if (QProcess::startDetached("x-terminal-emulator", {"-e", "ssh", target})) return true;
    if (QProcess::startDetached("gnome-terminal", {"--", "ssh", target})) return true;
    return QProcess::startDetached("xterm", {"-e", "ssh " + target});
#endif
}
