#include "Spreadsheet.h"
#include "FileFormat.h"
#include "FormulaParser.h"
#include <algorithm>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QCategoryAxis>

Spreadsheet::Spreadsheet(QWidget *parent)
    : QMainWindow(parent), formatBrushActive(false), currentZoom(100)
{
    setupUI();
    setupMenus();
}

Spreadsheet::~Spreadsheet()
{
}

QTableWidget* Spreadsheet::createTableWidget()
{
    // 设置为50行，26列（A-Z），后续可动态扩展
    int rows = 50;
    int columns = 26;
    QTableWidget *table = new QTableWidget(rows, columns);
    
    // 动态生成列名
    QStringList headerLabels;
    for (int i = 0; i < columns; ++i) {
        headerLabels.append(columnName(i));
    }
    table->setHorizontalHeaderLabels(headerLabels);
    
    // 设置所有行的行号
    for (int i = 0; i < rows; ++i) {
        table->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i + 1)));
    }
    
    connect(table, &QTableWidget::cellChanged, this, &Spreadsheet::onCellChanged);
    connect(table, &QTableWidget::cellClicked, this, &Spreadsheet::onCellSelected);
    connect(table, &QTableWidget::currentCellChanged, this, &Spreadsheet::onCellSelected);
    
    return table;
}

