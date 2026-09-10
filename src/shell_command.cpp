#include "shell_command.h"

#include <QProcess>
#include <QProcessEnvironment>

namespace markshot {

QString commandShellProgram()
{
#if defined(Q_OS_WIN)
    const QString comspec = QProcessEnvironment::systemEnvironment().value(QStringLiteral("COMSPEC"));
    return comspec.isEmpty() ? QStringLiteral("cmd.exe") : comspec;
#else
    QString shell = QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL"),
                                                                   QStringLiteral("/bin/sh"));
    return shell.isEmpty() ? QStringLiteral("/bin/sh") : shell;
#endif
}

QStringList commandShellArguments(const QString &commandLine)
{
#if defined(Q_OS_WIN)
    return {QStringLiteral("/D"), QStringLiteral("/V:OFF"), QStringLiteral("/S"), QStringLiteral("/C"), commandLine};
#else
    return {QStringLiteral("-c"), commandLine};
#endif
}

void setShellCommand(QProcess *process, const QString &commandLine)
{
    if (!process) {
        return;
    }
    process->setProgram(commandShellProgram());
    process->setArguments(commandShellArguments(commandLine));
}

}  // namespace markshot
