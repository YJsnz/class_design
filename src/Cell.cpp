#include "Cell.h"
#include "FormulaParser.h"
#include "Spreadsheet.h"
#include <variant>
#include <QColor>

Cell::Cell(Spreadsheet *spreadsheet) : type(Type::Empty), error(false), spreadsheet(spreadsheet), fontBold(false), fontSize(12), alignment(Alignment::Left), numberFormat(NumberFormat::General), backgroundColor(Qt::white) {}
Cell::Cell(int value, Spreadsheet *spreadsheet) : type(Type::Integer), data(value), error(false), spreadsheet(spreadsheet), fontBold(false), fontSize(12), alignment(Alignment::Left), numberFormat(NumberFormat::General), backgroundColor(Qt::white) {}

Cell::Cell(double value, Spreadsheet *spreadsheet) : type(Type::Float), data(value), error(false), spreadsheet(spreadsheet), fontBold(false), fontSize(12), alignment(Alignment::Left), numberFormat(NumberFormat::General), backgroundColor(Qt::white) {}

Cell::Cell(const QString &value, Spreadsheet *spreadsheet) : error(false), spreadsheet(spreadsheet), fontBold(false), fontSize(12), alignment(Alignment::Left), numberFormat(NumberFormat::General), backgroundColor(Qt::white) {
    parseValue(value);
}

Cell::Cell(const QString &formula, bool isFormula, Spreadsheet *spreadsheet) : type(Type::Formula), formula(formula), error(false), spreadsheet(spreadsheet), fontBold(false), fontSize(12), alignment(Alignment::Left), numberFormat(NumberFormat::General), backgroundColor(Qt::white) {}

Cell::Type Cell::getType() const {
    return type;
}

QString Cell::getValue() const {
    switch (type) {
    case Type::Empty:
        return "";
    case Type::Integer:
        return QString::number(std::get<int>(data));
    case Type::Float:
        return QString::number(std::get<double>(data));
    case Type::String:
        return std::get<QString>(data);
    case Type::Formula:
        return formula;
    default:
        return "";
    }
}

QString Cell::getDisplayValue() const {
    if (error) {
        return "#NA";
    }
    
    switch (type) {
    case Type::Empty:
        return "";
    case Type::Integer:
        return QString::number(std::get<int>(data));
    case Type::Float:
        return QString::number(std::get<double>(data));
    case Type::String:
        return std::get<QString>(data);
    case Type::Formula:
        return evaluateFormula();
    default:
        return "";
    }
}

void Cell::setValue(const QString &value) {
    parseValue(value);
}

bool Cell::hasError() const {
    return error;
}

void Cell::parseValue(const QString &value) {
    if (value.isEmpty()) {
        type = Type::Empty;
        return;
    }
    
    if (value.startsWith('=')) {
        type = Type::Formula;
        formula = value;
        error = false;
        return;
    }
    
    bool ok;
    int intValue = value.toInt(&ok);
    if (ok) {
        type = Type::Integer;
        data = intValue;
        error = false;
        return;
    }
    
    double doubleValue = value.toDouble(&ok);
    if (ok) {
        type = Type::Float;
        data = doubleValue;
        error = false;
        return;
    }
    
    type = Type::String;
    data = value;
    error = false;
}

QString Cell::evaluateFormula() const {
    if (formula.isEmpty()) {
        return "";
    }
    
    // 使用Spreadsheet对象来获取单元格值
    FormulaParser parser(formula.mid(1), spreadsheet); // 去掉开头的'='
    double result = parser.evaluate();
    if (parser.hasError()) {
        return "#NA";
    }
    
    return QString::number(result);
}

// 格式设置相关的方法实现
void Cell::setFontBold(bool bold) {
    fontBold = bold;
}

bool Cell::isFontBold() const {
    return fontBold;
}

void Cell::setFontSize(int size) {
    fontSize = size;
}

int Cell::getFontSize() const {
    return fontSize;
}

void Cell::setAlignment(Alignment alignment) {
    this->alignment = alignment;
}

Cell::Alignment Cell::getAlignment() const {
    return alignment;
}

void Cell::setNumberFormat(NumberFormat format) {
    numberFormat = format;
}

Cell::NumberFormat Cell::getNumberFormat() const {
    return numberFormat;
}

void Cell::setBackgroundColor(const QColor &color) {
    backgroundColor = color;
}

QColor Cell::getBackgroundColor() const {
    return backgroundColor;
}

void Cell::setSpreadsheet(Spreadsheet *spreadsheet) {
    this->spreadsheet = spreadsheet;
}

// 解析区域引用，返回区域内的所有单元格值
std::vector<double> Cell::parseRange(const QString &range) const {
    std::vector<double> values;
    // 简单实现，实际应用中需要根据区域获取单元格值
    return values;
}