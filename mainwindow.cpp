#include "mainwindow.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QPushButton>
#include <QPainter>
#include <QSizePolicy>
#include <QLineEdit>
#include <QValueAxis>
#include <QMenu>
#include <QPixmap>
#include <QClipboard>

#include <tuple>

#include "charteditmodel.h"
#include "fielddelegate.h"
#include "chartconfiguration.h"

#include <QDebug>

QT_CHARTS_USE_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , copyMenu(new QMenu(this))
    , deleteFileMenu(new QMenu(this))
{
    setupUi(this);
    setupChart();
    setupConfigNode();

    connect(btnAddFile, &QPushButton::clicked, this, &MainWindow::addFile);
    connect(btnSaveConfig, &QPushButton::clicked, this, &MainWindow::saveConfig);
    connect(btnLoadConfig, &QPushButton::clicked, this, &MainWindow::loadConfig);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearAll);

    copyMenu->addAction("Copy Chart");
    connect(copyMenu, &QMenu::triggered, this, &MainWindow::copyChart);

    deleteFileMenu->addAction("Delete");
    connect(deleteFileMenu, &QMenu::triggered, this, &MainWindow::removeFile);
}

void MainWindow::setupChart()
{
    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(new QSplineSeries);
    chart->createDefaultAxes();

    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setObjectName("chartView");
    chartView->installEventFilter(this);
}

void MainWindow::setupConfigNode()
{
    treeView->setRootIsDecorated(false);
    treeView->setHeaderHidden(true);
    treeView->setIndentation(20);
    treeView->setStyleSheet(
    "QTreeView::branch:has-siblings:adjoins-item {"
        "border-image: url(:/icons/branch-more.png) 0;"
    "}"
    "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
        "border-image: url(:/icons/branch-end.png) 0;"
    "}"
    );

    treeView->setItemDelegate(new FieldDelegate(this));

    QAbstractItemModel* configModel = new ChartEditModel(chartView->chart());
    treeView->setModel(configModel);

    treeView->setObjectName("treeView");
    treeView->installEventFilter(this);

    refreshEditors();
}

void MainWindow::addFile()
{
    try
    {
        QString filePath = QFileDialog::getOpenFileName(this, "Open File");
        static_cast<ChartEditModel*>(treeView->model())->addFile(filePath);
        refreshEditors();
        // expand last file's section
        treeView->expand(treeView->model()->index(treeView->model()->rowCount() - 1, 0));
        treeView->setCurrentIndex(treeView->model()->index(
            // first argument (1) is Columns field row number
            // third argument is the last file section index
            1, 0, treeView->model()->index(treeView->model()->rowCount() - 1, 0)
        ));
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "File open failed", e.what(), QMessageBox::Ok);
    }
}

void MainWindow::saveConfig()
{
    try
    {
        QString filePath = QFileDialog::getSaveFileName(this, "Save File");
        QFile* pconfigFile = new QFile(filePath);
        if (!pconfigFile->open(QIODevice::NewOnly | QIODevice::WriteOnly))
            throw std::runtime_error(pconfigFile->errorString().toStdString());
        ChartConfiguration config{
            chartView->chart()->title(),
            chartView->chart()->axisX()->titleText(),
            chartView->chart()->axisY()->titleText(),
            static_cast<QValueAxis*>(chartView->chart()->axisX())->min(),
            static_cast<QValueAxis*>(chartView->chart()->axisX())->max(),
            static_cast<QValueAxis*>(chartView->chart()->axisY())->min(),
            static_cast<QValueAxis*>(chartView->chart()->axisY())->max(),
            static_cast<QValueAxis*>(chartView->chart()->axisX())->tickCount(),
            static_cast<QValueAxis*>(chartView->chart()->axisY())->tickCount(),
            chartView->chart()->legend()->isVisible(),
            static_cast<ChartEditModel*>(treeView->model())->fileInfoList()
        };
        pconfigFile->write(ChartConfiguration::toByteArray(config));
        pconfigFile->close();
        delete pconfigFile;
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Config saving failed", e.what(), QMessageBox::Ok);
    }
}

