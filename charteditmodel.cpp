#include "charteditmodel.h"

#include <QValueAxis>
#include <QLegend>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QIcon>
#include <QLabel>
#include <QCursor>

QT_CHARTS_USE_NAMESPACE

const std::map<ChartEditModel::NodeType, QString> ChartEditModel::TYPE_TO_STRING = {
    {ChartEditModel::NodeType::Configuration,   "Configuration"},
    {ChartEditModel::NodeType::ChartTitle,      "Chart Title"},
    {ChartEditModel::NodeType::xTitle,          "X-Title"},
    {ChartEditModel::NodeType::yTitle,          "Y-Title"},
    {ChartEditModel::NodeType::xMin,            "Minimal X"},
    {ChartEditModel::NodeType::xMax,            "Maximal X"},
    {ChartEditModel::NodeType::yMin,            "Minimal Y"},
    {ChartEditModel::NodeType::yMax,            "Maximal Y"},
    {ChartEditModel::NodeType::xGrid,           "X Grid Number"},
    {ChartEditModel::NodeType::yGrid,           "Y Grid Number"},
    {ChartEditModel::NodeType::Legend,          "Show Legend"},
    {ChartEditModel::NodeType::FileName,        "Name"},
    {ChartEditModel::NodeType::FilePath,        "Path"},
    {ChartEditModel::NodeType::Columns,         "Columns"},
    {ChartEditModel::NodeType::LineWidth,       "Line Width"},
    {ChartEditModel::NodeType::LineColor,       "Line Color"},
    {ChartEditModel::NodeType::Multiplier,      "Multiplier"},
    {ChartEditModel::NodeType::Z0,              "Z0"}
};

ChartEditModel::ChartEditModel(QChart *chart, QObject *parent)
    : QAbstractItemModel(parent)
    , chart(chart)
    , cc(1)
    , selectedFile(-1)
{
    Node* config = new Node(this, NodeType::Configuration);
    config->children
        << new Node(this, NodeType::ChartTitle, config)
        << new Node(this, NodeType::xTitle,     config)
        << new Node(this, NodeType::yTitle,     config)
        << new Node(this, NodeType::xMin,       config)
        << new Node(this, NodeType::xMax,       config)
        << new Node(this, NodeType::yMin,       config)
        << new Node(this, NodeType::yMax,       config)
        << new Node(this, NodeType::xGrid,      config)
        << new Node(this, NodeType::yGrid,      config)
        << new Node(this, NodeType::Legend,     config);

    tree.push_back(config);

    coordinatesLabel = new QGraphicsTextItem(chart);
}

ChartEditModel::~ChartEditModel()
{
    for (auto node : tree)
        delete node;
}

QModelIndex ChartEditModel::index(int row, int column, const QModelIndex &parent) const
{
    Node* parentNode = static_cast<Node*>(parent.internalPointer());
    if (parentNode)
    {
        if (column != 0 || row < 0 || row >= parentNode->children.size())
            return QModelIndex();

        return createIndex(row, column, parentNode->children.at(row));
    }

    if (row >= tree.size() || row < 0 || column >= cc || column < 0)
        return QModelIndex();

    return createIndex(row, column, tree.at(row));
}

QModelIndex ChartEditModel::parent(const QModelIndex &child) const
{
    Node *childNode = static_cast<Node*>(child.internalPointer());
    if (child.isValid()) {
        Node *parentNode = parent(childNode);
        if (parentNode)
            return createIndex(row(parentNode), 0, parentNode);
    }
    return QModelIndex();
}

int ChartEditModel::rowCount(const QModelIndex &parent) const
{
    Node* node = static_cast<Node*>(parent.internalPointer());
    if (!node)
        return tree.size();
    return node->children.size();
}

int ChartEditModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return cc;
}