void Spreadsheet::setupUI()
{
    // 创建主窗口部件
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 添加公式栏
    setupFormulaBar();
    mainLayout->addWidget(formulaBarWidget);
    
    // 初始显示第一个表格
    QStackedWidget *stackedWidget = new QStackedWidget(this);
    
    // 创建3个表格页面
    for (int i = 0; i < 3; ++i) {
        QTableWidget *table = createTableWidget();
        tableWidgets.append(table);
        stackedWidget->addWidget(table);
    }
    stackedWidget->setCurrentIndex(0);
    
    // 将堆叠窗口添加到主布局
    mainLayout->addWidget(stackedWidget, 1);
    
    // 创建底部面板，包含标签页和缩放控件
    QWidget *bottomPanel = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(5, 2, 5, 2);
    bottomLayout->setSpacing(10);
    
    // 左下角：Sheet标签页
    QWidget *sheetPanel = new QWidget(this);
    QHBoxLayout *sheetLayout = new QHBoxLayout(sheetPanel);
    sheetLayout->setContentsMargins(0, 0, 0, 0);
    sheetLayout->setSpacing(2);
    
    for (int i = 0; i < 3; ++i) {
        QPushButton *sheetBtn = new QPushButton(QString("Sheet%1").arg(i + 1), this);
        sheetBtn->setCheckable(true);
        sheetBtn->setChecked(i == 0);
        sheetBtn->setFixedSize(60, 25);
        sheetBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #e0e0e0;"
            "   border: 1px solid #999;"
            "   border-radius: 3px;"
            "   font-size: 11px;"
            "}"
            "QPushButton:checked {"
            "   background-color: #f0f0f0;"
            "   border: 2px solid #0078d4;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: #d0d0d0;"
            "}"
        );
        connect(sheetBtn, &QPushButton::clicked, [this, i, stackedWidget]() {
            stackedWidget->setCurrentIndex(i);
            updateSheetButtons();
        });
        sheetButtons.append(sheetBtn);
        sheetLayout->addWidget(sheetBtn);
    }
    
    // 添加加号按钮
    QPushButton *addSheetBtn = new QPushButton("+", this);
    addSheetBtn->setFixedSize(30, 25);
    addSheetBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #e0e0e0;"
        "   border: 1px solid #999;"
        "   border-radius: 3px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #d0d0d0;"
        "}"
    );
    connect(addSheetBtn, &QPushButton::clicked, [this, sheetLayout, addSheetBtn, stackedWidget]() {
        int newIndex = tableWidgets.size();
        QTableWidget *table = createTableWidget();
        tableWidgets.append(table);
        stackedWidget->addWidget(table);
        
        // 创建新的sheet按钮
        QPushButton *sheetBtn = new QPushButton(QString("Sheet%1").arg(newIndex + 1), this);
        sheetBtn->setCheckable(true);
        sheetBtn->setFixedSize(60, 25);
        sheetBtn->setStyleSheet(
            "QPushButton {"
            "   background-color: #e0e0e0;"
            "   border: 1px solid #999;"
            "   border-radius: 3px;"
            "   font-size: 11px;"
            "}"
            "QPushButton:checked {"
            "   background-color: #f0f0f0;"
            "   border: 2px solid #0078d4;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "   background-color: #d0d0d0;"
            "}"
        );
        connect(sheetBtn, &QPushButton::clicked, [this, newIndex, stackedWidget]() {
            stackedWidget->setCurrentIndex(newIndex);
            updateSheetButtons();
        });
        sheetButtons.append(sheetBtn);
        
        // 移除加号按钮，添加新的sheet按钮，然后重新添加加号按钮
        sheetLayout->removeWidget(addSheetBtn);
        sheetLayout->addWidget(sheetBtn);
        sheetLayout->addWidget(addSheetBtn);
        
        // 切换到新的sheet
        stackedWidget->setCurrentIndex(newIndex);
        updateSheetButtons();
    });
    sheetLayout->addWidget(addSheetBtn);
    
    sheetLayout->addStretch();
    bottomLayout->addWidget(sheetPanel, 0, Qt::AlignLeft);
    
    // 右下角：缩放控件
    QWidget *zoomPanel = new QWidget(this);
    QHBoxLayout *zoomLayout = new QHBoxLayout(zoomPanel);
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->setSpacing(5);
    
    QLabel *zoomLabel = new QLabel("Zoom:", this);
    zoomSlider = new QSlider(Qt::Horizontal, this);
    zoomSlider->setRange(50, 150);
    zoomSlider->setValue(100);
    zoomSlider->setFixedWidth(120);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    zoomSlider->setTickInterval(25);
    
    zoomValueLabel = new QLabel("100%", this);
    zoomValueLabel->setFixedWidth(40);
    zoomValueLabel->setAlignment(Qt::AlignCenter);
    
    zoomLayout->addWidget(zoomLabel);
    zoomLayout->addWidget(zoomSlider);
    zoomLayout->addWidget(zoomValueLabel);
    
    connect(zoomSlider, &QSlider::valueChanged, this, &Spreadsheet::onZoomChanged);
    
    bottomLayout->addStretch();
    bottomLayout->addWidget(zoomPanel, 0, Qt::AlignRight);
    
    // 设置底部面板样式
    bottomPanel->setStyleSheet(
        "QWidget {"
        "   background-color: #f0f0f0;"
        "   border-top: 1px solid #ccc;"
        "}"
        "QSlider::groove:horizontal {"
        "   border: 1px solid #999;"
        "   height: 6px;"
        "   background: #e0e0e0;"
        "   margin: 2px 0;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: #0078d4;"
        "   border: 1px solid #0078d4;"
        "   width: 14px;"
        "   height: 14px;"
        "   border-radius: 7px;"
        "   margin: -4px 0;"
        "}"
        "QSlider::sub-page:horizontal {"
        "   background: #0078d4;"
        "   border: 1px solid #0078d4;"
        "   height: 6px;"
        "}"
    );
    
    mainLayout->addWidget(bottomPanel);
    
    // 添加格式设置工具栏
    QToolBar *formatToolBar = addToolBar("Format");
    
    // 字体选择下拉框
    QComboBox *fontComboBox = new QComboBox();
    fontComboBox->addItems({"Arial", "Times New Roman", "Courier New", "Calibri", "Verdana"});
    formatToolBar->addWidget(fontComboBox);
    connect(fontComboBox, &QComboBox::currentTextChanged, this, &Spreadsheet::onFontChanged);
    
    // 字体大小下拉框
    fontSizeComboBox = new QComboBox();
    QList<int> fontSizes = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
    for (int size : fontSizes) {
        fontSizeComboBox->addItem(QString::number(size));
    }
    fontSizeComboBox->setCurrentText("12");
    formatToolBar->addWidget(fontSizeComboBox);
    connect(fontSizeComboBox, &QComboBox::currentTextChanged, this, &Spreadsheet::onFontSizeChanged);
    
    // 字体样式按钮
    QAction *boldAction = formatToolBar->addAction("B");
    QAction *italicAction = formatToolBar->addAction("I");
    QAction *underlineAction = formatToolBar->addAction("U");
    connect(boldAction, &QAction::triggered, this, &Spreadsheet::onBoldClicked);
    connect(italicAction, &QAction::triggered, this, &Spreadsheet::onItalicClicked);
    connect(underlineAction, &QAction::triggered, this, &Spreadsheet::onUnderlineClicked);
    
    // 对齐方式按钮
    QAction *leftAlignAction = formatToolBar->addAction("Left");
    QAction *centerAlignAction = formatToolBar->addAction("Center");
    QAction *rightAlignAction = formatToolBar->addAction("Right");
    connect(leftAlignAction, &QAction::triggered, this, [this]() { onAlignClicked(0); });
    connect(centerAlignAction, &QAction::triggered, this, [this]() { onAlignClicked(1); });
    connect(rightAlignAction, &QAction::triggered, this, [this]() { onAlignClicked(2); });
    
    // 字体颜色按钮
    QAction *fontColorAction = formatToolBar->addAction("Font Color");
    connect(fontColorAction, &QAction::triggered, this, &Spreadsheet::onFontColorClicked);
    
    // 格子颜色按钮
    QAction *cellColorAction = formatToolBar->addAction("Cell Color");
    connect(cellColorAction, &QAction::triggered, this, &Spreadsheet::onCellColorClicked);
    
    // 格式刷按钮
    formatBrushAction = formatToolBar->addAction("Format Brush");
    connect(formatBrushAction, &QAction::triggered, this, &Spreadsheet::onFormatBrushClicked);
    
    // 清除按钮
    QAction *clearAction = formatToolBar->addAction("Clear");
    connect(clearAction, &QAction::triggered, this, &Spreadsheet::onClearClicked);
    
    // 合并单元格按钮
    QAction *mergeAction = formatToolBar->addAction("Merge Cells");
    connect(mergeAction, &QAction::triggered, this, &Spreadsheet::onMergeCells);
    
    // 插入图表按钮
    QAction *chartAction = formatToolBar->addAction("Insert Chart");
    connect(chartAction, &QAction::triggered, this, &Spreadsheet::onInsertChart);
    
    setCentralWidget(mainWidget);
    setWindowTitle("Spreadsheet");
    setGeometry(100, 100, 1200, 800);

    // 插入行和列的下拉菜单
    QMenu *insertMenu = new QMenu("Insert");
    QAction *insertRowAboveAction = insertMenu->addAction("Insert Row Above");
    QAction *insertRowBelowAction = insertMenu->addAction("Insert Row Below");
    QAction *insertColumnLeftAction = insertMenu->addAction("Insert Column Left");
    QAction *insertColumnRightAction = insertMenu->addAction("Insert Column Right");
    
    connect(insertRowAboveAction, &QAction::triggered, this, &Spreadsheet::onInsertRowAbove);
    connect(insertRowBelowAction, &QAction::triggered, this, &Spreadsheet::onInsertRowBelow);
    connect(insertColumnLeftAction, &QAction::triggered, this, &Spreadsheet::onInsertColumnLeft);
    connect(insertColumnRightAction, &QAction::triggered, this, &Spreadsheet::onInsertColumnRight);
    
    QAction *insertAction = new QAction("Insert");
    insertAction->setMenu(insertMenu);
    formatToolBar->addAction(insertAction);
}

