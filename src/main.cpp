#include <QApplication>
#include "Spreadsheet.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Spreadsheet w;
    w.show();
    return a.exec();
}