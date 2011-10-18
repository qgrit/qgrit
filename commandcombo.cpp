#include "commandcombo.h"

enum commandType
{
    commandPick = 0,
    commandEdit,
    commandReword,
    commandSquash,
    commandFixup,
    commandDrop
};

CommandCombo::CommandCombo(QTreeWidgetItem *item, QWidget *parent) :
    QComboBox(parent), m_item(item)
{
    this->addItem(tr("pick"));
    this->setItemData(commandPick, tr("Use commit.\nThe commit is not modified."), Qt::ToolTipRole);
    this->addItem(tr("edit"));
    this->setItemData(commandEdit, tr("Use commit.\nStop for amendinding.\nUsed for modifying/splitting a commit."), Qt::ToolTipRole);
    this->addItem(tr("reword"));
    this->setItemData(commandReword, tr("Use commit.\nEdit commit message."), Qt::ToolTipRole);
    this->addItem(tr("squash"));
    this->setItemData(commandSquash, tr("Use commit.\nMeld with commit below.\nUsed for combining 2 or more commits."), Qt::ToolTipRole);
    this->addItem(tr("fixup"));
    this->setItemData(commandFixup, tr("Use commit.\nMeld with commit below and discard this commit message.\nUsed for adding small fixes to a commit."), Qt::ToolTipRole);
    this->addItem(tr("drop"));
    this->setItemData(commandDrop, tr("Do not use commit.\nThe commit will not be included in the rebased version."), Qt::ToolTipRole);

    int index;
    QString s = item->data(0, Qt::DisplayRole).toString();
    if(s == QLatin1String("pick"))
        index = commandPick;
    else if(s == QLatin1String("edit"))
        index = commandEdit;
    else if(s == QLatin1String("reword"))
        index = commandReword;
    else if(s == QLatin1String("squash"))
        index = commandSquash;
    else if(s == QLatin1String("fixup"))
        index = commandFixup;
    else if(s == QLatin1String("#pick"))
        index = commandDrop;
    else
        Q_ASSERT(0 && "invalid string in setEditorData");

    this->setCurrentIndex(index);

    connect(this, SIGNAL(currentIndexChanged(int)), SLOT(slot_setCurrentIndex(int)));

    QPalette::ColorGroup cg = QPalette::Active;
    if(index == commandDrop)
        cg = QPalette::Disabled;

    QBrush br = this->palette().brush(cg, QPalette::Text);
    m_item->setData(1, Qt::ForegroundRole, br);
    m_item->setData(2, Qt::ForegroundRole, br);
    m_item->setData(3, Qt::ForegroundRole, br);
}


void CommandCombo::slot_setCurrentIndex(int index)
{
    QString s;
    switch(index)
    {
        case commandPick:
            s = QLatin1String("pick");
            break;
        case commandEdit:
            s = QLatin1String("edit");
            break;
        case commandReword:
            s = QLatin1String("reword");
            break;
        case commandSquash:
            s = QLatin1String("squash");
            break;
        case commandFixup:
            s = QLatin1String("fixup");
            break;
        case commandDrop:
            s = QLatin1String("#pick");
            break;
        default:
            Q_ASSERT(0 && "invalid index in slotsetCurrentIndex");
    }
    m_item->setData(0, Qt::DisplayRole, s);

    QPalette::ColorGroup cg = QPalette::Active;
    if(index == commandDrop)
        cg = QPalette::Disabled;

    QBrush br = this->palette().brush(cg, QPalette::Text);
    m_item->setData(1, Qt::ForegroundRole, br);
    m_item->setData(2, Qt::ForegroundRole, br);
    m_item->setData(3, Qt::ForegroundRole, br);
}
