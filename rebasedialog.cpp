#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QItemDelegate>
#include <QMessageBox>
#include <QTextStream>
#include <QTreeWidgetItem>

#include "gittool.h"
#include "rebasedialog.h"
#include "ui_rebasedialog.h"
#include "commandcombo.h"

#define QGRIT_VERSION "0.1"


//this is necessary otherwise the combo box is placed wrong vertically when items are moved up/down
//it probably happens because by default the height of a line is smaller than the height of a combo box
class LineHeightItemDelegate : public QItemDelegate
{
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        Q_UNUSED(option);
        Q_UNUSED(index);

        static bool initialized;
        static QSize size;
        if(!initialized)
        {
            QComboBox *cb = new QComboBox;
            size = cb->sizeHint();
            delete cb;
            initialized = true;
        }
        return size;
    }
};

RebaseDialog::RebaseDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::MainDialog),
    m_acceptclose(false)
{
    m_ui->setupUi(this);

    m_ui->labelVersion->setText(m_ui->labelVersion->text().replace(QLatin1String("%1"), QLatin1String(QGRIT_VERSION)));
    QStringList headerLabels;
    headerLabels << tr("Action") << tr("Nr") << tr("SHA-1") << tr("Description");
    m_ui->treeWidget->setHeaderLabels(headerLabels);
    m_ui->treeWidget->resizeColumnToContents(1);
    m_ui->treeWidget->setRootIsDecorated(false);
    m_ui->treeWidget->setItemDelegate(new LineHeightItemDelegate);
    m_ui->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ui->treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->treeWidget->setDragDropMode(QAbstractItemView::InternalMove);

    QStyle *s = this->style();
    m_ui->pushButtonUp->setIcon(s->standardIcon(QStyle::SP_ArrowUp, 0, m_ui->pushButtonUp));
    m_ui->pushButtonDown->setIcon(s->standardIcon(QStyle::SP_ArrowDown, 0, m_ui->pushButtonDown));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QAbstractItemModel *m = m_ui->treeWidget->model();
    connect(m, SIGNAL(rowsInserted(const QModelIndex,int,int)), this, SLOT(slot_itemIserted(QModelIndex,int,int)));
}

void RebaseDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        //the default implementation calls reject() instead
        //but we want to show a quit yes/no dialog
        this->close();
        return;
    }
    QDialog::keyPressEvent(event);
}

RebaseDialog::~RebaseDialog()
{
    delete m_ui;
}

//when a item is inserted into the list, we add a combo box widget to column 0
//moving items is done by removing elements and inserting them at a different point
//so we also end up here in this case
void RebaseDialog::slot_itemIserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent);
    for(int i = start; i <= end; i++)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->topLevelItem(i);

        //items that are inserted should not have any widget yet
        Q_ASSERT(!m_ui->treeWidget->itemWidget(item, 0));

        m_ui->treeWidget->setItemWidget(item, 0, new CommandCombo(item));
    }
}

static QString gitCommitMessage(const QString &commit)
{
    QByteArray stdOut;
    QStringList sl;
    //git log -1 --encoding=utf8 --pretty=format:%B

    sl << QLatin1String("log") << QLatin1String("-1") << QLatin1String("--encoding=utf8") << QLatin1String("--pretty=format:%B") << commit;
    GitTool::gitExecuteStdOutStdErr(sl, stdOut);
    GitTool::stripTrailingNewline(stdOut);

    return QString::fromUtf8(stdOut.constData(), stdOut.length());
}

bool RebaseDialog::readFile(const QString &filename)
{
    QFile f(filename);
    //on windows files are also separated only by \n
    if(!f.open(QFile::ReadWrite))
    {
        QMessageBox::critical(0, tr("Error reading rebase todo list"),
                              tr("Could not read file %1").replace(QLatin1String("%1"), filename));
        return false;
    }

    QTextStream ts(&f);
    QString s;
    QStringList l;
    while(!(s = ts.readLine()).isNull())
    {
        if(s.isEmpty())
            continue;
        if(!s.startsWith(QLatin1Char('#')) ||
            s.startsWith(QLatin1String("#pick")))
            l << s;
    }
    m_list.clear();

    foreach(QString str1, l)
    {
        //a line looks like this: "pick 412c9c3 description headline"
        int space = str1.indexOf(QLatin1Char(' '));
        QString action = str1.left(space);
        str1 = str1.mid(space + 1);

        //now line looks like this: "412c9c3 description headline"
        space = str1.indexOf(QLatin1Char(' '));
        QString sha1;
        QString description;
        if(space != -1)
        {
            sha1 = str1.left(space);
            description = str1.mid(space + 1);
        }
        else
        {
            sha1 = str1;
            description = QLatin1String("");
            //no description
        }
        QString longdesc = gitCommitMessage(sha1);

        //reorder list to look like in gitk
        //file: top = oldest
        //tool: top = newest
        m_list.prepend(ListEntry(action, sha1, description, longdesc));
    }

    m_filename = filename;
    return true;
}

