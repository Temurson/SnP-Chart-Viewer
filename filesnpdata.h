#ifndef FILESNPDATA_H
#define FILESNPDATA_H

#include <QtCharts/QSplineSeries>
#include <QList>
#include <QVector>
#include <QString>
#include <QColor>
#include <QFile>
#include <QString>

#include <complex>
#include <utility>
#include <tuple>

class FileSNPData
{
// PRIVATE FIELDS
private:
    QString filePath;
    QStringList fileDescription;
    QString dataHeader;
    qreal z0;

    QVector<qreal> frequencies;
    QVector<QVector<std::complex<qreal>>> dataPoints;
    int dimension;

    QList<std::pair<int, int>> columns;
    int lineWidth;
    QColor lineColor;
    qreal multiplier;

// PUBLIC METHODS
public:
    FileSNPData(QString filePath_);

    QString getFilePath() const
    {
        return filePath;
    }

    QString getFileName() const;

    QStringList getFileDescription() const
    {
        return fileDescription;
    }

    QString getDataHeader()
    {
        return dataHeader;
    }

    int getDataSize() const
    {
        return frequencies.size();
    }

    qreal getZ0() const
    {
        return z0;
    }

    QList<std::pair<int, int>> getColumns() const
    {
        return columns;
    }
    void setColumns(QList<std::pair<int, int>> columns_)
    {
        columns = std::move(columns_);
    }

    int getLineWidth() const
    {
        return lineWidth;
    }
    void setLineWidth(int lineWidth_)
    {
        lineWidth = lineWidth_;
    }

    QColor getLineColor() const
    {
        return lineColor;
    }
    void setLineColor(QColor lineColor_)
    {
        lineColor = std::move(lineColor_);
    }

    qreal getMultiplier() const
    {
        return multiplier;
    }
    void setMultiplier(qreal multiplier_)
    {
        multiplier = multiplier_;
    }

    std::tuple<qreal, qreal, qreal, qreal, QList<QtCharts::QSplineSeries*>>
    getDrawableData() const;

// PRIVATE METHODS
private:
    QFile* openFile() const;

    // file is deleted after readData
    void readData(QFile* pfile);

    void setDefaultConfig();
};

#endif // FILESNPDATA_H
