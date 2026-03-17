#ifndef FORMULAPARSER_H
#define FORMULAPARSER_H

#include <QString>
#include <vector>

class Spreadsheet; // 前向声明

class FormulaParser
{
public:
    FormulaParser(const QString &expression, Spreadsheet *spreadsheet = nullptr);
    double evaluate();
    bool hasError() const;

private:
    double parseExpression();
    double parseTerm();
    double parseFactor();
    double parsePrimary();
    double parseFunction();
    double parseCellReference();
    double parseNumber();
    QString parseIdentifier();
    void skipWhitespace();
    bool match(char c);
    void setError();
    double getCellValue(const QString &cellRef);
    std::vector<double> getRangeValues(const QString &rangeRef);

    QString expr;
    int pos;
    bool error;
    Spreadsheet *spreadsheet;
};

#endif // FORMULAPARSER_H