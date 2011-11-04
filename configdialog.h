#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui {
    class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();
    void updateGitToolStatus();
    void gitConfigSetClearSequenceEditor(bool set);
    QString gitConfigGetSequenceEditor();

private slots:
    void on_pushButtonExit_clicked();
    void on_pushButtonInstall_clicked();
    void on_pushButtonUninstall_clicked();

private:
    Ui::ConfigDialog *m_ui;
};

#endif // CONFIGDIALOG_H
