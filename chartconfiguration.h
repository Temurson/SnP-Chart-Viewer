#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QList>
#include <QColor>
#include <QTextStream>

struct FileInfo
{
    QString filePath;
    QString columns;
    int lineWidth;
    QColor lineColor;
    qreal multiplier;
};

struct ChartConfiguration
{
    QString chartTitle;
    QString xTitle;
    QString yTitle;
    qreal xMin;
    qreal xMax;
    qreal yMin;
    qreal yMax;
    int xGrid;
    int yGrid;
    bool legend;

    QList<FileInfo> files;

    static QByteArray toByteArray(ChartConfiguration config)
    {
        QByteArray result;
        QTextStream out(&result, QIODevice::WriteOnly);

        out << config.chartTitle << '\n'
            << config.xTitle << '\n'
            << config.yTitle << '\n'
            << config.xMin << '\n'
            << config.xMax << '\n'
            << config.yMin << '\n'
            << config.yMax << '\n'
            << config.xGrid << '\n'
            << config.yGrid << '\n'
            << config.legend << '\n';

        out << config.files.size() << '\n';
        foreach (FileInfo info, config.files)
        {
            out << info.filePath << '\n'
                << info.columns << '\n'
                << info.lineWidth << '\n'
                << info.lineColor.name() << '\n'
                << info.multiplier << '\n';
        }

        out.flush();

        return result;
    }

    static ChartConfiguration fromByteArray(QByteArray data)
    {
        ChartConfiguration config;

        QTextStream in(&data, QIODevice::ReadOnly);

        int legend;
        config.chartTitle = in.readLine();
        config.xTitle = in.readLine();
        config.yTitle = in.readLine();
        in >> config.xMin
           >> config.xMax
           >> config.yMin
           >> config.yMax
           >> config.xGrid
           >> config.yGrid
           >> legend;
        config.legend = static_cast<bool>(legend);

        int size;
        in >> size;
        for (int i = 0; i < size; ++i)
        {
            QString color;
            FileInfo fileInfo;
            fileInfo.filePath = in.readLine();
            fileInfo.columns = in.readLine();

            in >> color
               >> fileInfo.multiplier;
            fileInfo.lineColor = QColor(color);
            config.files.push_back(fileInfo);
        }

        return config;
    }
};

#endif // CONFIG_H
