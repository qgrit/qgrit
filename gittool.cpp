#include <QDebug>
#include <QSettings>

#include "gittool.h"

GitTool::GitTool()
{
}

bool GitTool::startupFindGit()
{
    m_GitCommand = QLatin1String("git");

    //first try with current PATH
    bool started = gitExecuteVersion();
    if(started)
        return true;

#ifdef _WIN32
    //try value set by msysGit setup
    QSettings settings(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1"), QSettings::NativeFormat);
    QVariant value = settings.value(QLatin1String("InstallLocation"));
    if(value.isValid())
    {
        m_GitCommand = value.toString() + QLatin1String("bin\\git.exe");
        started = gitExecuteVersion();
        if(started)
            return true;
    }

    //just guess and try default install path of msysGit developer setup
    m_GitCommand = QLatin1String("C:\\msysgit\\bin\\git.exe");
    started = gitExecuteVersion();
    if(started)
        return true;
#endif
    return false;
}

//returns -1 when program could not be started
//returns >= 0 (exit code of program)
int GitTool::gitExecuteStdOutStdErr(const QStringList &commandline, QByteArray &stdOut, QByteArray &stdErr)
{
    QProcess p;
    p.start(m_GitCommand, commandline);
    if(!p.waitForStarted(-1))
    {
        return -1;
    }

    if(!p.waitForFinished(-1))
        return -1;

    stdOut.clear();
    stdErr.clear();
    //FIXME can a deadlock happen when writing huge amount of data to stdout and stderr?
    stdOut.append(p.readAllStandardOutput());
    stdErr.append(p.readAllStandardError());

    return p.exitCode();
}

//many git commands add a trailing \n to output
void GitTool::stripTrailingNewline(QByteArray &bytearray)
{
    if(!bytearray.isEmpty() && bytearray.endsWith('\n'))
        bytearray.resize(bytearray.size() - 1);
}

bool GitTool::gitExecuteVersion()
{
    QByteArray stdOut;
    int exitCode = gitExecuteStdOutStdErr(QStringList() << QLatin1String("--version"), stdOut);

    if(exitCode != 0)
        return false;

    stripTrailingNewline(stdOut);
    QString str = QString::fromLatin1(stdOut.data(), stdOut.length());
    //git version 1.7.6.rc2.321.g21953.dirty
    //git version 1.7.7.617.gb55cd.dirty
    if(!str.startsWith(QLatin1String("git version ")))
    {
        qDebug() << "could not parse git version" << str;
        return false;
    }

    m_Version.version = str.mid(12);
    QStringList sl = m_Version.version.split(QChar(QLatin1Char('.')));

    if(sl.length() >= 1)
        m_Version.supermajor = sl[0].toInt();
    if(sl.length() >= 2)
        m_Version.major = sl[1].toInt();
    if(sl.length() >= 3)
        m_Version.minor = sl[2].toInt();
    return true;
}

GitVersion GitTool::gitVersion()
{
    return m_Version;
}

QString GitTool::m_GitCommand;
GitVersion GitTool::m_Version;
QByteArray GitTool::m_defaultArray;
