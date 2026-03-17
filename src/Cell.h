#ifndef CELL_H
#define CELL_H

#include <QString>
#include <QColor>
#include <variant>

class Spreadsheet; // 前向声明

class Cell {
public:
    enum class Type {
        Empty,
        Integer,
        Float,
        String,
        Formula
    };

    // 格式设置相关的枚举
    enum class Alignment {
        Left,
        Center,
        Right
    };

    enum class NumberFormat {
        General,
        Number,
        Currency,
        Percentage,
        Date,
        Time
    };

    Cell(Spreadsheet *spreadsheet = nullptr);
    Cell(int value, Spreadsheet *spreadsheet = nullptr);
    Cell(double value, Spreadsheet *spreadsheet = nullptr);
    Cell(const QString &value, Spreadsheet *spreadsheet = nullptr);
    Cell(const QString &formula, bool isFormula, Spreadsheet *spreadsheet = nullptr);

    Type getType() const;
    QString getValue() const;
    QString getDisplayValue() const;
    void setValue(const QString &value);
    bool hasError() const;

    // 格式设置相关的方法
    void setFontBold(bool bold);
    bool isFontBold() const;
    
    void setFontSize(int size);
    int getFontSize() const;
    
    void setAlignment(Alignment alignment);
    Alignment getAlignment() const;
    
    void setNumberFormat(NumberFormat format);
    NumberFormat getNumberFormat() const;
    
    void setBackgroundColor(const QColor &color);
    QColor getBackgroundColor() const;

    // 设置Spreadsheet对象
    void setSpreadsheet(Spreadsheet *spreadsheet);

private:
    Type type;
    std::variant<int, double, QString> data;
    QString formula;
    bool error;
    Spreadsheet *spreadsheet;
    
    // 格式相关的成员变量
    bool fontBold;
    int fontSize;
    Alignment alignment;
    NumberFormat numberFormat;
    QColor backgroundColor;
    
    void parseValue(const QString &value);
    QString evaluateFormula() const;
    std::vector<double> parseRange(const QString &range) const;
};

#endif // CELL_H