QTableWidget* Spreadsheet::getCurrentTable()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget*>(centralWidget()->layout()->itemAt(1)->widget());
    if (stackedWidget) {
        int currentIndex = stackedWidget->currentIndex();
        if (currentIndex >= 0 && currentIndex < tableWidgets.size()) {
            return tableWidgets[currentIndex];
        }
    }
    return nullptr;
}

void Spreadsheet::updateSheetButtons()
{
    // 从stackedWidget获取当前索引
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget*>(centralWidget()->layout()->itemAt(1)->widget());
    if (stackedWidget) {
        int currentIndex = stackedWidget->currentIndex();
        for (int i = 0; i < sheetButtons.size(); ++i) {
            sheetButtons[i]->setChecked(i == currentIndex);
        }
    }
}

void Spreadsheet::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu("File");
    
    newAction = fileMenu->addAction("New");
    openAction = fileMenu->addAction("Open");
    saveAction = fileMenu->addAction("Save");
    saveAsAction = fileMenu->addAction("Save As");
    
    connect(newAction, &QAction::triggered, this, &Spreadsheet::newFile);
    connect(openAction, &QAction::triggered, this, &Spreadsheet::openFile);
    connect(saveAction, &QAction::triggered, this, &Spreadsheet::saveFile);
    connect(saveAsAction, &QAction::triggered, this, &Spreadsheet::saveAsFile);
    
    // 添加数据菜单
    QMenu *dataMenu = menuBar()->addMenu("Data");
    
    QAction *sortAscAction = dataMenu->addAction("Sort Ascending");
    QAction *sortDescAction = dataMenu->addAction("Sort Descending");
    QAction *filterAction = dataMenu->addAction("Filter");
    
    connect(sortAscAction, &QAction::triggered, this, &Spreadsheet::onSortAscending);
    connect(sortDescAction, &QAction::triggered, this, &Spreadsheet::onSortDescending);
    connect(filterAction, &QAction::triggered, this, &Spreadsheet::onFilter);
    
    // 添加编辑菜单
    QMenu *editMenu = menuBar()->addMenu("Edit");
    
    QAction *findAction = editMenu->addAction("Find");
    QAction *replaceAction = editMenu->addAction("Replace");
    
    connect(findAction, &QAction::triggered, this, &Spreadsheet::onFind);
    connect(replaceAction, &QAction::triggered, this, &Spreadsheet::onReplace);
}

void Spreadsheet::onCellChanged(int row, int column)
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    // 检查是否需要扩展行数
    int currentRows = currentTable->rowCount();
    if (row >= currentRows - 5) { // 当编辑到倒数第5行时，添加5行
        int newRows = currentRows + 5;
        currentTable->setRowCount(newRows);
        
        // 设置新添加行的行号
        for (int i = currentRows; i < newRows; ++i) {
            currentTable->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i + 1)));
        }
    }
    
    // 检查是否需要扩展列数
    int currentColumns = currentTable->columnCount();
    if (column >= currentColumns - 5) { // 当编辑到倒数第5列时，添加5列
        int newColumns = currentColumns + 5;
        currentTable->setColumnCount(newColumns);
        
        // 设置新添加列的列名
        for (int i = currentColumns; i < newColumns; ++i) {
            currentTable->setHorizontalHeaderItem(i, new QTableWidgetItem(columnName(i)));
        }
    }
    
    QTableWidgetItem *item = currentTable->item(row, column);
    if (item) {
        QFont font = item->font();
        if (font.pointSize() == 0) {
            font.setPointSize(12);
            item->setFont(font);
        }
        
        QString text = item->text();
        if (text.startsWith('=')) {
            QString cellAddress = columnName(column) + QString::number(row + 1);
            if (hasCycle(cellAddress, text.mid(1))) {
                QMessageBox::warning(this, "Error", "输入非法：公式中存在循环引用");
                item->setText("Error");
                return;
            }
            
            FormulaParser parser(text.mid(1), this);
            double result = parser.evaluate();
            if (parser.hasError()) {
                item->setText("Error");
            } else {
                item->setText(QString::number(result));
            }
        }
    }
}

void Spreadsheet::onTabChanged(int index)
{
    updateSheetButtons();
}

void Spreadsheet::onCellSelected(int row, int column)
{
    QString cellAddress = columnName(column) + QString::number(row + 1);
    cellAddressComboBox->setCurrentText(cellAddress);
    
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QTableWidgetItem *item = currentTable->item(row, column);
    if (item) {
        formulaEdit->setText(item->text());
    } else {
        formulaEdit->clear();
    }
}

bool Spreadsheet::hasCycle(const QString &cellAddress, const QString &formula)
{
    calculatingCells.clear();
    return checkCycle(cellAddress, formula);
}

bool Spreadsheet::checkCycle(const QString &cellAddress, const QString &formula)
{
    if (calculatingCells.contains(cellAddress)) {
        return true;
    }
    
    calculatingCells.insert(cellAddress);
    
    QRegularExpression re("([A-Z]+[0-9]+)");
    QRegularExpressionMatchIterator it = re.globalMatch(formula);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString refCell = match.captured(1);
        
        if (refCell == cellAddress) {
            calculatingCells.remove(cellAddress);
            return true;
        }
        
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return false;
        
        QString colName;
        int rowNum = 0;
        int i = 0;
        while (i < refCell.length() && refCell[i].isLetter()) {
            colName.append(refCell[i]);
            i++;
        }
        if (i < refCell.length()) {
            rowNum = refCell.mid(i).toInt() - 1;
        }
        
        int col = columnIndex(colName);
        if (rowNum >= 0 && rowNum < currentTable->rowCount() && col >= 0 && col < currentTable->columnCount()) {
            QTableWidgetItem *item = currentTable->item(rowNum, col);
            if (item) {
                QString refFormula = item->text();
                if (refFormula.startsWith('=')) {
                    if (checkCycle(refCell, refFormula.mid(1))) {
                        calculatingCells.remove(cellAddress);
                        return true;
                    }
                }
            }
        }
    }
    
    calculatingCells.remove(cellAddress);
    return false;
}

