#include <QMessageBox>
#include <QProcess>
#include <QtCore>
#include <QDebug>

#include "gittool.h"
#include "configdialog.h"
#include "ui_configdialog.h"

static QString getRebaseiCommand()
{
    QString s = QApplication::applicationFilePath() + QLatin1String(" --rebasei");
#ifdef _WIN32
    //convert to a path that can be used by MINGW
    //already uses '/' as separator
    s.replace(QLatin1String(":/"), QLatin1String("/"));
    s = QLatin1String("/") + s;
#endif
    return s;
}

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::ConfigDialog)
{
    m_ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    updateGitToolStatus();
}

ConfigDialog::~ConfigDialog()
{
    delete m_ui;
}

void ConfigDialog::gitConfigSetClearSequenceEditor(bool set)
{
    QByteArray stdOut;
    QStringList sl;
    if(set)
        sl << QLatin1String("config") << QLatin1String("--global") << QLatin1String("sequence.editor") << getRebaseiCommand();
    else
        sl << QLatin1String("config") << QLatin1String("--global") << QLatin1String("--unset") << QLatin1String("sequence.editor");
    int exitCode = GitTool::gitExecuteStdOutStdErr(sl, stdOut);

    if(exitCode != 0)
        QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Could not set qgrit as rebase editor"));
    updateGitToolStatus();
}

QString ConfigDialog::gitConfigGetSequenceEditor()
{
    QByteArray stdOut;
    QStringList sl;

    sl << QLatin1String("config") << QLatin1String("--global") << QLatin1String("sequence.editor");
    int exitCode = GitTool::gitExecuteStdOutStdErr(sl, stdOut);
    if(exitCode == 0)
    {
        GitTool::stripTrailingNewline(stdOut);
        return QString::fromUtf8(stdOut.constData(), stdOut.length());
    }

    return QString();
}

static int compareVersions(const GitVersion &version, int supermajor, int major, int minor)
{
    if((version.supermajor - supermajor) != 0)
        return version.supermajor - supermajor;
    if((version.major - major) != 0)
        return version.major - major;
    if((version.minor - minor) != 0)
        return version.minor - minor;
    return 0;
}

void ConfigDialog::updateGitToolStatus()
{
    GitVersion ver = GitTool::gitVersion();

    if(compareVersions(ver, 1, 7, 8) < 0)
    {
        m_ui->labelStatus->setText(tr("Git versions before 1.7.8 are not supported by qgrit."));
        m_ui->pushButtonInstall->setEnabled(false);
        m_ui->pushButtonUninstall->setEnabled(false);
        return;
    }

    QString oldTool = gitConfigGetSequenceEditor();
    QString newTool = getRebaseiCommand();
    if(newTool == oldTool)
    {
        m_ui->labelStatus->setText(tr("QGrit is set as rebase editor"));
        m_ui->pushButtonInstall->setEnabled(false);
        m_ui->pushButtonUninstall->setEnabled(true);
    }
    else if(oldTool.isNull())
    {
        m_ui->labelStatus->setText(tr("No tool is set as rebase editor"));
        m_ui->pushButtonInstall->setEnabled(true);
        m_ui->pushButtonUninstall->setEnabled(false);
    }
    else
    {
        m_ui->labelStatus->setText(tr("Another tool is set as rebase editor"));
        m_ui->pushButtonInstall->setEnabled(true);
        m_ui->pushButtonUninstall->setEnabled(true);
    }
}

void ConfigDialog::on_pushButtonExit_clicked()
{
    this->close();
}

void ConfigDialog::on_pushButtonInstall_clicked()
{
    gitConfigSetClearSequenceEditor(true);
}

void ConfigDialog::on_pushButtonUninstall_clicked()
{
    gitConfigSetClearSequenceEditor(false);
}
