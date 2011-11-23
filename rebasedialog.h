#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QModelIndex>

namespace Ui {
    class MainDialog;
}

class ListEntry
{
public:
    QString action;
    QString sha1;
    QString description;
    QString longdesc;
    QList<QString> files;

    ListEntry(QString action, QString sha1, QString description, QString longdesc, QList<QString> files) :
        action(action), sha1(sha1), description(description), longdesc(longdesc), files(files)
    {}
};

class RebaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RebaseDialog(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *event);
    bool readFile(const QString &filename);
    void fillList();
    void moveUpDown(bool up);
    ~RebaseDialog();

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_pushButtonUp_clicked();
    void on_pushButtonDown_clicked();
    void on_pushButtonUndo_clicked();
    void on_pushButtonStart_clicked();
    void on_pushButtonCancel_clicked();
    void slot_itemIserted(const QModelIndex &parent, int start, int end);
    void slot_itemSelectionChanged();

private:
    Ui::MainDialog *m_ui;
    QList<ListEntry> m_list;
    QString m_filename;
    bool m_acceptclose;
};

#endif // MAINDIALOG_H