void Spreadsheet::onCellAddressChanged(const QString &address)
{
    if (address.isEmpty()) return;
    
    // 解析单元格地址
    QString colName;
    int rowNum = 0;
    int i = 0;
    while (i < address.length() && address[i].isLetter()) {
        colName.append(address[i].toUpper());
        i++;
    }
    if (i < address.length()) {
        rowNum = address.mid(i).toInt() - 1;
    }
    
    int col = columnIndex(colName);
    if (col >= 0 && rowNum >= 0) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        if (rowNum < currentTable->rowCount() && col < currentTable->columnCount()) {
            currentTable->setCurrentCell(rowNum, col);
            
            QTableWidgetItem *item = currentTable->item(rowNum, col);
            if (item) {
                formulaEdit->setText(item->text());
            } else {
                formulaEdit->clear();
            }
        }
    }
}

void Spreadsheet::onFormulaChanged(const QString &text)
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    int row = currentTable->currentRow();
    int column = currentTable->currentColumn();
    
    if (row >= 0 && column >= 0) {
        QTableWidgetItem *item = currentTable->item(row, column);
        if (!item) {
            item = new QTableWidgetItem();
            currentTable->setItem(row, column, item);
        }
        
        if (text.startsWith('=')) {
            // 避免空公式导致的问题
            if (text.length() == 1) {
                // 只输入了=，不需要检测循环
                item->setText(text);
                return;
            }
            
            QString cellAddress = columnName(column) + QString::number(row + 1);
            if (hasCycle(cellAddress, text.mid(1))) {
                QMessageBox::warning(this, "错误", "输入非法：公式中存在循环引用");
                
                // 恢复原始值
                if (item->text().startsWith('=')) {
                    formulaEdit->setText(item->text());
                } else {
                    formulaEdit->clear();
                }
                return;
            }
            
            // 计算公式
            FormulaParser parser(text.mid(1), this);
            double result = parser.evaluate();
            if (parser.hasError()) {
                item->setText("Error");
            } else {
                item->setText(QString::number(result));
            }
        } else {
            // 普通文本
            item->setText(text);
        }
    }
}

void Spreadsheet::setupFormulaBar()
{
    formulaBarWidget = new QWidget(this);
    QHBoxLayout *formulaBarLayout = new QHBoxLayout(formulaBarWidget);
    formulaBarLayout->setContentsMargins(5, 5, 5, 5);
    
    cellAddressComboBox = new QComboBox(formulaBarWidget);
    cellAddressComboBox->setMinimumWidth(100);
    cellAddressComboBox->setEditable(true);
    cellAddressComboBox->setPlaceholderText("A1");
    
    // 填充常用的单元格地址
    QStringList cellAddresses;
    for (char c = 'A'; c <= 'Z'; ++c) {
        for (int i = 1; i <= 10; ++i) {
            cellAddresses << QString("%1%2").arg(c).arg(i);
        }
    }
    cellAddressComboBox->addItems(cellAddresses);
    
    formulaLabel = new QLabel("fx", formulaBarWidget);
    formulaLabel->setMinimumWidth(30);
    formulaLabel->setAlignment(Qt::AlignCenter);
    
    formulaEdit = new QLineEdit(formulaBarWidget);
    formulaEdit->setPlaceholderText("Enter formula here");
    
    connect(formulaEdit, &QLineEdit::textChanged, this, &Spreadsheet::onFormulaChanged);
    connect(cellAddressComboBox, &QComboBox::currentTextChanged, this, &Spreadsheet::onCellAddressChanged);
    
    formulaBarLayout->addWidget(cellAddressComboBox);
    formulaBarLayout->addWidget(formulaLabel);
    formulaBarLayout->addWidget(formulaEdit);
    
    formulaBarWidget->setStyleSheet(
        "QWidget {"
        "   background: #f0f0f0;"
        "   border: 1px solid #ccc;"
        "}"
        "QComboBox, QLineEdit, QLabel {"
        "   border: 1px solid #ccc;"
        "   padding: 2px;"
        "   background: #e0e0e0;"
        "}"
    );
}

void Spreadsheet::onZoomChanged(int value)
{
    zoomValueLabel->setText(QString("%1%").arg(value));
    
    double scaleFactor = value / 100.0;
    
    for (QTableWidget *table : tableWidgets) {
        QFont font = table->font();
        font.setPointSizeF(9 * scaleFactor);
        table->setFont(font);
        
        for (int i = 0; i < table->columnCount(); ++i) {
            table->setColumnWidth(i, 80 * scaleFactor);
        }
        for (int i = 0; i < table->rowCount(); ++i) {
            table->setRowHeight(i, 20 * scaleFactor);
        }
    }
    
    currentZoom = value;
}

void Spreadsheet::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open File", "", "All Files (*.dat *.csv *.xlsx);;Dat Files (*.dat);;CSV Files (*.csv);;Excel Files (*.xlsx)");
    if (!filename.isEmpty()) {
        loadFromFile(filename);
        currentFile = filename;
    }
}

