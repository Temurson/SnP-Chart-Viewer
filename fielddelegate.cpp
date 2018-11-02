#include "fielddelegate.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QRect>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QRegExp>
#include <QColorDialog>
#include <QPainter>
#include <QStyleOption>

#include <stdexcept>
#include <limits>

#include "charteditmodel.h"
#include "filesnpdata.h"

class CorrectDoubleValidator
    : public QDoubleValidator
{
public:
    CorrectDoubleValidator(double bottom, double top, int decimals, QObject * parent = 0) :
        QDoubleValidator(bottom, top, decimals, parent)
    {
    }

    QValidator::State validate(QString &s, int &i) const override
    {
        Q_UNUSED(i);
        for (auto& c : s)
        {
            if (c != '-' && c != '+' &&
                c != '.' && !c.isDigit())
                return QValidator::Invalid;
        }

        bool ok;
        double d = s.toDouble(&ok);

        if (ok && d >= bottom() && d <= top()) {
            return QValidator::Acceptable;
        } else {
            return QValidator::Intermediate;
        }
    }
};

class ColumnValidator
    : public QValidator
{
public:
    ColumnValidator(QObject * parent = 0) :
        QValidator(parent)
    {
    }

    QValidator::State validate(QString &s, int &i) const override
    {
        Q_UNUSED(i);
        for (auto& c : s)
        {
            if (c != '[' && c != ']' &&
                c != ' ' && c != ',' &&
                !c.isDigit())
                return QValidator::Invalid;
        }

        QRegExp columnsFinder("(?:\\[\\d+,\\d+\\],)*\\[\\d+,\\d+\\]");
        columnsFinder.indexIn(s);
        bool ok = columnsFinder.matchedLength() != -1;

        if (ok) {
            return QValidator::Acceptable;
        } else {
            return QValidator::Intermediate;
        }
    }
};

class ColorEditor
    : public QWidget
{
public:
    ColorEditor(QColor color, const QModelIndex& index, QWidget* parent = 0)
        : QWidget(parent)
        , colorFromModel(color)
        , index(index)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setFocusPolicy(Qt::StrongFocus);
    }

    void setColor(QColor color)
    {
        colorFromModel = color;
        setStyleSheet(
            QString("border: 2px solid black;") +
            QString("background-color: ") + colorFromModel.name() + QString(";")
        );
    }

    QColor getColor() const
    {
        QColor c = colorFromDialog.isNull() ?
                   colorFromModel : colorFromDialog.value<QColor>();
        colorFromDialog = QVariant();
        return c;
    }

    void mouseReleaseEvent(QMouseEvent*) override
    {
        colorFromDialog = QVariant(QColorDialog::getColor(colorFromModel, parentWidget()));
        setFocus();
        clearFocus();
    }

private:
    QColor colorFromModel;
    mutable QVariant colorFromDialog;
    const QModelIndex& index;
};

FieldDelegate::FieldDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *FieldDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                      const QModelIndex &index) const
{
    Q_UNUSED(option);
    ChartEditModel::Node* node = static_cast<ChartEditModel::Node*>(index.internalPointer());

    QLineEdit* lineEdit;
    QSpinBox* spinBox;
    QCheckBox* checkBox;

    using NodeType = ChartEditModel::NodeType;
    switch (node->type) {
    case NodeType::ChartTitle:
    case NodeType::xTitle:
    case NodeType::yTitle:
        lineEdit = new QLineEdit(parent);
        lineEdit->setFrame(false);
        return lineEdit;
    case NodeType::xMin:
    case NodeType::xMax:
    case NodeType::yMin:
    case NodeType::yMax:
    case NodeType::Multiplier:
        lineEdit = new QLineEdit(parent);
        lineEdit->setValidator(new CorrectDoubleValidator(
            std::numeric_limits<qreal>::lowest(),
            std::numeric_limits<qreal>::max(),
            6
        ));
        lineEdit->setFrame(false);
        return lineEdit;
    case NodeType::xGrid:
    case NodeType::yGrid:
    case NodeType::LineWidth:
        spinBox = new QSpinBox(parent);
        spinBox->setRange(1, 100);
        spinBox->setSingleStep(1);
        spinBox->setFrame(false);
        return spinBox;
    case NodeType::Legend:
        checkBox = new QCheckBox(parent);
        return checkBox;
    case NodeType::Columns:
        lineEdit = new QLineEdit(parent);
        lineEdit->setValidator(new ColumnValidator);
        lineEdit->setFrame(false);
        return lineEdit;
    case NodeType::LineColor:
        return new ColorEditor(QColor(Qt::white), index, parent);
    }

    return new QWidget(parent);
}

void FieldDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ChartEditModel::Node* node = static_cast<ChartEditModel::Node*>(index.internalPointer());

    using NodeType = ChartEditModel::NodeType;
    switch (node->type) {
    case NodeType::ChartTitle:
    case NodeType::xTitle:
    case NodeType::yTitle:
    case NodeType::Columns:
        static_cast<QLineEdit*>(editor)->setText(index.data(Qt::EditRole).toString());
        break;
    case NodeType::xMin:
    case NodeType::xMax:
    case NodeType::yMin:
    case NodeType::yMax:
    case NodeType::Multiplier:
        static_cast<QLineEdit*>(editor)->setText(QString::number(index.data(Qt::EditRole).toDouble()));
        break;
    case NodeType::xGrid:
    case NodeType::yGrid:
    case NodeType::LineWidth:
        static_cast<QSpinBox*>(editor)->setValue(index.data(Qt::EditRole).toInt());
        break;
    case NodeType::Legend:
        static_cast<QCheckBox*>(editor)->setChecked(index.data(Qt::EditRole).toBool());
        break;
    case NodeType::LineColor:
        static_cast<ColorEditor*>(editor)->setColor(index.data(Qt::EditRole).value<QColor>());
        break;
    }
}
void FieldDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                  const QModelIndex &index) const
{
    ChartEditModel::Node* node = static_cast<ChartEditModel::Node*>(index.internalPointer());

    using NodeType = ChartEditModel::NodeType;
    switch (node->type) {
    case NodeType::ChartTitle:
    case NodeType::xTitle:
    case NodeType::yTitle:
    case NodeType::Columns:
        model->setData(
            index,
            QVariant(static_cast<QLineEdit*>(editor)->text())
        );
        break;
    case NodeType::xMin:
    case NodeType::xMax:
    case NodeType::yMin:
    case NodeType::yMax:
    case NodeType::Multiplier:
        model->setData(
            index,
            QVariant(static_cast<QLineEdit*>(editor)->text().replace(',', '.').toDouble())
        );
        break;
    case NodeType::xGrid:
    case NodeType::yGrid:
    case NodeType::LineWidth:
        model->setData(
            index,
            QVariant(static_cast<QSpinBox*>(editor)->value())
        );
        break;
    case NodeType::Legend:
        model->setData(
            index,
            QVariant(static_cast<QCheckBox*>(editor)->isChecked())
        );
        break;
    case NodeType::LineColor:
        model->setData(
            index,
            QVariant(static_cast<ColorEditor*>(editor)->getColor())
        );
        break;
    }

}

void FieldDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto t = static_cast<ChartEditModel::Node*>(index.internalPointer())->type;

    QRect rect = option.rect;
    if (t == ChartEditModel::NodeType::LineColor)
        rect.setWidth(rect.height());
    else
        rect.setWidth(rect.width() / 2);
    rect.moveLeft(rect.left() + option.rect.width() / 2);
    editor->setGeometry(rect);
}