void MainWindow::loadConfig()
{
    try
    {
        QString filePath = QFileDialog::getOpenFileName(this, "Open File");
        QFile* pconfigFile = new QFile(filePath);
        if (!pconfigFile->open(QIODevice::ExistingOnly | QIODevice::ReadOnly))
            throw std::runtime_error(pconfigFile->errorString().toStdString());
        ChartConfiguration config(ChartConfiguration::fromByteArray(pconfigFile->readAll()));

        pconfigFile->close();
        delete pconfigFile;

        chartView->chart()->setTitle(config.chartTitle);
        chartView->chart()->axisX()->setTitleText(config.xTitle);
        chartView->chart()->axisY()->setTitleText(config.yTitle);
        static_cast<QValueAxis*>(chartView->chart()->axisX())->setRange(config.xMin, config.xMax);
        static_cast<QValueAxis*>(chartView->chart()->axisY())->setRange(config.yMin, config.yMax);
        static_cast<QValueAxis*>(chartView->chart()->axisX())->setTickCount(config.xGrid);
        static_cast<QValueAxis*>(chartView->chart()->axisY())->setTickCount(config.yGrid);
        chartView->chart()->legend()->setVisible(config.legend);
        static_cast<ChartEditModel*>(treeView->model())->setFiles(config.files);

        refreshEditors();
    }
    catch (const std::exception& e)
    {
        QMessageBox::critical(this, "Config loading failed", e.what(), QMessageBox::Ok);
    }
}

void MainWindow::clearAll()
{
    while (treeView->model()->rowCount() != 1)
        static_cast<ChartEditModel*>(treeView->model())->removeFile(0);

    static_cast<QValueAxis*>(chartView->chart()->axisX())->setRange(0, 1);
    static_cast<QValueAxis*>(chartView->chart()->axisY())->setRange(0, 1);

    refreshEditors();
}

void MainWindow::refreshEditors()
{
    for (int i = 0; i < treeView->model()->rowCount(); ++i)
    {
        QModelIndex idxRoot = treeView->model()->index(i, 0);
        for (int j = 0; j < treeView->model()->rowCount(idxRoot); ++j)
        {
            QModelIndex idxLeaf = treeView->model()->index(j, 0, idxRoot);
            if (treeView->isPersistentEditorOpen(idxLeaf))
                treeView->closePersistentEditor(idxLeaf);
            treeView->openPersistentEditor(idxLeaf);
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *e)
{
    if (watched->objectName() == "chartView")
    {
        if (e->type() == QEvent::Wheel)
        {
            QWheelEvent* event = static_cast<QWheelEvent*>(e);
            if (event->delta() > 0)
                chartView->chart()->zoomIn();
            else
                chartView->chart()->zoomOut();
            refreshEditors();
            event->accept();
            return true;
        }
        if (e->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* event = static_cast<QMouseEvent*>(e);
            if (event->button() == Qt::RightButton)
            {
                copyMenu->setVisible(true);
                copyMenu->move(QCursor::pos());
            }
            else
            {
                copyMenu->setVisible(false);
            }
            event->accept();
            return true;
        }
    }
    if (watched->objectName() == "treeView")
    {
        if (e->type() == QEvent::ContextMenu)
        {
            e->accept();

            QContextMenuEvent* event = static_cast<QContextMenuEvent*>(e);
            auto fileIndex = treeView->indexAt(event->pos());
            if (static_cast<ChartEditModel::Node*>(fileIndex.internalPointer())->type == ChartEditModel::NodeType::FileName)
            {
                deleteFileMenu->setVisible(true);
                deleteFileMenu->move(QCursor::pos());
                fileToBeRemoved = fileIndex.row() - 1;
            }
            else
            {
                deleteFileMenu->setVisible(false);
            }

            return true;
        }
    }

    return false;
}

void MainWindow::copyChart() const
{
    QPixmap p(chartView->size());
    QPainter painter(&p);
    painter.setRenderHint(QPainter::Antialiasing);
    chartView->render(&painter);
    QApplication::clipboard()->setPixmap(p);
}

void MainWindow::removeFile()
{
    static_cast<ChartEditModel*>(treeView->model())->removeFile(fileToBeRemoved);
}