void Spreadsheet::saveFile()
{
    if (currentFile.isEmpty()) {
        saveAsFile();
    } else {
        saveToFile(currentFile);
    }
}

void Spreadsheet::saveAsFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save File", "", "Dat Files (*.dat);;CSV Files (*.csv);;Excel Files (*.xlsx)");
    if (!filename.isEmpty()) {
        saveToFile(filename);
        currentFile = filename;
    }
}

void Spreadsheet::saveAsExcel(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    
    QTextStream out(&file);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><worksheet name=\"Sheet1\"><sheetData>";
    
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    for (int row = 0; row < currentTable->rowCount(); ++row) {
        out << "<row r=\"" << (row + 1) << "\">";
        for (int col = 0; col < currentTable->columnCount(); ++col) {
            QTableWidgetItem *item = currentTable->item(row, col);
            if (item) {
                QString value = item->text();
                out << "<c r=\"" << columnName(col) << (row + 1) << "\"><v>" << value << "</v></c>";
            }
        }
        out << "</row>";
    }
    
    out << "</sheetData></worksheet></workbook>";
    file.close();
}

void Spreadsheet::newFile()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    currentTable->clearContents();
    currentFile.clear();
}

void Spreadsheet::loadFromFile(const QString &filename)
{
    QMap<QString, Cell> cells;
    if (FileFormat::load(filename, cells)) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        currentTable->clearContents();
        for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
            QString cellName = it.key();
            QString colName;
            int rowNum = 0;
            int i = 0;
            while (i < cellName.length() && cellName[i].isLetter()) {
                colName.append(cellName[i]);
                i++;
            }
            if (i < cellName.length()) {
                rowNum = cellName.mid(i).toInt() - 1;
            }
            int colNum = columnIndex(colName);
            
            if (rowNum >= 0 && rowNum < currentTable->rowCount() && colNum >= 0 && colNum < currentTable->columnCount()) {
                QTableWidgetItem *item = new QTableWidgetItem(it.value().getValue());
                currentTable->setItem(rowNum, colNum, item);
            }
        }
    }
}

void Spreadsheet::saveToFile(const QString &filename)
{
    QMap<QString, Cell> cells;
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    for (int row = 0; row < currentTable->rowCount(); ++row) {
        for (int col = 0; col < currentTable->columnCount(); ++col) {
            QTableWidgetItem *item = currentTable->item(row, col);
            if (item) {
                QString cellName = columnName(col) + QString::number(row + 1);
                Cell cell(item->text(), false, this);
                cells[cellName] = cell;
            }
        }
    }
    FileFormat::save(filename, cells);
}

QString Spreadsheet::columnName(int column) const
{
    QString name;
    while (column >= 0) {
        name.prepend(QChar('A' + (column % 26)));
        column = (column / 26) - 1;
    }
    return name;
}

int Spreadsheet::columnIndex(const QString &name) const
{
    int index = 0;
    for (QChar ch : name) {
        index = index * 26 + (ch.toUpper().unicode() - 'A' + 1);
    }
    return index - 1;
}

double Spreadsheet::getCellNumericValue(int row, const QString &colName) {
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return 0;
    int col = columnIndex(colName);
    if (row < 0 || row >= currentTable->rowCount() || col < 0 || col >= currentTable->columnCount()) {
        return 0;
    }
    
    QTableWidgetItem *item = currentTable->item(row, col);
    if (!item) {
        return 0;
    }
    
    QString text = item->text();
    bool ok;
    double value = text.toDouble(&ok);
    
    if (text.startsWith('=')) {
        QString cellAddress = colName + QString::number(row + 1);
        if (calculatingCells.contains(cellAddress)) {
            return 0;
        }

        calculatingCells.insert(cellAddress);

        FormulaParser parser(text.mid(1), this);
        double result = parser.evaluate();
        if (!parser.hasError()) {
            value = result;
            ok = true;
        }

        calculatingCells.remove(cellAddress);
    }
    
    return ok ? value : 0;
}

std::vector<double> Spreadsheet::getRangeNumericValues(int startRow, const QString &startCol, int endRow, const QString &endCol) {
    std::vector<double> values;
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return values;
    
    int startColIndex = columnIndex(startCol);
    int endColIndex = columnIndex(endCol);
    
    if (startRow > endRow) std::swap(startRow, endRow);
    if (startColIndex > endColIndex) std::swap(startColIndex, endColIndex);
    
    for (int row = startRow; row <= endRow; ++row) {
        for (int col = startColIndex; col <= endColIndex; ++col) {
            if (row >= 0 && row < currentTable->rowCount() && col >= 0 && col < currentTable->columnCount()) {
                QTableWidgetItem *item = currentTable->item(row, col);
                if (item) {
                    QString text = item->text();
                    bool ok;
                    double value = text.toDouble(&ok);
                    
                    if (text.startsWith('=')) {
                        QString cellAddress = columnName(col) + QString::number(row + 1);
                        if (calculatingCells.contains(cellAddress)) {
                            continue;
                        }

                        calculatingCells.insert(cellAddress);

                        FormulaParser parser(text.mid(1), this);
                        double result = parser.evaluate();
                        if (!parser.hasError()) {
                            value = result;
                            ok = true;
                        }

                        calculatingCells.remove(cellAddress);
                    }
                    
                    if (ok) {
                        values.push_back(value);
                    }
                }
            }
        }
    }
    
    return values;
}

void Spreadsheet::onBoldClicked()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        QFont font = item->font();
        font.setBold(!font.bold());
        item->setFont(font);
    }
}

void Spreadsheet::onFontSizeChanged(const QString &size)
{
    bool ok;
    int fontSize = size.toInt(&ok);
    if (ok) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
        for (QTableWidgetItem *item : selectedItems) {
            if (item) {
                QFont font = item->font();
                font.setPointSize(fontSize);
                item->setFont(font);
            }
        }
    }
}

