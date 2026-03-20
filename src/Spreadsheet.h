#ifndef SPREADSHEET_H
#define SPREADSHEET_H

#include <QMainWindow>
#include <QTableWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QComboBox>
#include <QInputDialog>
#include <QSet>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialog>
#include <QColorDialog>
#include <QStackedWidget>
#include <QTabWidget>
#include <QSlider>
#include <QWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QNetworkAccessManager>
#include "Cell.h"

// 前向声明
class QChart;
class QChartView;
class QBarSeries;
class QBarSet;
class QLineSeries;
class QPieSeries;
class QValueAxis;
class QCategoryAxis;

class Spreadsheet : public QMainWindow
{
    Q_OBJECT

public:
    Spreadsheet(QWidget *parent = nullptr);
    ~Spreadsheet();

private slots:
        void onCellChanged(int row, int column);
        void openFile();
        void saveFile();
        void saveAsFile();
        void newFile();
        void onBoldClicked();
        void onFontSizeChanged(const QString &size);
        void onAlignClicked(int alignment);
        void onSortAscending();
        void onSortDescending();
        void onFilter();
        void onFind();
        void onReplace();
        void onFormatBrushClicked();
        void onCellClickedForFormatBrush(int row, int column);
        void onClearClicked();
        void onFontChanged(const QString &font);
        void onItalicClicked();
        void onUnderlineClicked();
        void onFontColorClicked();
        void onCellColorClicked();
        void onTabChanged(int index);
        void onZoomChanged(int value);
        void onCellSelected(int row, int column);
        void onFormulaChanged(const QString &text);
        void onCellAddressChanged(const QString &address);
        void onMergeCells();
        void onInsertChart();
        void onInsertRowAbove();
        void onInsertRowBelow();
        void onInsertColumnLeft();
        void onInsertColumnRight();
        void onInsertFunction();
        void onHelpClicked();
        void onAIAssistantClicked();

public:
    // 为FormulaParser提供的方法
    double getCellNumericValue(int row, const QString &colName);
    std::vector<double> getRangeNumericValues(int startRow, const QString &startCol, int endRow, const QString &endCol);

private:
    QVector<QTableWidget*> tableWidgets;
    QVector<QPushButton*> sheetButtons;
    QAction *openAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *newAction;
    QAction *formatBrushAction;
    QString currentFile;
    QFont formatBrush;
    bool formatBrushActive;
    QComboBox *fontSizeComboBox;
    QSlider *zoomSlider;
    QLabel *zoomValueLabel;
    int currentZoom;
    QWidget *formulaBarWidget;
    QComboBox *cellAddressComboBox;
    QLineEdit *formulaEdit;
    QLabel *formulaLabel;
    QSet<QString> calculatingCells;
    QNetworkAccessManager *networkManager;
    
    void setupUI();
    void setupMenus();
    void setupFormulaBar();
    void loadFromFile(const QString &filename);
    void saveToFile(const QString &filename);
    void saveAsExcel(const QString &filename);
    QString columnName(int column) const;
    int columnIndex(const QString &name) const;
    QTableWidget* createTableWidget();
    QTableWidget* getCurrentTable();
    void updateSheetButtons();
    bool hasCycle(const QString &cellAddress, const QString &formula);
    bool checkCycle(const QString &cellAddress, const QString &formula, QMap<QString, QString> &editingCells);
    bool isCellInRange(const QString &cellAddress, const QString &startCell, const QString &endCell);
};

#endif // SPREADSHEET_H