#ifndef FIELDDELEGATE_H
#define FIELDDELEGATE_H

#include <QObject>
#include <QWidget>
#include <QStyledItemDelegate>

class FieldDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT

public:
    FieldDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &/* index */) const override;
};
#endif // FIELDDELEGATE_H