void Spreadsheet::onAlignClicked(int alignment)
{
    Qt::AlignmentFlag align;
    switch (alignment) {
    case 0:
        align = Qt::AlignLeft;
        break;
    case 1:
        align = Qt::AlignCenter;
        break;
    case 2:
        align = Qt::AlignRight;
        break;
    default:
        align = Qt::AlignLeft;
        break;
    }
    
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        item->setTextAlignment(align);
    }
}

void Spreadsheet::onFontChanged(const QString &font)
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        QFont fontObj = item->font();
        fontObj.setFamily(font);
        item->setFont(fontObj);
    }
}

void Spreadsheet::onItalicClicked()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        QFont font = item->font();
        font.setItalic(!font.italic());
        item->setFont(font);
    }
}

void Spreadsheet::onUnderlineClicked()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        QFont font = item->font();
        font.setUnderline(!font.underline());
        item->setFont(font);
    }
}

void Spreadsheet::onFontColorClicked()
{
    QColor color = QColorDialog::getColor(Qt::black, this, "Select Font Color");
    if (color.isValid()) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
        for (QTableWidgetItem *item : selectedItems) {
            item->setForeground(QBrush(color));
        }
    }
}

void Spreadsheet::onCellColorClicked()
{
    QColor color = QColorDialog::getColor(Qt::white, this, "Select Cell Color");
    if (color.isValid()) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
        for (QTableWidgetItem *item : selectedItems) {
            item->setBackground(QBrush(color));
        }
    }
}

void Spreadsheet::onFormatBrushClicked()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    if (!formatBrushActive) {
        if (currentTable->selectedItems().isEmpty()) {
            QMessageBox::information(this, "Format Brush", "Please select a cell to copy format from");
            return;
        }
        
        QTableWidgetItem *item = currentTable->selectedItems().first();
        formatBrush = item->font();
        formatBrushActive = true;
        formatBrushAction->setEnabled(false);
        
        connect(currentTable, &QTableWidget::cellClicked, this, &Spreadsheet::onCellClickedForFormatBrush);
    }
}

void Spreadsheet::onCellClickedForFormatBrush(int row, int column)
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QTableWidgetItem *item = currentTable->item(row, column);
    if (item) {
        item->setFont(formatBrush);
    }
    
    formatBrushActive = false;
    formatBrushAction->setEnabled(true);
    disconnect(currentTable, &QTableWidget::cellClicked, this, &Spreadsheet::onCellClickedForFormatBrush);
}

void Spreadsheet::onClearClicked()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    QList<QTableWidgetItem *> selectedItems = currentTable->selectedItems();
    for (QTableWidgetItem *item : selectedItems) {
        item->setText("");
        QFont font;
        item->setFont(font);
        item->setTextAlignment(Qt::AlignLeft);
        item->setForeground(QBrush(Qt::black));
        item->setBackground(QBrush(Qt::white));
    }
}

void Spreadsheet::onSortAscending()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    if (currentTable->selectedItems().isEmpty()) {
        QMessageBox::information(this, "Sort", "Please select a column to sort");
        return;
    }
    
    int currentColumn = currentTable->currentColumn();
    currentTable->sortItems(currentColumn, Qt::AscendingOrder);
}

void Spreadsheet::onSortDescending()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    if (currentTable->selectedItems().isEmpty()) {
        QMessageBox::information(this, "Sort", "Please select a column to sort");
        return;
    }
    
    int currentColumn = currentTable->currentColumn();
    currentTable->sortItems(currentColumn, Qt::DescendingOrder);
}

void Spreadsheet::onFilter()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    if (currentTable->selectedItems().isEmpty()) {
        QMessageBox::information(this, "Filter", "Please select a column to filter");
        return;
    }
    
    int currentColumn = currentTable->currentColumn();
    
    QSet<QString> uniqueValues;
    for (int row = 0; row < currentTable->rowCount(); ++row) {
        QTableWidgetItem *item = currentTable->item(row, currentColumn);
        if (item) {
            uniqueValues.insert(item->text());
        }
    }
    
    QStringList values = uniqueValues.values();
    values.sort();
    
    QDialog dialog(this);
    dialog.setWindowTitle("Filter");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *filterLabel = new QLabel("Select value to filter:");
    QComboBox *valueComboBox = new QComboBox();
    valueComboBox->addItems(values);
    
    QPushButton *filterButton = new QPushButton("Filter");
    QPushButton *clearButton = new QPushButton("Clear Filter");
    QPushButton *cancelButton = new QPushButton("Cancel");
    
    layout->addWidget(filterLabel);
    layout->addWidget(valueComboBox);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(filterButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(filterButton, &QPushButton::clicked, [&, currentTable, currentColumn]() {
        QString selectedValue = valueComboBox->currentText();
        if (!selectedValue.isEmpty()) {
            for (int row = 0; row < currentTable->rowCount(); ++row) {
                QTableWidgetItem *item = currentTable->item(row, currentColumn);
                if (item && item->text() != selectedValue) {
                    currentTable->hideRow(row);
                } else {
                    currentTable->showRow(row);
                }
            }
        }
        dialog.accept();
    });
    
    connect(clearButton, &QPushButton::clicked, [&, currentTable]() {
        for (int row = 0; row < currentTable->rowCount(); ++row) {
            currentTable->showRow(row);
        }
        dialog.accept();
    });
    
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    dialog.exec();
}

