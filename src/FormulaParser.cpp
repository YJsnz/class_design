#include "FormulaParser.h"
#include "Spreadsheet.h"
#include <cmath>
#include <QDebug>

FormulaParser::FormulaParser(const QString &expression, Spreadsheet *spreadsheet) 
    : expr(expression), pos(0), error(false), spreadsheet(spreadsheet) {}

double FormulaParser::evaluate() {
    pos = 0;
    error = false;
    double result = parseExpression();
    if (pos < expr.length()) {
        setError();
    }
    return result;
}

bool FormulaParser::hasError() const {
    return error;
}

double FormulaParser::parseExpression() {
    double result = parseTerm();
    while (pos < expr.length()) {
        skipWhitespace();
        if (expr[pos] == '+') {
            pos++;
            result += parseTerm();
        } else if (expr[pos] == '-') {
            pos++;
            result -= parseTerm();
        } else {
            break;
        }
    }
    return result;
}

double FormulaParser::parseTerm() {
    double result = parseFactor();
    while (pos < expr.length()) {
        skipWhitespace();
        if (expr[pos] == '*') {
            pos++;
            result *= parseFactor();
        } else if (expr[pos] == '/') {
            pos++;
            double divisor = parseFactor();
            if (divisor == 0) {
                setError();
                return 0;
            }
            result /= divisor;
        } else if (expr[pos] == '%') {
            pos++;
            double divisor = parseFactor();
            if (divisor == 0) {
                setError();
                return 0;
            }
            result = fmod(result, divisor);
        } else {
            break;
        }
    }
    return result;
}

double FormulaParser::parseFactor() {
    skipWhitespace();
    if (expr[pos] == '-') {
        pos++;
        return -parsePrimary();
    } else if (expr[pos] == '+') {
        pos++;
        return parsePrimary();
    } else {
        return parsePrimary();
    }
}

double FormulaParser::parsePrimary() {
    skipWhitespace();
    if (expr[pos] == '(') {
        pos++;
        double result = parseExpression();
        skipWhitespace();
        if (expr[pos] != ')') {
            setError();
            return 0;
        }
        pos++;
        return result;
    } else if (expr[pos].isLetter()) {
        QString ident = parseIdentifier();
        if (ident == "sqrt") {
            return sqrt(parseFunction());
        } else if (ident == "abs") {
            return fabs(parseFunction());
        } else if (ident == "sin") {
            return sin(parseFunction());
        } else if (ident == "cos") {
            return cos(parseFunction());
        } else if (ident == "tan") {
            return tan(parseFunction());
        } else if (ident == "asin") {
            return asin(parseFunction());
        } else if (ident == "acos") {
            return acos(parseFunction());
        } else if (ident == "atan") {
            return atan(parseFunction());
        } else if (ident == "exp") {
            return exp(parseFunction());
        } else if (ident == "log") {
            return log(parseFunction());
        } else if (ident == "log10") {
            return log10(parseFunction());
        } else if (ident == "pow") {
            skipWhitespace();
            if (expr[pos] != '(') {
                setError();
                return 0;
            }
            pos++;
            double base = parseExpression();
            skipWhitespace();
            if (expr[pos] != ',') {
                setError();
                return 0;
            }
            pos++;
            double exponent = parseExpression();
            skipWhitespace();
            if (expr[pos] != ')') {
                setError();
                return 0;
            }
            pos++;
            return pow(base, exponent);
        } else if (ident == "round") {
            skipWhitespace();
            if (expr[pos] != '(') {
                setError();
                return 0;
            }
            pos++;
            double value = parseExpression();
            skipWhitespace();
            if (expr[pos] == ',') {
                pos++;
                double decimals = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                double factor = pow(10, decimals);
                return round(value * factor) / factor;
            } else {
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return round(value);
            }
        } else if (ident == "ceil") {
            return ceil(parseFunction());
        } else if (ident == "floor") {
            return floor(parseFunction());
        } else if (ident == "sum" || ident == "avg" || ident == "max" || ident == "min" || ident == "count") {
            skipWhitespace();
            if (expr[pos] != '(') {
                setError();
                return 0;
            }
            pos++;
            
            // 解析参数列表
            std::vector<double> values;
            while (pos < expr.length()) {
                skipWhitespace();
                
                // 如果是数字，直接解析
                if (expr[pos].isDigit() || expr[pos] == '.') {
                    values.push_back(parseNumber());
                } else if (expr[pos].isLetter()) {
                    // 解析单元格引用或区域引用
                    int start = pos;
                    while (pos < expr.length() && (expr[pos].isLetter() || expr[pos].isDigit() || expr[pos] == ':')) {
                        pos++;
                    }
                    QString ref = expr.mid(start, pos - start);
                    
                    if (ref.contains(':')) {
                        // 区域引用
                        std::vector<double> rangeValues = getRangeValues(ref);
                        values.insert(values.end(), rangeValues.begin(), rangeValues.end());
                    } else {
                        // 单个单元格引用
                        values.push_back(getCellValue(ref));
                    }
                } else {
                    break;
                }
                
                skipWhitespace();
                if (expr[pos] == ',') {
                    pos++;
                } else if (expr[pos] == ')') {
                    break;
                }
            }
            
            skipWhitespace();
            if (expr[pos] != ')') {
                setError();
                return 0;
            }
            pos++;
            
            // 计算函数结果
            if (values.empty()) {
                return 0;
            }
            
            if (ident == "sum") {
                double sum = 0;
                for (double val : values) {
                    sum += val;
                }
                return sum;
            } else if (ident == "avg") {
                double sum = 0;
                for (double val : values) {
                    sum += val;
                }
                return sum / values.size();
            } else if (ident == "max") {
                double maxVal = values[0];
                for (double val : values) {
                    if (val > maxVal) maxVal = val;
                }
                return maxVal;
            } else if (ident == "min") {
                double minVal = values[0];
                for (double val : values) {
                    if (val < minVal) minVal = val;
                }
                return minVal;
            } else if (ident == "count") {
                return values.size();
            }
            return 0;
        } else {
            return parseCellReference();
        }
    } else if (expr[pos].isDigit() || expr[pos] == '.') {
        return parseNumber();
    } else {
        setError();
        return 0;
    }
}

