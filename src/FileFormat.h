#ifndef FILEFORMAT_H
#define FILEFORMAT_H

#include <QString>
#include <QMap>
#include "Cell.h"

class FileFormat {
public:
    static bool save(const QString &filename, const QMap<QString, Cell> &cells);
    static bool load(const QString &filename, QMap<QString, Cell> &cells);

private:
    static QString cellKey(int row, int column);
    static bool saveAsDat(const QString &filename, const QMap<QString, Cell> &cells);
    static bool saveAsCsv(const QString &filename, const QMap<QString, Cell> &cells);
    static bool saveAsXlsx(const QString &filename, const QMap<QString, Cell> &cells);
    static bool loadFromDat(const QString &filename, QMap<QString, Cell> &cells);
    static bool loadFromCsv(const QString &filename, QMap<QString, Cell> &cells);
    static bool loadFromXlsx(const QString &filename, QMap<QString, Cell> &cells);
};

#endif // FILEFORMAT_H