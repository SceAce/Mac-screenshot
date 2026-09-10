#pragma once

#include <QString>
#include <QStringList>

class QProcess;

namespace markshot {

QString commandShellProgram();
QStringList commandShellArguments(const QString &commandLine);
void setShellCommand(QProcess *process, const QString &commandLine);

}  // namespace markshot