QVariant ChartEditModel::data(const QModelIndex &index, int role) const
{
    Node* node = static_cast<Node*>(index.internalPointer());
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DecorationRole)
    {
        if (node->type == NodeType::FileName)
        {
            if (row(node) != selectedFile + 1)
                return QIcon(":/icons/close.png");
            else
                return QIcon(":/icons/check.png");
        }
        if (node->type == NodeType::Configuration)
        {
            return QIcon(":/icons/config.png");
        }
    }
    if (role == Qt::DisplayRole)
    {
        if (node->type == NodeType::FileName)
            return files.at(row(node) - 1).getFileName();
        if (node->type == NodeType::FilePath)
            return files.at(row(parent(node)) - 1).getFilePath();
        if (node->type == NodeType::Z0)
            return "Z0 = " + QString::number(files.at(row(parent(node)) - 1).getZ0());

        QString result = TYPE_TO_STRING.at(node->type);
        return result;
    }
    if (role == Qt::EditRole)
    {
        switch (node->type) {
        case NodeType::ChartTitle:
            return QVariant(chart->title());
            break;
        case NodeType::xTitle:
            return QVariant(chart->axisX()->titleText());
            break;
        case NodeType::yTitle:
            return QVariant(chart->axisY()->titleText());
            break;
        case NodeType::xMin:
            return QVariant(static_cast<QValueAxis*>(chart->axisX())->min());
            break;
        case NodeType::xMax:
            return QVariant(static_cast<QValueAxis*>(chart->axisX())->max());
            break;
        case NodeType::yMin:
            return QVariant(static_cast<QValueAxis*>(chart->axisY())->min());
            break;
        case NodeType::yMax:
            return QVariant(static_cast<QValueAxis*>(chart->axisY())->max());
            break;
        case NodeType::xGrid:
            return QVariant(static_cast<QValueAxis*>(chart->axisX())->tickCount());
            break;
        case NodeType::yGrid:
            return QVariant(static_cast<QValueAxis*>(chart->axisY())->tickCount());
            break;
        case NodeType::Legend:
            return QVariant(chart->legend()->isVisible());
            break;
        case NodeType::Columns:
            return listToStringColumns(files.at(row(parent(node)) - 1).getColumns());
            break;
        case NodeType::LineWidth:
            return files.at(row(parent(node)) - 1).getLineWidth();
            break;
        case NodeType::LineColor:
            return files.at(row(parent(node)) - 1).getLineColor();
            break;
        case NodeType::Multiplier:
            return files.at(row(parent(node)) - 1).getMultiplier();
            break;
        }

        return QVariant();
    }
    return QVariant();
}

bool ChartEditModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(role);
    Node* node = static_cast<Node*>(index.internalPointer());
    if (!index.isValid() || !node)
        return false;
    switch (node->type) {
    case NodeType::ChartTitle:
        chart->setTitle(value.toString());
        break;
    case NodeType::xTitle:
        chart->axisX()->setTitleText(value.toString());
        break;
    case NodeType::yTitle:
        chart->axisY()->setTitleText(value.toString());
        break;
    case NodeType::xMin:
        if (static_cast<QValueAxis*>(chart->axisX())->max() <= value.toDouble())
            return false;
        chart->axisX()->setMin(value);
        break;
    case NodeType::xMax:
        if (static_cast<QValueAxis*>(chart->axisX())->min() >= value.toDouble())
            return false;
        chart->axisX()->setMax(value);
        break;
    case NodeType::yMin:
        if (static_cast<QValueAxis*>(chart->axisY())->max() <= value.toDouble())
            return false;
        chart->axisY()->setMin(value);
        break;
    case NodeType::yMax:
        if (static_cast<QValueAxis*>(chart->axisY())->min() >= value.toDouble())
            return false;
        chart->axisY()->setMax(value);
        break;
    case NodeType::xGrid:
        static_cast<QValueAxis*>(chart->axisX())->setTickCount(value.toInt());
        break;
    case NodeType::yGrid:
        static_cast<QValueAxis*>(chart->axisY())->setTickCount(value.toInt());
        break;
    case NodeType::Legend:
        chart->legend()->setVisible(value.toBool());
        break;
    case NodeType::Columns:
        selectedFile = row(parent(node)) - 1;
        files[selectedFile].setColumns(stringToListColumns(value.toString()));
        drawLines();
        break;
    case NodeType::LineWidth:
        files[row(parent(node)) - 1].setLineWidth(value.toInt());
        drawLines();
        break;
    case NodeType::LineColor:
        files[row(parent(node)) - 1].setLineColor(value.value<QColor>());
        drawLines();
        break;
    case NodeType::Multiplier:
        files[row(parent(node)) - 1].setMultiplier(value.toDouble());
        drawLines();
        break;
    }
    emit dataChanged(index, index);
    return true;
}

bool ChartEditModel::hasChildren(const QModelIndex &parent) const
{
    Node* node = static_cast<Node*>(parent.internalPointer());
    return node == 0 ?
           tree.size() > 0 && cc > 0 :
           !node->children.isEmpty();
}

Qt::ItemFlags ChartEditModel::flags(const QModelIndex &index) const
{
    NodeType t = static_cast<Node*>(index.internalPointer())->type;
    if (!index.isValid())
        return 0;
    if (t != NodeType::Configuration &&
        t != NodeType::FileName &&
        t != NodeType::FilePath &&
        t != NodeType::Z0)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    return QAbstractItemModel::flags(index);
}

