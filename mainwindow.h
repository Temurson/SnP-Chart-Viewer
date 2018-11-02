#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

#include <QList>
class QWidget;
class QMenu;

#include "filesnpdata.h"

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

// PUBLIC METHODS
public:
    MainWindow(QWidget* pwgt = 0);

// PRIVATE METHODS
private:
    // creates empty QChart and ChartView
    void setupChart();

    // creates Configuration node in the TreeView
    void setupConfigNode();

    void saveConfig();

    void loadConfig();

    void clearAll();

    void refreshEditors();

    bool eventFilter(QObject* watched, QEvent* event);

    void copyChart() const;
    QMenu* copyMenu;
    QMenu* deleteFileMenu;
    int fileToBeRemoved;

    void removeFile();

public slots:
    // opens file selection window and reads the file
    // called when clicking on "AddFile" button
    void addFile();
};

#endif // MAINWINDOW_H
