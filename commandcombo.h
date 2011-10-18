#ifndef PICKCOMBO_H
#define PICKCOMBO_H

#include <QComboBox>
#include <QTreeWidgetItem>

class CommandCombo : public QComboBox
{
    Q_OBJECT
public:
    explicit CommandCombo(QTreeWidgetItem *item, QWidget *parent = 0);

signals:

public Q_SLOTS:
    void slot_setCurrentIndex(int index);

private:
    QTreeWidgetItem *m_item;
};

#endif // PICKCOMBO_H
