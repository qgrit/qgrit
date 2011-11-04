#ifndef GITTOOL_H
#define GITTOOL_H

#include <QString>
#include <QStringList>
#include <QProcessEnvironment>

class GitVersion
{
public:
    QString version;
    int supermajor;
    int major;
    int minor;
};


class GitTool
{
private:
    GitTool();

public:
    static bool startupFindGit();
    static int gitExecuteStdOutStdErr(const QStringList &commandline, QByteArray &stdOut = m_defaultArray, QByteArray &stdErr = m_defaultArray);
    static void stripTrailingNewline(QByteArray &bytearray);
    static GitVersion gitVersion();

private:
    static bool gitExecuteVersion();

    static GitVersion m_Version;
    static QString m_GitCommand;
    static QByteArray m_defaultArray;
};

#endif // GITTOOL_H
