#ifndef CHARTEDITMODEL_H
#define CHARTEDITMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QtCharts/QChart>
#include <QList>
#include <QtCharts/QSplineSeries>
#include <QLabel>
#include <QGraphicsTextItem>

#include <map>
#include <utility>

#include "chartconfiguration.h"
#include "filesnpdata.h"

class ChartEditModel
    : public QAbstractItemModel
{
    Q_OBJECT

public:
    ChartEditModel(QtCharts::QChart* chart, QObject* parent = 0);

    ~ChartEditModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    bool hasChildren(const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;


    void addFile(QString filePath);
    QList<FileInfo> fileInfoList() const;
    void setFiles(QList<FileInfo> fileInfoList);
    void removeFile(int fileIndex);

    enum class NodeType
    {
    Configuration,
        ChartTitle,
        xTitle, yTitle,
        xMin, xMax,
        yMin, yMax,
        xGrid, yGrid,
        Legend,
    FileName,
        FilePath,
        Columns,
        LineWidth,
        LineColor,
        Multiplier,
        Z0,
    Invalid
    };

    struct Node
    {
        Node(ChartEditModel* model, NodeType type = NodeType::Invalid, Node *parent = 0)
            : model(model)
            , parent(parent)
            , type(type)
        {}
        ~Node()
        {
            for (auto node : children)
                delete node;
        }

        Node* parent;
        QVector<Node*> children;
        NodeType type;
        ChartEditModel* model;
    };

    static const std::map<NodeType, QString> TYPE_TO_STRING;

private:

    void drawLines() const;

    QList<std::pair<int, int>> stringToListColumns(QString line) const;
    QString listToStringColumns(QList<std::pair<int, int>> columns) const;

    Node *parent(Node *child) const;
    int row(Node *node) const;

    int cc;
    QVector<Node*> tree;

    QtCharts::QChart* chart;

    QList<FileSNPData> files;
    int selectedFile;

    QGraphicsTextItem* coordinatesLabel;
};

#endif // CHARTEDITMODEL_H