void Spreadsheet::onFind()
{
    bool ok;
    QString searchText = QInputDialog::getText(this, "Find", "Enter text to find:", QLineEdit::Normal, "", &ok);
    
    if (ok && !searchText.isEmpty()) {
        QTableWidget *currentTable = getCurrentTable();
        if (!currentTable) return;
        bool found = false;
        for (int row = 0; row < currentTable->rowCount() && !found; ++row) {
            for (int col = 0; col < currentTable->columnCount() && !found; ++col) {
                QTableWidgetItem *item = currentTable->item(row, col);
                if (item && item->text().contains(searchText, Qt::CaseInsensitive)) {
                    currentTable->setCurrentItem(item);
                    currentTable->scrollToItem(item);
                    found = true;
                }
            }
        }
        
        if (!found) {
            QMessageBox::information(this, "Find", "Text not found");
        }
    }
}

void Spreadsheet::onReplace()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Replace");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *findLabel = new QLabel("Find what:");
    QLineEdit *findEdit = new QLineEdit();
    
    QLabel *replaceLabel = new QLabel("Replace with:");
    QLineEdit *replaceEdit = new QLineEdit();
    
    QPushButton *replaceButton = new QPushButton("Replace");
    QPushButton *cancelButton = new QPushButton("Cancel");
    
    layout->addWidget(findLabel);
    layout->addWidget(findEdit);
    layout->addWidget(replaceLabel);
    layout->addWidget(replaceEdit);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(replaceButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    connect(replaceButton, &QPushButton::clicked, [&]() {
        QString findText = findEdit->text();
        QString replaceText = replaceEdit->text();
        
        if (!findText.isEmpty()) {
            int count = 0;
            QTableWidget *currentTable = getCurrentTable();
            if (!currentTable) return;
            for (int row = 0; row < currentTable->rowCount(); ++row) {
                for (int col = 0; col < currentTable->columnCount(); ++col) {
                    QTableWidgetItem *item = currentTable->item(row, col);
                    if (item && item->text().contains(findText, Qt::CaseInsensitive)) {
                        item->setText(item->text().replace(findText, replaceText, Qt::CaseInsensitive));
                        count++;
                    }
                }
            }
            QMessageBox::information(&dialog, "Replace", QString("Replaced %1 occurrences").arg(count));
        }
        dialog.accept();
    });
    
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    dialog.exec();
}

void Spreadsheet::onMergeCells()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    // 使用selectedRanges()获取选中区域，这样可以包含空单元格
    QList<QTableWidgetSelectionRange> selectedRanges = currentTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
        QMessageBox::information(this, "Merge Cells", "Please select cells to merge");
        return;
    }
    
    // 只处理第一个选中区域
    QTableWidgetSelectionRange range = selectedRanges[0];
    int minRow = range.topRow();
    int maxRow = range.bottomRow();
    int minCol = range.leftColumn();
    int maxCol = range.rightColumn();
    
    // 获取左上角单元格的内容
    QTableWidgetItem *topLeftItem = currentTable->item(minRow, minCol);
    QString content = topLeftItem ? topLeftItem->text() : "";
    
    // 清除所有选中单元格的内容，除了左上角
    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            if (row != minRow || col != minCol) {
                delete currentTable->takeItem(row, col);
            }
        }
    }
    
    // 合并单元格
    currentTable->setSpan(minRow, minCol, maxRow - minRow + 1, maxCol - minCol + 1);
    
    // 设置合并后单元格的内容
    if (!content.isEmpty()) {
        if (!topLeftItem) {
            topLeftItem = new QTableWidgetItem(content);
            currentTable->setItem(minRow, minCol, topLeftItem);
        } else {
            topLeftItem->setText(content);
        }
    }
}

