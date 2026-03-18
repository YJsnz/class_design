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
        // 保存当前位置，用于判断是函数还是单元格引用
        int startPos = pos;
        
        // 解析标识符
        QString ident = parseIdentifier();
        QString lowerIdent = ident.toLower();
        
        // 检查是否是函数（后面跟着'('）
        skipWhitespace();
        if (pos < expr.length() && expr[pos] == '(') {
            // 是函数
            if (lowerIdent == "sqrt") {
                return sqrt(parseFunction());
            } else if (lowerIdent == "abs") {
                return fabs(parseFunction());
            } else if (lowerIdent == "sin") {
                return sin(parseFunction());
            } else if (lowerIdent == "cos") {
                return cos(parseFunction());
            } else if (lowerIdent == "tan") {
                return tan(parseFunction());
            } else if (lowerIdent == "asin") {
                return asin(parseFunction());
            } else if (lowerIdent == "acos") {
                return acos(parseFunction());
            } else if (lowerIdent == "atan") {
                return atan(parseFunction());
            } else if (lowerIdent == "exp") {
                return exp(parseFunction());
            } else if (lowerIdent == "log") {
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                double value = parseExpression();
                skipWhitespace();
                if (expr[pos] == ',') {
                    // 两个参数：value, base
                    pos++;
                    double base = parseExpression();
                    skipWhitespace();
                    if (expr[pos] != ')') {
                        setError();
                        return 0;
                    }
                    pos++;
                    return log(value) / log(base);
                } else {
                    // 一个参数：value（自然对数）
                    if (expr[pos] != ')') {
                        setError();
                        return 0;
                    }
                    pos++;
                    return log(value);
                }
            } else if (lowerIdent == "log10") {
                return log10(parseFunction());
            } else if (lowerIdent == "pow" || lowerIdent == "power") {
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
            } else if (lowerIdent == "round") {
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
            } else if (lowerIdent == "ceil") {
                return ceil(parseFunction());
            } else if (lowerIdent == "floor") {
                return floor(parseFunction());
            } else if (lowerIdent == "len") {
                // LEN函数：返回字符串长度
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                // 解析字符串参数
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.length();
            } else if (lowerIdent == "concat") {
                // CONCAT函数：连接字符串
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString result = "";
                while (pos < expr.length()) {
                    skipWhitespace();
                    if (expr[pos] == '"') {
                        pos++;
                        QString str = "";
                        while (pos < expr.length() && expr[pos] != '"') {
                            str.append(expr[pos]);
                            pos++;
                        }
                        if (pos < expr.length()) pos++;
                        result.append(str);
                    }
                    skipWhitespace();
                    if (expr[pos] == ',') {
                        pos++;
                    } else if (expr[pos] == ')') {
                        pos++;
                        break;
                    } else {
                        setError();
                        return 0;
                    }
                }
                return result.length();
            } else if (lowerIdent == "left") {
                // LEFT函数：返回字符串左边的指定长度字符
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                int length = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.left(length).length();
            } else if (lowerIdent == "right") {
                // RIGHT函数：返回字符串右边的指定长度字符
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                int length = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.right(length).length();
            } else if (lowerIdent == "mid") {
                // MID函数：返回字符串中间的指定长度字符
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                int start = parseExpression() - 1; // Excel中索引从1开始
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                int length = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.mid(start, length).length();
            } else if (lowerIdent == "upper") {
                // UPPER函数：转换为大写
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.toUpper().length();
            } else if (lowerIdent == "lower") {
                // LOWER函数：转换为小写
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return str.toLower().length();
            } else if (lowerIdent == "proper") {
                // PROPER函数：转换为首字母大写
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                QString str = "";
                if (expr[pos] == '"') {
                    pos++;
                    while (pos < expr.length() && expr[pos] != '"') {
                        str.append(expr[pos]);
                        pos++;
                    }
                    if (pos < expr.length()) pos++;
                }
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                // 简单实现：首字母大写，其他小写
                if (str.isEmpty()) return 0;
                QString result = str.left(1).toUpper() + str.mid(1).toLower();
                return result.length();
            } else if (lowerIdent == "if") {
                // IF函数：IF(condition, value_if_true, value_if_false)
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                double condition = parseExpression();
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                double value_if_true = parseExpression();
                skipWhitespace();
                if (expr[pos] != ',') {
                    setError();
                    return 0;
                }
                pos++;
                double value_if_false = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return condition != 0 ? value_if_true : value_if_false;
            } else if (lowerIdent == "and") {
                // AND函数：AND(condition1, condition2, ...)
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                bool result = true;
                while (pos < expr.length()) {
                    skipWhitespace();
                    if (expr[pos] == ')') {
                        pos++;
                        break;
                    }
                    double condition = parseExpression();
                    if (condition == 0) {
                        result = false;
                    }
                    skipWhitespace();
                    if (expr[pos] == ',') {
                        pos++;
                    } else if (expr[pos] == ')') {
                        pos++;
                        break;
                    } else {
                        setError();
                        return 0;
                    }
                }
                return result ? 1 : 0;
            } else if (lowerIdent == "or") {
                // OR函数：OR(condition1, condition2, ...)
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                bool result = false;
                while (pos < expr.length()) {
                    skipWhitespace();
                    if (expr[pos] == ')') {
                        pos++;
                        break;
                    }
                    double condition = parseExpression();
                    if (condition != 0) {
                        result = true;
                    }
                    skipWhitespace();
                    if (expr[pos] == ',') {
                        pos++;
                    } else if (expr[pos] == ')') {
                        pos++;
                        break;
                    } else {
                        setError();
                        return 0;
                    }
                }
                return result ? 1 : 0;
            } else if (lowerIdent == "not") {
                // NOT函数：NOT(condition)
                skipWhitespace();
                if (expr[pos] != '(') {
                    setError();
                    return 0;
                }
                pos++;
                double condition = parseExpression();
                skipWhitespace();
                if (expr[pos] != ')') {
                    setError();
                    return 0;
                }
                pos++;
                return condition == 0 ? 1 : 0;
            } else if (lowerIdent == "sum" || lowerIdent == "avg" || lowerIdent == "average" || lowerIdent == "max" || lowerIdent == "min" || lowerIdent == "count" || lowerIdent == "counta" || lowerIdent == "median" || lowerIdent == "mode" || lowerIdent == "stdev" || lowerIdent == "var") {
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
                
                if (lowerIdent == "sum") {
                    double sum = 0;
                    for (double val : values) {
                        sum += val;
                    }
                    return sum;
                } else if (lowerIdent == "avg") {
                    double sum = 0;
                    for (double val : values) {
                        sum += val;
                    }
                    return sum / values.size();
                } else if (lowerIdent == "max") {
                    double maxVal = values[0];
                    for (double val : values) {
                        if (val > maxVal) maxVal = val;
                    }
                    return maxVal;
                } else if (lowerIdent == "min") {
                    double minVal = values[0];
                    for (double val : values) {
                        if (val < minVal) minVal = val;
                    }
                    return minVal;
                } else if (lowerIdent == "count") {
                    return values.size();
                } else if (lowerIdent == "counta") {
                    return values.size(); // 简化实现，与COUNT相同
                } else if (lowerIdent == "median") {
                    std::sort(values.begin(), values.end());
                    int size = values.size();
                    if (size % 2 == 0) {
                        return (values[size/2 - 1] + values[size/2]) / 2;
                    } else {
                        return values[size/2];
                    }
                } else if (lowerIdent == "mode") {
                    // 简化实现，返回第一个出现次数最多的值
                    if (values.empty()) return 0;
                    std::map<double, int> frequency;
                    for (double val : values) {
                        frequency[val]++;
                    }
                    int maxFreq = 0;
                    double mode = values[0];
                    for (auto &pair : frequency) {
                        if (pair.second > maxFreq) {
                            maxFreq = pair.second;
                            mode = pair.first;
                        }
                    }
                    return mode;
                } else if (lowerIdent == "stdev") {
                    // 计算标准差
                    if (values.size() <= 1) return 0;
                    double mean = 0;
                    for (double val : values) {
                        mean += val;
                    }
                    mean /= values.size();
                    double variance = 0;
                    for (double val : values) {
                        variance += pow(val - mean, 2);
                    }
                    variance /= (values.size() - 1);
                    return sqrt(variance);
                } else if (lowerIdent == "var") {
                    // 计算方差
                    if (values.size() <= 1) return 0;
                    double mean = 0;
                    for (double val : values) {
                        mean += val;
                    }
                    mean /= values.size();
                    double variance = 0;
                    for (double val : values) {
                        variance += pow(val - mean, 2);
                    }
                    variance /= (values.size() - 1);
                    return variance;
                }
                return 0;
            } else {
                // 不是函数，是单元格引用
                pos = startPos; // 重置pos到起始位置
                return parseCellReference();
            }
        } else {
            // 不是函数，是单元格引用
            pos = startPos; // 重置pos到起始位置
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