void ChartEditModel::addFile(QString filePath)
{
    int idx = -1;
    for (int i = 0; i < files.size(); ++i)
    {
        if (files.at(i).getFilePath() == filePath)
            idx = i;
    }
    if (idx != -1)
    {
        selectedFile = idx;
        drawLines();
        return;
    }

    files.push_back(FileSNPData(filePath));

    Node* fileNode = new Node(this, NodeType::FileName);
    fileNode->children
        << new Node(this, NodeType::FilePath,     fileNode)
        << new Node(this, NodeType::Columns,      fileNode)
        << new Node(this, NodeType::LineWidth,    fileNode)
        << new Node(this, NodeType::LineColor,    fileNode)
        << new Node(this, NodeType::Multiplier,   fileNode)
        << new Node(this, NodeType::Z0,           fileNode);

    emit beginInsertRows(QModelIndex(), selectedFile + 1, selectedFile + 1);
    tree.push_back(fileNode);
    emit endInsertRows();
    emit dataChanged(index(selectedFile + 1, 0, QModelIndex()), index(selectedFile + 1, 0, QModelIndex()));
}

QList<FileInfo> ChartEditModel::fileInfoList() const
{
    QList<FileInfo> result;
    foreach (FileSNPData fileData, files)
    {
        result.push_back({
            fileData.getFilePath(),
            // TODO columns from files can be invalid
            listToStringColumns(fileData.getColumns()),
            fileData.getLineWidth(),
            fileData.getLineColor(),
            fileData.getMultiplier()
        });
    }
    return result;
}

void ChartEditModel::setFiles(QList<FileInfo> fileInfoList)
{
    foreach (FileInfo info, fileInfoList)
    {
        addFile(info.filePath);
    }
}

void ChartEditModel::removeFile(int fileIndex)
{
    if (fileIndex < 0 || fileIndex >= files.size())
        throw std::invalid_argument("No such file opened");

    emit beginRemoveRows(QModelIndex(), fileIndex + 1, fileIndex + 1);
    delete tree.at(fileIndex + 1);
    tree.erase(tree.begin() + fileIndex + 1);
    files.erase(files.begin() + fileIndex);
    emit endRemoveRows();
    emit dataChanged(index(fileIndex, 0, QModelIndex()), index(fileIndex, 0, QModelIndex()));

    if (selectedFile == files.size())
        --selectedFile;

    drawLines();
}

void ChartEditModel::drawLines() const
{
    if (files.isEmpty() || selectedFile == -1)
    {
        chart->removeAllSeries();
        return;
    }

    chart->removeAllSeries();

    qreal xMin, xMax, yMin, yMax;
    QList<QSplineSeries*> curves;
    std::tie(xMin, xMax, yMin, yMax, curves) =
        files.at(selectedFile).getDrawableData();

    foreach (QSplineSeries* curve, curves)
    {
        chart->addSeries(curve);
        connect(curve,
                &QSplineSeries::hovered,
                [this, curve](const QPointF& p, bool hovered) -> void
                {
                    if (!hovered)
                    {
                        coordinatesLabel->setVisible(false);
                        return;
                    }
                    QPointF chartCoordinates = chart->mapToPosition(p, curve);
                    coordinatesLabel->setPlainText(
                        QString("[") + QString::number(p.x()) + "," + QString::number(p.y()) + "]"
                    );
                    chartCoordinates.setX(chartCoordinates.x() - coordinatesLabel->boundingRect().width() / 2);
                    chartCoordinates.setY(chartCoordinates.y() - 20);
                    coordinatesLabel->setPos(chartCoordinates);
                    coordinatesLabel->setVisible(true);
                    coordinatesLabel->setZValue(1);
                }
        );
        curve->attachAxis(chart->axisX());
        curve->attachAxis(chart->axisY());
        auto pen = static_cast<QSplineSeries*>(chart->series().back())->pen();
        pen.setWidth(files.at(selectedFile).getLineWidth());
        static_cast<QSplineSeries*>(chart->series().back())->setPen(pen);
    }

    // set color only for the last curve
    auto pen = static_cast<QSplineSeries*>(chart->series().back())->pen();
    pen.setColor(files.at(selectedFile).getLineColor());
    static_cast<QSplineSeries*>(chart->series().back())->setPen(pen);

    chart->axisX()->setRange(xMin, xMax);
    chart->axisY()->setRange(yMin, yMax);
}

QList<std::pair<int, int>> ChartEditModel::stringToListColumns(QString line) const
{
    QList<std::pair<int, int>> result;

    QRegularExpression columnFinder("\\[(\\d+),(\\d+)\\]");
    int idx = 0;
    QRegularExpressionMatch match;
    while ((idx = line.indexOf(columnFinder, idx, &match) + 1) != 0)
    {
        result.push_back({match.captured(1).toInt(), match.captured(2).toInt()});
    }
    return result;
}

QString ChartEditModel::listToStringColumns(QList<std::pair<int, int>> columns) const
{
    QString result;

    for (const auto& c : columns)
    {
        result += "[" + QString::number(c.first) + "," + QString::number(c.second) + "],";
    }

    result.chop(1);

    return result;
}

ChartEditModel::Node *ChartEditModel::parent(Node *child) const
{
    return child ? child->parent : 0;
}

int ChartEditModel::row(Node *node) const
{
    if (node->parent)
        return parent(node)->children.indexOf(node);
    return tree.indexOf(node);
}