void Spreadsheet::onInsertChart()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    // 获取选中区域
    QList<QTableWidgetSelectionRange> selectedRanges = currentTable->selectedRanges();
    if (selectedRanges.isEmpty()) {
        QMessageBox::information(this, "Insert Chart", "Please select data range for chart");
        return;
    }
    
    // 只处理第一个选中区域
    QTableWidgetSelectionRange range = selectedRanges[0];
    int minRow = range.topRow();
    int maxRow = range.bottomRow();
    int minCol = range.leftColumn();
    int maxCol = range.rightColumn();
    
    // 提取数据
    QStringList categories;
    QList<double> values;
    
    // 检查是横向选择还是纵向选择
    int rowCount = maxRow - minRow + 1;
    int colCount = maxCol - minCol + 1;
    
    if (colCount > 1 && rowCount == 1) {
        // 横向选择：多列一行，每个单元格都是一个数据点
        for (int col = minCol; col <= maxCol; ++col) {
            QTableWidgetItem *valueItem = currentTable->item(minRow, col);
            if (valueItem) {
                // 使用单元格的实际值作为分类名称
                categories.append(valueItem->text());
                bool ok;
                double value = valueItem->text().toDouble(&ok);
                if (ok) {
                    values.append(value);
                } else {
                    values.append(0);
                }
            } else {
                categories.append(QString("Column %1").arg(columnName(col)));
                values.append(0);
            }
        }
    } else if (rowCount > 1 && colCount == 1) {
        // 纵向选择：多行一列，每个单元格都是一个数据点
        for (int row = minRow; row <= maxRow; ++row) {
            QTableWidgetItem *valueItem = currentTable->item(row, minCol);
            if (valueItem) {
                // 使用单元格的实际值作为分类名称
                categories.append(valueItem->text());
                bool ok;
                double value = valueItem->text().toDouble(&ok);
                if (ok) {
                    values.append(value);
                } else {
                    values.append(0);
                }
            } else {
                categories.append(QString("Row %1").arg(row + 1));
                values.append(0);
            }
        }
    } else if (colCount > 1 && rowCount > 1) {
        // 矩形选择：第一列是分类，其他列是数值
        for (int row = minRow; row <= maxRow; ++row) {
            // 获取分类名称
            QTableWidgetItem *categoryItem = currentTable->item(row, minCol);
            if (categoryItem) {
                categories.append(categoryItem->text());
            } else {
                categories.append(QString("Item %1").arg(row - minRow + 1));
            }
            
            // 获取数值（使用第二列）
            if (minCol + 1 <= maxCol) {
                QTableWidgetItem *valueItem = currentTable->item(row, minCol + 1);
                if (valueItem) {
                    bool ok;
                    double value = valueItem->text().toDouble(&ok);
                    if (ok) {
                        values.append(value);
                    } else {
                        values.append(0);
                    }
                } else {
                    values.append(0);
                }
            } else {
                values.append(0);
            }
        }
    } else {
        // 单个单元格
        categories.append("Value");
        QTableWidgetItem *valueItem = currentTable->item(minRow, minCol);
        if (valueItem) {
            bool ok;
            double value = valueItem->text().toDouble(&ok);
            if (ok) {
                values.append(value);
            } else {
                values.append(0);
            }
        } else {
            values.append(0);
        }
    }
    
    // 创建图表类型选择对话框
    QDialog dialog(this);
    dialog.setWindowTitle("Insert Chart");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *chartTypeLabel = new QLabel("Select chart type:");
    QComboBox *chartTypeComboBox = new QComboBox();
    chartTypeComboBox->addItems({"Bar Chart", "Line Chart", "Pie Chart"});
    
    QPushButton *createButton = new QPushButton("Create Chart");
    QPushButton *cancelButton = new QPushButton("Cancel");
    
    layout->addWidget(chartTypeLabel);
    layout->addWidget(chartTypeComboBox);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
    
    QChart *chart = nullptr;
    
    connect(createButton, &QPushButton::clicked, [&]() {
        QString chartType = chartTypeComboBox->currentText();
        
        if (chartType == "Bar Chart") {
            // 创建柱状图
            QBarSeries *series = new QBarSeries();
            QBarSet *set = new QBarSet("Values");
            
            for (double value : values) {
                *set << value;
            }
            
            series->append(set);
            
            chart = new QChart();
            chart->addSeries(series);
            chart->setTitle("Bar Chart");
            
            QCategoryAxis *axisX = new QCategoryAxis();
            for (const QString &category : categories) {
                axisX->append(category, categories.indexOf(category));
            }
            axisX->setRange(0, categories.size() - 1);
            
            QValueAxis *axisY = new QValueAxis();
            axisY->setRange(0, *std::max_element(values.begin(), values.end()) * 1.1);
            
            chart->addAxis(axisX, Qt::AlignBottom);
            chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisX);
            series->attachAxis(axisY);
        } else if (chartType == "Line Chart") {
            // 创建折线图
            QLineSeries *series = new QLineSeries();
            
            for (int i = 0; i < values.size(); ++i) {
                series->append(i, values[i]);
            }
            
            chart = new QChart();
            chart->addSeries(series);
            chart->setTitle("Line Chart");
            
            QCategoryAxis *axisX = new QCategoryAxis();
            for (const QString &category : categories) {
                axisX->append(category, categories.indexOf(category));
            }
            axisX->setRange(0, categories.size() - 1);
            
            QValueAxis *axisY = new QValueAxis();
            axisY->setRange(0, *std::max_element(values.begin(), values.end()) * 1.1);
            
            chart->addAxis(axisX, Qt::AlignBottom);
            chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis(axisX);
            series->attachAxis(axisY);
        } else if (chartType == "Pie Chart") {
            // 创建饼图
            QPieSeries *series = new QPieSeries();
            
            for (int i = 0; i < values.size(); ++i) {
                series->append(categories[i], values[i]);
            }
            
            chart = new QChart();
            chart->addSeries(series);
            chart->setTitle("Pie Chart");
        }
        
        dialog.accept();
    });
    
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted && chart) {
        // 创建图表视图
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        
        // 创建图表窗口
        QDialog *chartDialog = new QDialog(this);
        chartDialog->setWindowTitle("Chart");
        chartDialog->setGeometry(100, 100, 800, 600);
        
        QVBoxLayout *chartLayout = new QVBoxLayout(chartDialog);
        chartLayout->addWidget(chartView);
        
        chartDialog->show();
    }
}

void Spreadsheet::onInsertRowAbove()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    int currentRow = currentTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "Insert Row", "Please select a cell first");
        return;
    }
    
    currentTable->insertRow(currentRow);
}

void Spreadsheet::onInsertRowBelow()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    int currentRow = currentTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "Insert Row", "Please select a cell first");
        return;
    }
    
    currentTable->insertRow(currentRow + 1);
}

void Spreadsheet::onInsertColumnLeft()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    int currentColumn = currentTable->currentColumn();
    if (currentColumn < 0) {
        QMessageBox::information(this, "Insert Column", "Please select a cell first");
        return;
    }
    
    currentTable->insertColumn(currentColumn);
    
    // 只更新新插入的列的标题，避免性能问题
    currentTable->setHorizontalHeaderItem(currentColumn, new QTableWidgetItem(columnName(currentColumn)));
}

void Spreadsheet::onInsertColumnRight()
{
    QTableWidget *currentTable = getCurrentTable();
    if (!currentTable) return;
    
    int currentColumn = currentTable->currentColumn();
    if (currentColumn < 0) {
        QMessageBox::information(this, "Insert Column", "Please select a cell first");
        return;
    }
    
    int newColumn = currentColumn + 1;
    currentTable->insertColumn(newColumn);
    
    // 只更新新插入的列的标题，避免性能问题
    currentTable->setHorizontalHeaderItem(newColumn, new QTableWidgetItem(columnName(newColumn)));
}