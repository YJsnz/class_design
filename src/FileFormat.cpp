#include "FileFormat.h"
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QRegularExpression>

bool FileFormat::save(const QString &filename, const QMap<QString, Cell> &cells) {
    if (filename.endsWith(".dat", Qt::CaseInsensitive)) {
        return saveAsDat(filename, cells);
    } else if (filename.endsWith(".csv", Qt::CaseInsensitive)) {
        return saveAsCsv(filename, cells);
    } else if (filename.endsWith(".xlsx", Qt::CaseInsensitive)) {
        return saveAsXlsx(filename, cells);
    }
    return false;
}

bool FileFormat::load(const QString &filename, QMap<QString, Cell> &cells) {
    if (filename.endsWith(".dat", Qt::CaseInsensitive)) {
        return loadFromDat(filename, cells);
    } else if (filename.endsWith(".csv", Qt::CaseInsensitive)) {
        return loadFromCsv(filename, cells);
    } else if (filename.endsWith(".xlsx", Qt::CaseInsensitive)) {
        return loadFromXlsx(filename, cells);
    }
    return false;
}

bool FileFormat::saveAsDat(const QString &filename, const QMap<QString, Cell> &cells) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QDataStream out(&file);
    out << cells.size();
    
    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        out << it.key();
        out << static_cast<int>(it.value().getType());
        out << it.value().getValue();
    }
    
    file.close();
    return true;
}

bool FileFormat::saveAsCsv(const QString &filename, const QMap<QString, Cell> &cells) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    
    // 找出最大的行和列
    int maxRow = 0;
    int maxCol = 0;
    
    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        QString key = it.key();
        QString colName;
        int rowNum = 0;
        int i = 0;
        while (i < key.length() && key[i].isLetter()) {
            colName.append(key[i]);
            i++;
        }
        if (i < key.length()) {
            rowNum = key.mid(i).toInt() - 1;
        }
        
        int colNum = 0;
        for (QChar ch : colName) {
            colNum = colNum * 26 + (ch.toUpper().unicode() - 'A' + 1);
        }
        colNum--;
        
        if (rowNum > maxRow) maxRow = rowNum;
        if (colNum > maxCol) maxCol = colNum;
    }
    
    // 生成CSV文件
    for (int row = 0; row <= maxRow; ++row) {
        for (int col = 0; col <= maxCol; ++col) {
            QString key = cellKey(row, col);
            if (cells.contains(key)) {
                out << cells[key].getValue();
            }
            if (col < maxCol) {
                out << ",";
            }
        }
        out << "\n";
    }
    
    file.close();
    return true;
}

bool FileFormat::saveAsXlsx(const QString &filename, const QMap<QString, Cell> &cells) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream out(&file);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><worksheet name=\"Sheet1\"><sheetData>";
    
    // 找出最大的行
    int maxRow = 0;
    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        QString key = it.key();
        int i = 0;
        while (i < key.length() && key[i].isLetter()) {
            i++;
        }
        if (i < key.length()) {
            int rowNum = key.mid(i).toInt() - 1;
            if (rowNum > maxRow) maxRow = rowNum;
        }
    }
    
    for (int row = 0; row <= maxRow; ++row) {
        out << "<row r=\"" << (row + 1) << "\">";
        
        // 找出当前行的最大列
        int maxCol = 0;
        for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
            QString key = it.key();
            QString colName;
            int rowNum = 0;
            int i = 0;
            while (i < key.length() && key[i].isLetter()) {
                colName.append(key[i]);
                i++;
            }
            if (i < key.length()) {
                rowNum = key.mid(i).toInt() - 1;
                if (rowNum == row) {
                    int colNum = 0;
                    for (QChar ch : colName) {
                        colNum = colNum * 26 + (ch.toUpper().unicode() - 'A' + 1);
                    }
                    colNum--;
                    if (colNum > maxCol) maxCol = colNum;
                }
            }
        }
        
        for (int col = 0; col <= maxCol; ++col) {
            QString key = cellKey(row, col);
            if (cells.contains(key)) {
                out << "<c r=\"" << key << "\"><v>" << cells[key].getValue() << "</v></c>";
            }
        }
        
        out << "</row>";
    }
    
    out << "</sheetData></worksheet></workbook>";
    file.close();
    return true;
}

bool FileFormat::loadFromDat(const QString &filename, QMap<QString, Cell> &cells) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QDataStream in(&file);
    int size;
    in >> size;
    
    cells.clear();
    for (int i = 0; i < size; ++i) {
        QString key;
        int type;
        QString value;
        in >> key >> type >> value;
        
        Cell cell(value);
        cells[key] = cell;
    }
    
    file.close();
    return true;
}

bool FileFormat::loadFromCsv(const QString &filename, QMap<QString, Cell> &cells) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    cells.clear();
    
    int row = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(",");
        
        for (int col = 0; col < parts.size(); ++col) {
            QString value = parts[col].trimmed();
            if (!value.isEmpty()) {
                QString key = cellKey(row, col);
                Cell cell(value);
                cells[key] = cell;
            }
        }
        
        row++;
    }
    
    file.close();
    return true;
}

bool FileFormat::loadFromXlsx(const QString &filename, QMap<QString, Cell> &cells) {
    // 简单的XML解析，实际应用中可能需要更复杂的解析
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    cells.clear();
    
    // 查找所有单元格
    QRegularExpression re("<c r=\"([A-Z0-9]+)\"><v>(.*?)</v></c>");
    QRegularExpressionMatchIterator it = re.globalMatch(content);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);
        
        Cell cell(value);
        cells[key] = cell;
    }
    
    return true;
}

QString FileFormat::cellKey(int row, int column) {
    QString colName;
    int col = column;
    while (col >= 0) {
        colName.prepend(QChar('A' + (col % 26)));
        col = (col / 26) - 1;
    }
    return colName + QString::number(row + 1);
}