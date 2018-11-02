#include "filesnpdata.h"

#include <QRegExp>
#include <QTextStream>
#include <QDir>

#include <stdexcept>
#include <limits>

#include "mainwindow.h"

QT_CHARTS_USE_NAMESPACE

FileSNPData::FileSNPData(QString filePath_)
    : filePath(filePath_)
{
    QFile* pfile = openFile();
    readData(pfile);
    // pfile is deleted after readData
    setDefaultConfig();
}

QString FileSNPData::getFileName() const
{
    return filePath.mid(filePath.lastIndexOf('/') + 1);
}

QFile* FileSNPData::openFile() const
{
    // RegExp for finding file format
    QRegExp formatRegExp = QRegExp("\\.s\\d+p$");
    if (!filePath.contains(formatRegExp))
        throw std::domain_error("Incorrect file format. Please open files with format \".sNp\" only.");

    QFile* pdataFile = new QFile(filePath);
    if (!pdataFile->open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ExistingOnly))
        throw std::runtime_error(pdataFile->errorString().toStdString());

    return pdataFile;
}

void FileSNPData::readData(QFile* pfile)
{
    QTextStream in(pfile);

    // find N in .sNp
    QRegExp formatRegExp = QRegExp("\\.s(\\d+)p$");
    formatRegExp.indexIn(filePath);
    dimension = formatRegExp.cap(1).toInt();

    const auto isFirstCharEqual =
    [](QTextStream& in, char c)
    {
        // device is used because QTextStream
        // does not have any convenient way of reading
        // chars without removing them from the stream
        char firstChar;
        in.device()->getChar(&firstChar);
        in.device()->ungetChar(firstChar);
        return firstChar == c;
    };

    // read file description
    forever
    {
        if (isFirstCharEqual(in, '!'))
            fileDescription.push_back(in.device()->readLine());
        else
            break;
    }

    // read data header
    if (isFirstCharEqual(in, '#'))
        dataHeader = in.device()->readLine();

    // parse z0 value from header
    {
    QTextStream headerReader(&dataHeader);
    QString s;
    headerReader >> s >> s >> s >> s >> s >> z0;
    }

    // discard other comments
    forever
    {
        if (isFirstCharEqual(in, '!'))
            in.device()->readLine();
        else
            break;
    }

    dataPoints.resize(dimension * dimension);
    // read the data
    // it is assumed that no comments are present after this point
    forever
    {
        qreal freq, real, imag;
        in >> freq;
        // checking end of file in while condition
        // may not work if there are space symbols
        // after the last meaningful line
        if (in.atEnd())
            break;
        frequencies.push_back(freq);
        for (int i = 0; i < dimension * dimension; ++i)
        {
            in >> real >> imag;
            dataPoints[i].push_back({real, imag});
        }
    }

    pfile->close();
    delete pfile;
}

void FileSNPData::setDefaultConfig()
{
    lineWidth = 1;
    multiplier = 1;
}

std::tuple<qreal, qreal, qreal, qreal, QList<QtCharts::QSplineSeries*>>
FileSNPData::getDrawableData() const
{
    QList<QSplineSeries*> result;
    qreal xMin, xMax, yMin, yMax;
    xMin = yMin = std::numeric_limits<qreal>::max();
    xMax = yMax = std::numeric_limits<qreal>::min();

    for (const auto& column : columns)
    {
        QSplineSeries* pseries = new QSplineSeries;
        int idx = (column.first - 1) * dimension + (column.second - 1);
        for (int i = 0; i < getDataSize(); ++i)
        {
            // take only real part for now
            // TODO use multiplier
            pseries->append(QPointF(frequencies[i], dataPoints[idx][i].real()));
            xMin = qMin(xMin, frequencies[i]);
            xMax = qMax(xMax, frequencies[i]);
            yMin = qMin(yMin, dataPoints[idx][i].real());
            yMax = qMax(yMax, dataPoints[idx][i].real());
        }
        result.push_back(pseries);
    }

    return std::make_tuple(xMin, xMax, yMin, yMax, result);
}
