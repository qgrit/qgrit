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
    connect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slot_itemSelectionChanged()));

    QStyle *s = this->style();
    m_ui->pushButtonUp->setIcon(s->standardIcon(QStyle::SP_ArrowUp, 0, m_ui->pushButtonUp));
    m_ui->pushButtonDown->setIcon(s->standardIcon(QStyle::SP_ArrowDown, 0, m_ui->pushButtonDown));

    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QAbstractItemModel *m = m_ui->treeWidget->model();
    connect(m, SIGNAL(rowsInserted(const QModelIndex,int,int)), this, SLOT(slot_itemIserted(QModelIndex,int,int)));
}

void RebaseDialog::slot_itemSelectionChanged()
{
    //clear background and remove tooltip
    for(int i = 0; i < m_ui->treeWidget->topLevelItemCount(); i++)
    {
        m_ui->treeWidget->topLevelItem(i)->setBackground(1, QBrush());
        m_ui->treeWidget->topLevelItem(i)->setToolTip(1, QString());
    }

    QList<QTreeWidgetItem *> selectedItems = m_ui->treeWidget->selectedItems();
    QStringList allfiles;
    //get list of all files modified by selected commits
    foreach(QTreeWidgetItem *item, selectedItems)
    {
        QList<QString> files = item->data(0, Qt::UserRole).toStringList();
        allfiles.append(files);
    }
    allfiles.sort();
    allfiles.removeDuplicates();
    //now check all unselected items to see if they modify the same files
    for(int i = 0; i < m_ui->treeWidget->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->topLevelItem(i);
        if(!selectedItems.contains(item))
        {
            QStringList affectedfiles;
            QList<QString> files = item->data(0, Qt::UserRole).toStringList();
            foreach(const QString &str, allfiles)
            {
                if(files.contains(str))
                    affectedfiles.append(str);
            }
            if(!affectedfiles.isEmpty())
            {
                item->setBackground(1, QBrush(Qt::yellow));
                item->setToolTip(1, tr("This commit modifies the same files as the selected commits:\n")
                                 + affectedfiles.join(QLatin1String("\n")));
            }
        }
    }
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

static QString gitCommitMessage(const QString &commit, QList<QString> &filenames)
{
    QByteArray stdOut;
    QStringList sl;
    //git log -1 -z --name-only --encoding=utf8 --pretty=format:%B%x01 commit_id

    //headline
    //
    //log message
    //\x01fileA\x00fileB\x00fileC\x00

    //using %x00 does not work on Windows
    sl << QLatin1String("log") << QLatin1String("-1") << QLatin1String("-z") << QLatin1String("--name-only") << QLatin1String("--encoding=utf8") << QLatin1String("--pretty=format:%B%x01") << commit;
    GitTool::gitExecuteStdOutStdErr(sl, stdOut);

    QList<QByteArray> split = stdOut.split('\x01');
    Q_ASSERT(split.length() == 2);
    QByteArray message = split.at(0);
    split = split.at(1).split('\x00');
    QByteArray empty = split.takeLast();
    Q_ASSERT(empty.isEmpty());

    for(int i = 0; i < split.length(); ++i)
    {
        QByteArray b = split.at(i);
        //first entry has errornous \n at start
        if(b.startsWith('\n'))
            b = b.mid(1);
        filenames.append(QString::fromUtf8(b.constData()));
    }

    GitTool::stripTrailingNewline(message);
    return QString::fromUtf8(message.constData());
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
        QList<QString> filenames;
        QString longdesc = gitCommitMessage(sha1, filenames);

        //reorder list to look like in gitk
        //file: top = oldest
        //tool: top = newest
        m_list.prepend(ListEntry(action, sha1, description, longdesc, filenames));
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
        QString tooltip = entry.longdesc + QLatin1String("\n");
        for(int j = 0; j < entry.files.size(); j++)
        {
            tooltip = tooltip + QLatin1String("\n") + entry.files.at(j);
            //maximum 10 files to keep overview
            if(j >= 9 && entry.files.size() > 10)
            {
                tooltip = tooltip + QString(QLatin1String("\n... %1 other files")).replace(
                            QLatin1String("%1"), QString::number(entry.files.size() - 10));
                break;
            }
        }
        item->setToolTip(3, tooltip);
        item->setData(0, Qt::UserRole, QVariant(entry.files));

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
