#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QTextStream>

#include "gittool.h"
#include "configdialog.h"
#include "rebasedialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/qgrit16.png"));
    icon.addFile(QString::fromUtf8(":/icons/qgrit32.png"));
    icon.addFile(QString::fromUtf8(":/icons/qgrit48.png"));
    QApplication::setWindowIcon(icon);

    const QStringList &arguments = a.arguments();
    QString filename;
    bool rebasei = false;

    for(int i = 1; i < arguments.length(); i++)
    {
        if(arguments.at(i) == QLatin1String("--rebasei"))
            rebasei = true;
        else if(i == arguments.length() - 1)
            filename = arguments.at(i);
    }

    if(rebasei)
    {
        if(filename.endsWith(QLatin1String("git-rebase-todo"))) //usually .git/rebase-merge/git-rebase-todo
        {
            bool found = GitTool::startupFindGit();
            if(!found)
            {
                QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Could not find installed git version."));
                return 1;
            }
            RebaseDialog d;
            d.readFile(filename);
            d.fillList();
            d.show();
            return a.exec();
        }
        else
        {
            //we were mistakenly invoked as a text editor
            //exit with a error
            //we could start editors like kwrite/kate/gvim/gedit which do not depend on being executed at a tty
            QMessageBox::critical(0, QObject::tr("Error"),
                                  QObject::tr("qgrit was invoked as a text editor for \"%1\"\n"
                                              "This is not supported, please fix your git configuration to use qgrit only as rebase -i editor.").replace(QLatin1String("%1"), filename));
            return 1;
        }
    }
    else if(arguments.length() == 1)
    {
        //no arguments supplied, show a configuration dialog
        bool found = GitTool::startupFindGit();
        if(!found)
        {
            QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("Could not find installed git version."));
            return 1;
        }
        ConfigDialog d;
        d.show();
        return a.exec();
    }
    else
    {
        //invoked without --rebasei
        //forbid to make tool extensible in the future
        QMessageBox::critical(0, QObject::tr("Error"),
                              QObject::tr("qgrit is supposed to be started by git rebase -i\n"
                                          "with \"--rebasei path/to/git-rebase-todo\" as argument."));

        return 1;
    }
}