double FormulaParser::parseFunction() {
    skipWhitespace();
    if (expr[pos] != '(') {
        setError();
        return 0;
    }
    pos++;
    double result = parseExpression();
    skipWhitespace();
    if (expr[pos] != ')') {
        setError();
        return 0;
    }
    pos++;
    return result;
}

double FormulaParser::parseCellReference() {
    // 解析单元格引用或区域引用
    int start = pos;
    while (pos < expr.length() && (expr[pos].isLetter() || expr[pos].isDigit() || expr[pos] == ':')) {
        pos++;
    }
    QString ref = expr.mid(start, pos - start);
    
    // 检查是否是区域引用（包含冒号）
    if (ref.contains(':')) {
        // 区域引用，暂时返回0，后续会由表函数处理
        return 0;
    } else {
        // 单个单元格引用
        return getCellValue(ref);
    }
}

double FormulaParser::getCellValue(const QString &cellRef) {
    if (!spreadsheet) {
        return 0; // 如果没有spreadsheet引用，返回0
    }
    
    // 解析单元格引用，如"A1" -> 行0，列0
    QString colName;
    int rowNum = 0;
    int i = 0;
    while (i < cellRef.length() && cellRef[i].isLetter()) {
        colName.append(cellRef[i]);
        i++;
    }
    if (i < cellRef.length()) {
        rowNum = cellRef.mid(i).toInt() - 1;
    }
    
    // 调用spreadsheet的方法获取单元格值
    // 这里需要Spreadsheet类提供获取单元格值的方法
    return spreadsheet->getCellNumericValue(rowNum, colName);
}

std::vector<double> FormulaParser::getRangeValues(const QString &rangeRef) {
    std::vector<double> values;
    if (!spreadsheet || !rangeRef.contains(':')) {
        return values;
    }
    
    // 解析区域引用，如"A1:B3"
    QStringList parts = rangeRef.split(':');
    if (parts.size() != 2) {
        return values;
    }
    
    QString startCell = parts[0];
    QString endCell = parts[1];
    
    // 解析起始和结束单元格
    QString startCol;
    int startRow = 0;
    int i = 0;
    while (i < startCell.length() && startCell[i].isLetter()) {
        startCol.append(startCell[i]);
        i++;
    }
    if (i < startCell.length()) {
        startRow = startCell.mid(i).toInt() - 1;
    }
    
    QString endCol;
    int endRow = 0;
    i = 0;
    while (i < endCell.length() && endCell[i].isLetter()) {
        endCol.append(endCell[i]);
        i++;
    }
    if (i < endCell.length()) {
        endRow = endCell.mid(i).toInt() - 1;
    }
    
    // 获取区域内的所有单元格值
    return spreadsheet->getRangeNumericValues(startRow, startCol, endRow, endCol);
}

double FormulaParser::parseNumber() {
    int start = pos;
    while (pos < expr.length() && (expr[pos].isDigit() || expr[pos] == '.')) {
        pos++;
    }
    QString numStr = expr.mid(start, pos - start);
    bool ok;
    double num = numStr.toDouble(&ok);
    if (!ok) {
        setError();
        return 0;
    }
    return num;
}

QString FormulaParser::parseIdentifier() {
    int start = pos;
    while (pos < expr.length() && expr[pos].isLetter()) {
        pos++;
    }
    return expr.mid(start, pos - start);
}

void FormulaParser::skipWhitespace() {
    while (pos < expr.length() && expr[pos].isSpace()) {
        pos++;
    }
}

bool FormulaParser::match(char c) {
    skipWhitespace();
    if (pos < expr.length() && expr[pos] == c) {
        pos++;
        return true;
    }
    return false;
}

void FormulaParser::setError() {
    error = true;
}