void RebaseDialog::fillList()
{
    QTreeWidgetItem *itemfirst = 0;

    m_ui->treeWidget->setUpdatesEnabled(false);
    m_ui->treeWidget->clear();
    int i = 0;
    foreach(const ListEntry entry, m_list)
    {
        QStringList a;
        a << entry.action << QString::number(i + 1) << entry.sha1 << entry.description;
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->treeWidget, a);
        //do not allow to drop other items onto this one,
        //otherwise the other element is removed, also this makes no sense...
        item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        item->setToolTip(3, entry.longdesc);

        if(!itemfirst)
            itemfirst = item;
        i++;
    }
    if(itemfirst)
        m_ui->treeWidget->setCurrentItem(itemfirst);

    m_ui->treeWidget->setUpdatesEnabled(true);
}

void RebaseDialog::moveUpDown(bool up)
{
    QItemSelectionModel *selmodel = m_ui->treeWidget->selectionModel();
    QModelIndexList rows = selmodel->selectedRows();
    if(rows.length() == 0)
        return; //empty list, no selection

    m_ui->treeWidget->setUpdatesEnabled(false);

    QList<QTreeWidgetItem *> list;

    //first sort the list otherwise it might depend on selection order when using Ctrl-click
    //sort compares row, column, parent
    qSort(rows);

    //start from bottom to not modify index during loop
    for(int i = rows.length() - 1; i >= 0; i--)
        list.prepend(m_ui->treeWidget->takeTopLevelItem(rows.at(i).row()));

    int insertpos;
    if (up)
        insertpos = qMax(rows.first().row() - 1, 0);
    else
        insertpos = qMin(rows.last().row() + 2 - rows.length(), m_ui->treeWidget->topLevelItemCount());

    for(int i = 0; i < list.length(); i++)
    {
        QTreeWidgetItem *item = list.at(i);
        m_ui->treeWidget->insertTopLevelItem(insertpos + i, item);
    }

    m_ui->treeWidget->clearSelection();
    m_ui->treeWidget->setCurrentItem(list.at(0));
    for(int i = 0; i< list.length(); i++)
        list.at(i)->setSelected(true);

    m_ui->treeWidget->setUpdatesEnabled(true);
}

void RebaseDialog::on_pushButtonUp_clicked()
{
    moveUpDown(true);
}

void RebaseDialog::on_pushButtonDown_clicked()
{
    moveUpDown(false);
}

void RebaseDialog::on_pushButtonUndo_clicked()
{
    int result = QMessageBox::question(this, tr("Undo changes done to list"),
                                       tr("Do you really want to undo all changes made to the rebase todo list?"),
                                       QMessageBox::Yes, QMessageBox::No);
    if(result == QMessageBox::Yes)
        fillList();
}

void RebaseDialog::on_pushButtonStart_clicked()
{
    QFile f(m_filename);
    //on windows files are also separated only by \n
    bool success = f.open(QFile::WriteOnly | QFile::Truncate);
    if(!success)
    {
        QMessageBox::critical(0, tr("Error"),
                              tr("Could not write file %1").replace(QLatin1String("%1"), m_filename));
        return;
    }
    QTextStream ts(&f);

    int count = m_ui->treeWidget->topLevelItemCount();
    //write out again in original order
    for(int i = count - 1; i >= 0; i--)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->topLevelItem(i);
        QString action = item->data(0, Qt::DisplayRole).toString();
        QString sha1 = item->data(2, Qt::DisplayRole).toString();
        QString description = item->data(3, Qt::DisplayRole).toString();
        ts << QString(action + QLatin1Char(' ') + sha1 + QLatin1Char(' ') + description + QLatin1Char('\n'));
    }
    m_acceptclose = true;
    this->close();
}

void RebaseDialog::on_pushButtonCancel_clicked()
{
    this->close();
}

void RebaseDialog::closeEvent(QCloseEvent *event)
{
    if(m_acceptclose)
    {
        event->accept();
        return;
    }

    int result = QMessageBox::question(this, tr("Abort interactive rebase"),
                                       tr("Do you really want to abort the interactive rebase?"),
                                       QMessageBox::Yes, QMessageBox::No);

    if(result == QMessageBox::Yes)
    {
        QFile f(m_filename);
        //a empty file aborts the rebase
        //we can not remove the file because rebase -i tries to open the file even if it does not exist anymore
        f.resize(0);
        event->accept();
    }
    else
        event->ignore();
}
