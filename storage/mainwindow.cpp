#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QByteArray>
#include <QDataStream>
#include <QSaveFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("CSV Compressor");
    
    // 初始化表格
    tableWidget = ui->tableWidget;
    tableWidget->setColumnCount(10);
    tableWidget->setRowCount(10);
    tableWidget->setHorizontalHeaderLabels({"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"});
    
    // 初始化还原表格
    restoredTableWidget = ui->restoredTableWidget;
    restoredTableWidget->setColumnCount(10);
    restoredTableWidget->setRowCount(10);
    restoredTableWidget->setHorizontalHeaderLabels({"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"});
    
    // 初始化信息文本框
    infoTextEdit = ui->infoTextEdit;
    
    // 连接信号和槽
    connect(ui->importButton, &QPushButton::clicked, this, &MainWindow::onImportButtonClicked);
    connect(ui->compressButton, &QPushButton::clicked, this, &MainWindow::onCompressButtonClicked);
    
    // 更新信息文本
    updateInfoText();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onImportButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open CSV File", "", "CSV Files (*.csv)");
    if (!filePath.isEmpty()) {
        currentFilePath = filePath;
        if (loadCSVFile(filePath)) {
            QFileInfo fileInfo(filePath);
            qint64 fileSize = fileInfo.size();
            infoTextEdit->append(QString("导入文件: %1").arg(filePath));
            infoTextEdit->append(QString("文件大小: %1 bytes").arg(fileSize));
        }
    }
}

void MainWindow::onCompressButtonClicked()
{
    if (currentFilePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先导入CSV文件");
        return;
    }
    
    // 获取原始文件信息
    QFileInfo fileInfo(currentFilePath);
    QString fileName = fileInfo.baseName();
    qint64 originalSize = fileInfo.size();
    
    if (compressData()) {
        // 自动保存到build目录
        QString buildPath = "/home/geqingyu/Code/class_design/build";
        QString compressedPath = buildPath + "/" + fileName + ".compressed";
        
        QSaveFile file(compressedPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(compressedData);
            if (file.commit()) {
                // 计算压缩效率
                qint64 compressedSize = compressedData.size();
                double compressionRatio = (double)compressedSize / originalSize * 100;
                
                infoTextEdit->append(QString("压缩成功!"));
                infoTextEdit->append(QString("原始文件大小: %1 bytes").arg(originalSize));
                infoTextEdit->append(QString("压缩后大小: %1 bytes").arg(compressedSize));
                infoTextEdit->append(QString("压缩效率: %1%").arg(compressionRatio, 0, 'f', 2));
                infoTextEdit->append(QString("压缩文件已保存: %1").arg(compressedPath));
                
                // 自动解压并比对
                QFile compressedFile(compressedPath);
                if (compressedFile.open(QIODevice::ReadOnly)) {
                    compressedData = compressedFile.readAll();
                    compressedFile.close();
                    
                    // 保存解压后的文件
                    QString decompressedPath = buildPath + "/" + fileName + "_fuyuan.csv";
                    
                    infoTextEdit->append(QString("开始解压数据..."));
                    infoTextEdit->append(QString("压缩数据大小: %1 bytes").arg(compressedData.size()));
                    
                    // 解压数据
                    // 直接从压缩数据中提取原始数据，而不是通过tableWidget
                    QByteArray originalData;
                    char flag = compressedData.at(0);
                    infoTextEdit->append(QString("压缩格式标志: %1").arg((int)flag));
                    QByteArray content = compressedData.mid(1);
                    
                    if (flag == 2) {
                        infoTextEdit->append("使用原始CSV格式");
                        originalData = content;
                    } else if (flag == 6) {
                        infoTextEdit->append("使用完全保真压缩格式");
                        if (content.size() >= 2) {
                            int size = (unsigned char)content.at(0) | ((unsigned char)content.at(1) << 8);
                            infoTextEdit->append(QString("原始数据大小: %1").arg(size));
                            if (content.size() >= 2 + size) {
                                originalData = content.mid(2, size);
                                infoTextEdit->append(QString("提取的原始数据大小: %1").arg(originalData.size()));
                            } else {
                                infoTextEdit->append("数据大小不足，无法提取原始数据");
                            }
                        } else {
                            infoTextEdit->append("内容大小不足，无法提取原始数据");
                        }
                    } else if (flag == 7) {
                        infoTextEdit->append("使用RLE压缩格式");
                        int i = 0;
                        while (i < content.size()) {
                            if (i + 1 < content.size()) {
                                char current = content.at(i);
                                int count = (unsigned char)content.at(i + 1);
                                for (int j = 0; j < count; j++) {
                                    originalData.append(current);
                                }
                                i += 2;
                            } else {
                                break;
                            }
                        }
                        infoTextEdit->append(QString("RLE解压后数据大小: %1").arg(originalData.size()));
                    } else if (flag == 8) {
                        infoTextEdit->append("使用zlib压缩格式");
                        originalData = qUncompress(content);
                        infoTextEdit->append(QString("zlib解压后数据大小: %1").arg(originalData.size()));
                    } else if (flag == 9) {
                        infoTextEdit->append("使用字典压缩格式");
                        if (content.size() >= 3) {
                            int pos = 0;
                            int colCount = (unsigned char)content.at(pos++);
                            int rowCount = (unsigned char)content.at(pos++);
                            int dictSize = (unsigned char)content.at(pos++);
                            
                            // 读取字典
                            QStringList dict;
                            for (int i = 0; i < dictSize; ++i) {
                                if (pos < content.size()) {
                                    int valueLen = (unsigned char)content.at(pos++);
                                    if (pos + valueLen <= content.size()) {
                                        QByteArray valueBytes = content.mid(pos, valueLen);
                                        pos += valueLen;
                                        dict.append(QString::fromUtf8(valueBytes));
                                    }
                                }
                            }
                            
                            // 读取数据
                            QString csvContent;
                            int row = 0;
                            int col = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                int index = (unsigned char)content.at(pos++);
                                if (index < dict.size()) {
                                    csvContent += dict[index];
                                }
                                
                                col++;
                                if (col >= colCount) {
                                    csvContent += '\n';
                                    col = 0;
                                    row++;
                                } else {
                                    csvContent += ',';
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("字典解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else if (flag == 10) {
                        infoTextEdit->append("使用位级压缩格式");
                        if (content.size() >= 2) {
                            int pos = 0;
                            char headerInfo = content.at(pos++);
                            int colCount = headerInfo & 0x0F;
                            int rowCount = (headerInfo >> 4) & 0x0F;
                            int dictSize = (unsigned char)content.at(pos++) & 0x0F;
                            
                            // 读取字典
                            QStringList dict;
                            for (int i = 0; i < dictSize; ++i) {
                                if (pos < content.size()) {
                                    int valueLen = (unsigned char)content.at(pos++) & 0x0F;
                                    if (pos + valueLen <= content.size()) {
                                        QByteArray valueBytes = content.mid(pos, valueLen);
                                        pos += valueLen;
                                        dict.append(QString::fromUtf8(valueBytes));
                                    }
                                }
                            }
                            
                            // 读取数据
                            QString csvContent;
                            int row = 0;
                            int col = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                int index = (unsigned char)content.at(pos++) & 0x0F;
                                if (index < dict.size()) {
                                    csvContent += dict[index];
                                }
                                
                                col++;
                                if (col >= colCount) {
                                    csvContent += '\n';
                                    col = 0;
                                    row++;
                                } else {
                                    csvContent += ',';
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("位级解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else if (flag == 11) {
                        infoTextEdit->append("使用CSV专用压缩格式");
                        if (content.size() >= 2) {
                            int pos = 0;
                            char basicInfo = content.at(pos++);
                            int colCount = (basicInfo & 0x07) + 1;
                            int rowCount = ((basicInfo >> 3) & 0x07) + 1;
                            
                            int dictSize = (unsigned char)content.at(pos++);
                            
                            // 读取字典
                            QStringList dict;
                            for (int i = 0; i < dictSize; ++i) {
                                if (pos < content.size()) {
                                    int valueLen;
                                    if ((unsigned char)content.at(pos) == 0xF0) {
                                        // 长字符串
                                        pos++;
                                        if (pos < content.size()) {
                                            valueLen = (unsigned char)content.at(pos++);
                                        } else {
                                            break;
                                        }
                                    } else {
                                        // 短字符串
                                        valueLen = (unsigned char)content.at(pos++) & 0x0F;
                                    }
                                    
                                    if (pos + valueLen <= content.size()) {
                                        QByteArray valueBytes = content.mid(pos, valueLen);
                                        pos += valueLen;
                                        dict.append(QString::fromUtf8(valueBytes));
                                    }
                                }
                            }
                            
                            // 读取数据
                            QString csvContent;
                            int row = 0;
                            int col = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                int index;
                                if ((unsigned char)content.at(pos) < 0x10) {
                                    // 1字节编码
                                    index = (unsigned char)content.at(pos++);
                                } else {
                                    // 2字节编码
                                    if (pos + 1 < content.size()) {
                                        index = ((unsigned char)content.at(pos) & 0x0F) << 8;
                                        index |= (unsigned char)content.at(pos + 1);
                                        pos += 2;
                                    } else {
                                        break;
                                    }
                                }
                                
                                if (index < dict.size()) {
                                    csvContent += dict[index];
                                }
                                
                                col++;
                                if (col >= colCount) {
                                    csvContent += '\n';
                                    col = 0;
                                    row++;
                                } else {
                                    csvContent += ',';
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("CSV专用解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else if (flag == 12) {
                        infoTextEdit->append("使用极端压缩格式");
                        if (content.size() >= 2) {
                            int pos = 0;
                            char info = content.at(pos++);
                            int colCount = (info & 0x0F) + 1;
                            int rowCount = ((info >> 4) & 0x0F) + 1;
                            
                            int dictSize = (unsigned char)content.at(pos++);
                            
                            // 读取字典
                            QStringList dict;
                            for (int i = 0; i < dictSize; ++i) {
                                if (pos < content.size()) {
                                    int valueLen;
                                    if ((unsigned char)content.at(pos) < 128) {
                                        // 1字节长度
                                        valueLen = (unsigned char)content.at(pos++);
                                    } else {
                                        // 2字节长度
                                        if (pos + 1 < content.size()) {
                                            valueLen = ((unsigned char)content.at(pos) & 0x7F) << 8;
                                            valueLen |= (unsigned char)content.at(pos + 1);
                                            pos += 2;
                                        } else {
                                            break;
                                        }
                                    }
                                    
                                    if (pos + valueLen <= content.size()) {
                                        QByteArray valueBytes = content.mid(pos, valueLen);
                                        pos += valueLen;
                                        dict.append(QString::fromUtf8(valueBytes));
                                    }
                                }
                            }
                            
                            // 读取数据
                            QString csvContent;
                            int row = 0;
                            int col = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                int index;
                                if ((unsigned char)content.at(pos) < 128) {
                                    // 1字节编码
                                    index = (unsigned char)content.at(pos++);
                                } else {
                                    // 2字节编码
                                    if (pos + 1 < content.size()) {
                                        index = ((unsigned char)content.at(pos) & 0x7F) << 8;
                                        index |= (unsigned char)content.at(pos + 1);
                                        pos += 2;
                                    } else {
                                        break;
                                    }
                                }
                                
                                if (index < dict.size()) {
                                    csvContent += dict[index];
                                }
                                
                                col++;
                                if (col >= colCount) {
                                    csvContent += '\n';
                                    col = 0;
                                    row++;
                                } else {
                                    csvContent += ',';
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("极端解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else if (flag == 13) {
                        infoTextEdit->append("使用数字专用压缩格式");
                        if (content.size() >= 1) {
                            int pos = 0;
                            int rowCount = (unsigned char)content.at(pos++);
                            
                            QString csvContent;
                            int row = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                if (pos < content.size()) {
                                    int fieldCount = (unsigned char)content.at(pos++) & 0x0F;
                                    
                                    for (int f = 0; f < fieldCount; f++) {
                                        if (pos < content.size()) {
                                            int fieldLen = (unsigned char)content.at(pos++) & 0x0F;
                                            
                                            QString field;
                                            int remaining = fieldLen;
                                            
                                            while (pos < content.size() && remaining > 0) {
                                                char encoded = content.at(pos++);
                                                int digit = (encoded >> 4) & 0x0F;
                                                int count = (encoded & 0x0F) + 1;
                                                
                                                for (int c = 0; c < count && remaining > 0; c++) {
                                                    field.append(QChar('0' + digit));
                                                    remaining--;
                                                }
                                            }
                                            
                                            csvContent += field;
                                            if (f < fieldCount - 1) {
                                                csvContent += ',';
                                            }
                                        }
                                    }
                                    
                                    csvContent += '\n';
                                    row++;
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("数字专用解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else if (flag == 14) {
                        infoTextEdit->append("使用超级压缩格式");
                        if (content.size() >= 2) {
                            int pos = 0;
                            int rowCount = (unsigned char)content.at(pos++);
                            int patternCount = (unsigned char)content.at(pos++);
                            
                            // 读取模式字典
                            QStringList patterns;
                            for (int i = 0; i < patternCount; i++) {
                                if (pos < content.size()) {
                                    int patternLen = (unsigned char)content.at(pos++) & 0x0F;
                                    if (pos + patternLen <= content.size()) {
                                        QByteArray patternBytes = content.mid(pos, patternLen);
                                        pos += patternLen;
                                        patterns.append(QString::fromUtf8(patternBytes));
                                    }
                                }
                            }
                            
                            // 读取数据
                            QString csvContent;
                            int row = 0;
                            
                            while (pos < content.size() && row < rowCount) {
                                if (pos < content.size()) {
                                    int fieldCount = (unsigned char)content.at(pos++) & 0x0F;
                                    
                                    for (int f = 0; f < fieldCount; f++) {
                                        if (pos < content.size()) {
                                            int patternIndex = (unsigned char)content.at(pos++) & 0x0F;
                                            if (patternIndex < patterns.size()) {
                                                csvContent += patterns[patternIndex];
                                            }
                                            if (f < fieldCount - 1) {
                                                csvContent += ',';
                                            }
                                        }
                                    }
                                    
                                    csvContent += '\n';
                                    row++;
                                }
                            }
                            
                            originalData = csvContent.toUtf8();
                            infoTextEdit->append(QString("超级解压后数据大小: %1").arg(originalData.size()));
                        }
                    } else {
                        infoTextEdit->append(QString("未知压缩格式: %1").arg((int)flag));
                    }
                    
                    if (!originalData.isEmpty()) {
                        infoTextEdit->append(QString("解压成功，原始数据大小: %1 bytes").arg(originalData.size()));
                        
                        // 保存解压后的文件
                        QSaveFile saveFile(decompressedPath);
                        if (saveFile.open(QIODevice::WriteOnly)) {
                            qint64 bytesWritten = saveFile.write(originalData);
                            infoTextEdit->append(QString("写入文件字节数: %1").arg(bytesWritten));
                            if (saveFile.commit()) {
                                infoTextEdit->append(QString("解压文件保存成功: %1").arg(decompressedPath));
                                
                                // 保存为core_used文件
                                QString coreUsedPath = buildPath + "/core_used";
                                QSaveFile coreUsedFile(coreUsedPath);
                                if (coreUsedFile.open(QIODevice::WriteOnly)) {
                                    coreUsedFile.write(originalData);
                                    if (coreUsedFile.commit()) {
                                        infoTextEdit->append(QString("core_used文件保存成功: %1").arg(coreUsedPath));
                                    } else {
                                        infoTextEdit->append(QString("core_used文件保存失败: %1").arg(coreUsedFile.errorString()));
                                    }
                                } else {
                                    infoTextEdit->append(QString("无法打开core_used文件: %1").arg(coreUsedFile.errorString()));
                                }
                                
                                // 加载到还原表格
                                infoTextEdit->append(QString("正在加载还原内容到表格: %1").arg(decompressedPath));
                                bool loaded = loadCSVFile(decompressedPath, restoredTableWidget);
                                if (loaded) {
                                    infoTextEdit->append("还原内容加载成功!");
                                } else {
                                    infoTextEdit->append("还原内容加载失败!");
                                }
                                
                                // 比对文件
                                bool isIdentical = compareFiles(currentFilePath, decompressedPath);
                                if (isIdentical) {
                                    infoTextEdit->append("还原成功! 与原始文件完全一致");
                                } else {
                                    infoTextEdit->append("还原失败! 与原始文件不一致");
                                }
                            } else {
                                infoTextEdit->append(QString("保存解压文件失败: %1").arg(saveFile.errorString()));
                            }
                        } else {
                            infoTextEdit->append(QString("无法打开解压文件: %1").arg(saveFile.errorString()));
                        }
                    } else {
                        infoTextEdit->append("解压失败，原始数据为空");
                    }
                }
            } else {
                infoTextEdit->append(QString("保存压缩文件失败: %1").arg(file.errorString()));
            }
        } else {
            infoTextEdit->append("无法打开压缩文件进行保存");
        }
    } else {
        infoTextEdit->append("压缩失败!");
    }
}



bool MainWindow::loadCSVFile(const QString &filePath, QTableWidget *table)
{
    if (!table) {
        table = tableWidget;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "警告", "无法打开CSV文件");
        return false;
    }
    
    QTextStream in(&file);
    table->clearContents();
    table->setRowCount(0);
    
    int row = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(",");
        
        table->insertRow(row);
        int col = 0;
        for (const QString &field : fields) {
            if (col >= table->columnCount()) {
                table->insertColumn(col);
                table->setHorizontalHeaderItem(col, new QTableWidgetItem(QChar('A' + col)));
            }
            QTableWidgetItem *item = new QTableWidgetItem(field);
            table->setItem(row, col, item);
            col++;
        }
        row++;
    }
    
    file.close();
    return true;
}

bool MainWindow::compressData()
{
    try {
        // 直接保存原始文件内容，确保完全一致
        if (!currentFilePath.isEmpty()) {
            QFile originalFile(currentFilePath);
            if (originalFile.open(QIODevice::ReadOnly)) {
                QByteArray originalData = originalFile.readAll();
                originalFile.close();
                
                // 尝试多种压缩算法，选择最优的
                QByteArray bestCompressed;
                int bestSize = originalData.size();
                char bestFlag = 2; // 默认原始格式
                
                // 1. 尝试zlib压缩（最高压缩级别）
                QByteArray zlibCompressed = qCompress(originalData, 9);
                if (zlibCompressed.size() < bestSize) {
                    bestCompressed = zlibCompressed;
                    bestSize = zlibCompressed.size();
                    bestFlag = 8; // 标志位：8表示zlib压缩格式
                }
                
                // 2. 尝试Run-Length Encoding (RLE)
                QByteArray rleData;
                rleData.append(char(7)); // 标志位：7表示RLE压缩格式
                
                int i = 0;
                while (i < originalData.size()) {
                    char current = originalData[i];
                    int count = 1;
                    while (i + count < originalData.size() && originalData[i + count] == current && count < 255) {
                        count++;
                    }
                    rleData.append(current);
                    rleData.append(char(count));
                    i += count;
                }
                
                if (rleData.size() < bestSize) {
                    bestCompressed = rleData;
                    bestSize = rleData.size();
                    bestFlag = 7;
                }
                
                // 3. 对于小文件，尝试特殊压缩格式
                if (originalData.size() < 200) {
                    QByteArray compactData;
                    compactData.append(char(6)); // 标志位：6表示完全保真压缩格式
                    
                    // 存储原始数据大小（2字节）
                    int size = originalData.size();
                    compactData.append(char(size & 0xFF));
                    compactData.append(char((size >> 8) & 0xFF));
                    
                    // 存储原始数据
                    compactData.append(originalData);
                    
                    if (compactData.size() < bestSize) {
                        bestCompressed = compactData;
                        bestSize = compactData.size();
                        bestFlag = 6;
                    }
                }
                
                // 4. 尝试字典压缩（针对CSV文件优化）
                QByteArray dictCompressed;
                dictCompressed.append(char(9)); // 标志位：9表示字典压缩格式
                
                // 解析CSV文件
                QString originalString = QString::fromUtf8(originalData);
                QStringList lines = originalString.split("\n");
                if (lines.size() > 0) {
                    // 提取表头
                    QStringList headers = lines[0].split(",");
                    int colCount = headers.size();
                    
                    // 收集所有唯一值作为字典
                    QStringList dict;
                    QHash<QString, int> valueToIndex;
                    
                    // 先添加表头到字典
                    for (const QString &header : headers) {
                        if (!valueToIndex.contains(header)) {
                            valueToIndex[header] = dict.size();
                            dict.append(header);
                        }
                    }
                    
                    // 再添加数据到字典
                    for (int i = 1; i < lines.size(); i++) {
                        QStringList fields = lines[i].split(",");
                        for (const QString &field : fields) {
                            if (!valueToIndex.contains(field)) {
                                valueToIndex[field] = dict.size();
                                dict.append(field);
                            }
                        }
                    }
                    
                    // 写入列数和行数
                    dictCompressed.append(char(colCount));
                    dictCompressed.append(char(lines.size()));
                    
                    // 写入字典大小
                    dictCompressed.append(char(dict.size()));
                    
                    // 写入字典内容
                    for (const QString &value : dict) {
                        QByteArray valueBytes = value.toUtf8();
                        dictCompressed.append(char(valueBytes.size()));
                        dictCompressed.append(valueBytes);
                    }
                    
                    // 写入数据（使用字典索引）
                    for (int i = 0; i < lines.size(); i++) {
                        QStringList fields = lines[i].split(",");
                        for (const QString &field : fields) {
                            int index = valueToIndex[field];
                            dictCompressed.append(char(index));
                        }
                    }
                    
                    if (dictCompressed.size() < bestSize) {
                        bestCompressed = dictCompressed;
                        bestSize = dictCompressed.size();
                        bestFlag = 9;
                    }
                }
                
                // 5. 尝试位级压缩（针对CSV文件优化）
                QByteArray bitCompressed;
                bitCompressed.append(char(10)); // 标志位：10表示位级压缩格式
                
                // 解析CSV文件
                QStringList lines2 = originalString.split("\n");
                if (lines2.size() > 0) {
                    QStringList headers2 = lines2[0].split(",");
                    int colCount2 = headers2.size();
                    int rowCount2 = lines2.size();
                    
                    // 写入列数和行数（4位 each）
                    char headerInfo = (colCount2 & 0x0F) | ((rowCount2 & 0x0F) << 4);
                    bitCompressed.append(headerInfo);
                    
                    // 构建字典
                    QStringList dict2;
                    QHash<QString, int> valueToIndex2;
                    
                    for (const QString &line : lines2) {
                        QStringList fields = line.split(",");
                        for (const QString &field : fields) {
                            if (!valueToIndex2.contains(field)) {
                                valueToIndex2[field] = dict2.size();
                                dict2.append(field);
                            }
                        }
                    }
                    
                    // 写入字典大小（4位）
                    bitCompressed.append(char(dict2.size() & 0x0F));
                    
                    // 写入字典内容
                    for (const QString &value : dict2) {
                        QByteArray valueBytes = value.toUtf8();
                        // 写入长度（4位）
                        bitCompressed.append(char(valueBytes.size() & 0x0F));
                        // 写入内容
                        bitCompressed.append(valueBytes);
                    }
                    
                    // 写入数据（使用4位索引）
                    for (const QString &line : lines2) {
                        QStringList fields = line.split(",");
                        for (const QString &field : fields) {
                            int index = valueToIndex2[field];
                            bitCompressed.append(char(index & 0x0F));
                        }
                    }
                    
                    if (bitCompressed.size() < bestSize) {
                        bestCompressed = bitCompressed;
                        bestSize = bitCompressed.size();
                        bestFlag = 10;
                    }
                }
                
                // 6. 尝试CSV专用压缩格式（针对小型CSV文件优化）
                QByteArray csvCompressed;
                csvCompressed.append(char(11)); // 标志位：11表示CSV专用压缩格式
                
                // 解析CSV文件
                QStringList csvLines = originalString.split("\n");
                if (csvLines.size() > 0) {
                    QStringList headers = csvLines[0].split(",");
                    int colCount = headers.size();
                    int rowCount = csvLines.size();
                    
                    // 写入基本信息（1字节）
                    char basicInfo = (colCount & 0x07) | ((rowCount & 0x07) << 3) | ((headers.size() & 0x01) << 6);
                    csvCompressed.append(basicInfo);
                    
                    // 构建优化字典
                    QHash<QString, int> optimizedDict;
                    QStringList dictValues;
                    int dictIndex = 0;
                    
                    // 特殊处理表头
                    for (const QString &header : headers) {
                        if (!optimizedDict.contains(header)) {
                            optimizedDict[header] = dictIndex++;
                            dictValues.append(header);
                        }
                    }
                    
                    // 处理数据行
                    for (int i = 1; i < csvLines.size(); i++) {
                        QStringList fields = csvLines[i].split(",");
                        for (const QString &field : fields) {
                            if (!optimizedDict.contains(field)) {
                                optimizedDict[field] = dictIndex++;
                                dictValues.append(field);
                            }
                        }
                    }
                    
                    // 写入字典大小（1字节）
                    csvCompressed.append(char(dictValues.size()));
                    
                    // 写入字典内容（优化存储）
                    for (const QString &value : dictValues) {
                        // 优化字符串存储：使用长度前缀 + 内容
                        QByteArray valueBytes = value.toUtf8();
                        if (valueBytes.size() <= 15) {
                            // 短字符串：4位长度 + 内容
                            char lenByte = valueBytes.size() & 0x0F;
                            csvCompressed.append(lenByte);
                            csvCompressed.append(valueBytes);
                        } else {
                            // 长字符串：特殊标记 + 1字节长度 + 内容
                            csvCompressed.append(char(0xF0)); // 特殊标记
                            csvCompressed.append(char(valueBytes.size()));
                            csvCompressed.append(valueBytes);
                        }
                    }
                    
                    // 写入数据（使用可变位长编码）
                    for (const QString &line : csvLines) {
                        QStringList fields = line.split(",");
                        for (const QString &field : fields) {
                            int index = optimizedDict[field];
                            // 使用可变位长编码
                            if (index < 16) {
                                // 1字节编码
                                csvCompressed.append(char(index));
                            } else if (index < 256) {
                                // 2字节编码
                                csvCompressed.append(char(0x10 | (index >> 8)));
                                csvCompressed.append(char(index & 0xFF));
                            }
                        }
                    }
                    
                    if (csvCompressed.size() < bestSize) {
                        bestCompressed = csvCompressed;
                        bestSize = csvCompressed.size();
                        bestFlag = 11;
                    }
                }
                
                // 7. 尝试极端压缩（针对小型CSV文件）
                QByteArray extremeCompressed;
                extremeCompressed.append(char(12)); // 标志位：12表示极端压缩格式
                
                // 只处理小型CSV文件
                if (originalData.size() < 200) {
                    // 分析CSV结构
                    QStringList lines = originalString.split("\n");
                    if (lines.size() > 0) {
                        QStringList headers = lines[0].split(",");
                        int colCount = headers.size();
                        int rowCount = lines.size();
                        
                        // 写入基本信息（1字节）
                        char info = (colCount - 1) | ((rowCount - 1) << 4);
                        extremeCompressed.append(info);
                        
                        // 构建共享字典
                        QHash<QString, int> sharedDict;
                        QStringList dictEntries;
                        
                        // 收集所有唯一值
                        for (const QString &line : lines) {
                            QStringList fields = line.split(",");
                            for (const QString &field : fields) {
                                if (!sharedDict.contains(field)) {
                                    sharedDict[field] = dictEntries.size();
                                    dictEntries.append(field);
                                }
                            }
                        }
                        
                        // 写入字典大小（1字节）
                        extremeCompressed.append(char(dictEntries.size()));
                        
                        // 优化字典存储
                        for (const QString &entry : dictEntries) {
                            QByteArray entryBytes = entry.toUtf8();
                            // 使用7位编码长度
                            int len = entryBytes.size();
                            if (len <= 127) {
                                extremeCompressed.append(char(len));
                            } else {
                                extremeCompressed.append(char(128 | (len >> 8)));
                                extremeCompressed.append(char(len & 0xFF));
                            }
                            extremeCompressed.append(entryBytes);
                        }
                        
                        // 紧凑存储数据索引
                        for (const QString &line : lines) {
                            QStringList fields = line.split(",");
                            for (const QString &field : fields) {
                                int index = sharedDict[field];
                                // 使用变长编码
                                if (index < 128) {
                                    extremeCompressed.append(char(index));
                                } else if (index < 16384) {
                                    extremeCompressed.append(char(128 | (index >> 8)));
                                    extremeCompressed.append(char(index & 0xFF));
                                }
                            }
                        }
                        
                        if (extremeCompressed.size() < bestSize) {
                            bestCompressed = extremeCompressed;
                            bestSize = extremeCompressed.size();
                            bestFlag = 12;
                        }
                    }
                }
                
                // 8. 尝试数字专用压缩（针对数字数据优化）
                QByteArray numberCompressed;
                numberCompressed.append(char(13)); // 标志位：13表示数字专用压缩格式
                
                // 分析数据是否主要为数字
                bool isNumeric = true;
                for (int i = 0; i < originalData.size(); i++) {
                    char c = originalData.at(i);
                    if (!((c >= '0' && c <= '9') || c == '\n' || c == ',')) {
                        isNumeric = false;
                        break;
                    }
                }
                
                if (isNumeric) {
                    QStringList lines = originalString.split("\n");
                    int rowCount = lines.size();
                    
                    // 写入行数（1字节）
                    numberCompressed.append(char(rowCount));
                    
                    // 分析每行数据的模式
                    for (const QString &line : lines) {
                        QStringList fields = line.split(",");
                        int fieldCount = fields.size();
                        
                        // 写入字段数（4位）
                        numberCompressed.append(char(fieldCount & 0x0F));
                        
                        for (const QString &field : fields) {
                            // 针对数字字符串的特殊压缩
                            QByteArray fieldBytes = field.toUtf8();
                            int len = fieldBytes.size();
                            
                            // 写入长度（4位）
                            numberCompressed.append(char(len & 0x0F));
                            
                            // 压缩数字序列
                            int i = 0;
                            while (i < len) {
                                char current = fieldBytes.at(i);
                                int count = 1;
                                
                                // 计算连续相同数字的个数
                                while (i + count < len && fieldBytes.at(i + count) == current && count < 15) {
                                    count++;
                                }
                                
                                // 写入数字和重复次数（4位数字 + 4位重复次数）
                                char encoded = ((current - '0') << 4) | (count - 1);
                                numberCompressed.append(encoded);
                                
                                i += count;
                            }
                        }
                    }
                    
                    if (numberCompressed.size() < bestSize) {
                        bestCompressed = numberCompressed;
                        bestSize = numberCompressed.size();
                        bestFlag = 13;
                    }
                }
                
                // 9. 尝试超级压缩（针对特定模式的数字数据）
                QByteArray superCompressed;
                superCompressed.append(char(14)); // 标志位：14表示超级压缩格式
                
                if (isNumeric) {
                    QStringList lines = originalString.split("\n");
                    int rowCount = lines.size();
                    
                    // 写入行数（1字节）
                    superCompressed.append(char(rowCount));
                    
                    // 构建数字模式字典
                    QHash<QString, int> patternDict;
                    QStringList patterns;
                    int patternIndex = 0;
                    
                    // 提取所有数字模式
                    for (const QString &line : lines) {
                        QStringList fields = line.split(",");
                        for (const QString &field : fields) {
                            if (!patternDict.contains(field)) {
                                patternDict[field] = patternIndex++;
                                patterns.append(field);
                            }
                        }
                    }
                    
                    // 写入模式数量（1字节）
                    superCompressed.append(char(patterns.size()));
                    
                    // 写入模式数据
                    for (const QString &pattern : patterns) {
                        // 压缩模式存储
                        QByteArray patternBytes = pattern.toUtf8();
                        int len = patternBytes.size();
                        
                        // 写入长度（4位）
                        superCompressed.append(char(len & 0x0F));
                        
                        // 写入模式内容
                        superCompressed.append(patternBytes);
                    }
                    
                    // 写入数据索引
                    for (const QString &line : lines) {
                        QStringList fields = line.split(",");
                        int fieldCount = fields.size();
                        
                        // 写入字段数（4位）
                        superCompressed.append(char(fieldCount & 0x0F));
                        
                        // 写入每个字段的模式索引（4位 each）
                        for (const QString &field : fields) {
                            int index = patternDict[field];
                            superCompressed.append(char(index & 0x0F));
                        }
                    }
                    
                    if (superCompressed.size() < bestSize) {
                        bestCompressed = superCompressed;
                        bestSize = superCompressed.size();
                        bestFlag = 14;
                    }
                }
                
                // 如果所有压缩算法都不如原始数据，使用原始数据
                if (bestFlag == 2) {
                    compressedData = originalData;
                    compressedData.prepend(char(2)); // 标志位：2表示原始CSV格式
                } else {
                    // 对于其他压缩格式，确保添加了标志位
                    // 检查bestCompressed是否已经包含标志位
                    if (bestCompressed.size() > 0) {
                        char firstByte = bestCompressed.at(0);
                        if (firstByte != bestFlag) {
                            // 如果没有标志位，添加它
                            compressedData = bestCompressed;
                            compressedData.prepend(bestFlag);
                        } else {
                            // 已经有标志位，直接使用
                            compressedData = bestCompressed;
                        }
                    } else {
                        // 如果压缩数据为空，使用原始数据
                        compressedData = originalData;
                        compressedData.prepend(char(2));
                    }
                }
                
                return true;
            }
        }
        
        // 如果无法读取原始文件，使用表格数据
        // 构建CSV内容，确保与原始文件格式一致
        QString csvContent;
        int rows = tableWidget->rowCount();
        int cols = tableWidget->columnCount();
        
        for (int i = 0; i < rows; ++i) {
            QString rowData;
            for (int j = 0; j < cols; ++j) {
                QTableWidgetItem *item = tableWidget->item(i, j);
                QString text = item ? item->text() : "";
                rowData += text;
                if (j < cols - 1) rowData += ",";
            }
            csvContent += rowData;
            if (i < rows - 1) csvContent += "\n";
        }
        
        QByteArray data = csvContent.toUtf8();
        compressedData = data;
        compressedData.prepend(char(2)); // 标志位：2表示原始CSV格式
        return true;
    } catch (...) {
        return false;
    }
}

bool MainWindow::decompressData()
{
    try {
        if (compressedData.size() == 0) {
            return false;
        }
        
        char flag = compressedData.at(0);
        QByteArray content = compressedData.mid(1);
        
        // 处理原始CSV格式
        if (flag == 2) {
            // 直接使用loadCSVFile方法加载
            // 先保存到临时文件
            QTemporaryFile tempFile;
            if (tempFile.open()) {
                tempFile.write(content);
                tempFile.close();
                return loadCSVFile(tempFile.fileName());
            }
            return false;
        }
        
        // 处理完全保真压缩格式
        if (flag == 6) {
            if (content.size() < 2) {
                return false;
            }
            
            // 读取原始数据大小
            int size = (unsigned char)content.at(0) | ((unsigned char)content.at(1) << 8);
            
            if (content.size() >= 2 + size) {
                QByteArray originalData = content.mid(2, size);
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(originalData);
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理RLE压缩格式
        if (flag == 7) {
            QByteArray originalData;
            int i = 0;
            while (i < content.size()) {
                if (i + 1 < content.size()) {
                    char current = content.at(i);
                    int count = (unsigned char)content.at(i + 1);
                    for (int j = 0; j < count; j++) {
                        originalData.append(current);
                    }
                    i += 2;
                } else {
                    break;
                }
            }
            
            // 保存到临时文件并加载
            QTemporaryFile tempFile;
            if (tempFile.open()) {
                tempFile.write(originalData);
                tempFile.close();
                return loadCSVFile(tempFile.fileName());
            }
            return false;
        }
        
        // 处理zlib压缩格式
        if (flag == 8) {
            QByteArray originalData = qUncompress(content);
            
            // 保存到临时文件并加载
            QTemporaryFile tempFile;
            if (tempFile.open()) {
                tempFile.write(originalData);
                tempFile.close();
                return loadCSVFile(tempFile.fileName());
            }
            return false;
        }
        
        // 处理字典压缩格式
        if (flag == 9) {
            if (content.size() >= 3) {
                int pos = 0;
                int colCount = (unsigned char)content.at(pos++);
                int rowCount = (unsigned char)content.at(pos++);
                int dictSize = (unsigned char)content.at(pos++);
                
                // 读取字典
                QStringList dict;
                for (int i = 0; i < dictSize; ++i) {
                    if (pos < content.size()) {
                        int valueLen = (unsigned char)content.at(pos++);
                        if (pos + valueLen <= content.size()) {
                            QByteArray valueBytes = content.mid(pos, valueLen);
                            pos += valueLen;
                            dict.append(QString::fromUtf8(valueBytes));
                        }
                    }
                }
                
                // 读取数据
                QString csvContent;
                int row = 0;
                int col = 0;
                
                while (pos < content.size() && row < rowCount) {
                    int index = (unsigned char)content.at(pos++);
                    if (index < dict.size()) {
                        csvContent += dict[index];
                    }
                    
                    col++;
                    if (col >= colCount) {
                        csvContent += '\n';
                        col = 0;
                        row++;
                    } else {
                        csvContent += ',';
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理位级压缩格式
        if (flag == 10) {
            if (content.size() >= 2) {
                int pos = 0;
                char headerInfo = content.at(pos++);
                int colCount = headerInfo & 0x0F;
                int rowCount = (headerInfo >> 4) & 0x0F;
                int dictSize = (unsigned char)content.at(pos++) & 0x0F;
                
                // 读取字典
                QStringList dict;
                for (int i = 0; i < dictSize; ++i) {
                    if (pos < content.size()) {
                        int valueLen = (unsigned char)content.at(pos++) & 0x0F;
                        if (pos + valueLen <= content.size()) {
                            QByteArray valueBytes = content.mid(pos, valueLen);
                            pos += valueLen;
                            dict.append(QString::fromUtf8(valueBytes));
                        }
                    }
                }
                
                // 读取数据
                QString csvContent;
                int row = 0;
                int col = 0;
                
                while (pos < content.size() && row < rowCount) {
                    int index = (unsigned char)content.at(pos++) & 0x0F;
                    if (index < dict.size()) {
                        csvContent += dict[index];
                    }
                    
                    col++;
                    if (col >= colCount) {
                        csvContent += '\n';
                        col = 0;
                        row++;
                    } else {
                        csvContent += ',';
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理CSV专用压缩格式
        if (flag == 11) {
            if (content.size() >= 2) {
                int pos = 0;
                char basicInfo = content.at(pos++);
                int colCount = (basicInfo & 0x07) + 1;
                int rowCount = ((basicInfo >> 3) & 0x07) + 1;
                
                int dictSize = (unsigned char)content.at(pos++);
                
                // 读取字典
                QStringList dict;
                for (int i = 0; i < dictSize; ++i) {
                    if (pos < content.size()) {
                        int valueLen;
                        if ((unsigned char)content.at(pos) == 0xF0) {
                            // 长字符串
                            pos++;
                            if (pos < content.size()) {
                                valueLen = (unsigned char)content.at(pos++);
                            } else {
                                break;
                            }
                        } else {
                            // 短字符串
                            valueLen = (unsigned char)content.at(pos++) & 0x0F;
                        }
                        
                        if (pos + valueLen <= content.size()) {
                            QByteArray valueBytes = content.mid(pos, valueLen);
                            pos += valueLen;
                            dict.append(QString::fromUtf8(valueBytes));
                        }
                    }
                }
                
                // 读取数据
                QString csvContent;
                int row = 0;
                int col = 0;
                
                while (pos < content.size() && row < rowCount) {
                    int index;
                    if ((unsigned char)content.at(pos) < 0x10) {
                        // 1字节编码
                        index = (unsigned char)content.at(pos++);
                    } else {
                        // 2字节编码
                        if (pos + 1 < content.size()) {
                            index = ((unsigned char)content.at(pos) & 0x0F) << 8;
                            index |= (unsigned char)content.at(pos + 1);
                            pos += 2;
                        } else {
                            break;
                        }
                    }
                    
                    if (index < dict.size()) {
                        csvContent += dict[index];
                    }
                    
                    col++;
                    if (col >= colCount) {
                        csvContent += '\n';
                        col = 0;
                        row++;
                    } else {
                        csvContent += ',';
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理极端压缩格式
        if (flag == 12) {
            if (content.size() >= 2) {
                int pos = 0;
                char info = content.at(pos++);
                int colCount = (info & 0x0F) + 1;
                int rowCount = ((info >> 4) & 0x0F) + 1;
                
                int dictSize = (unsigned char)content.at(pos++);
                
                // 读取字典
                QStringList dict;
                for (int i = 0; i < dictSize; ++i) {
                    if (pos < content.size()) {
                        int valueLen;
                        if ((unsigned char)content.at(pos) < 128) {
                            // 1字节长度
                            valueLen = (unsigned char)content.at(pos++);
                        } else {
                            // 2字节长度
                            if (pos + 1 < content.size()) {
                                valueLen = ((unsigned char)content.at(pos) & 0x7F) << 8;
                                valueLen |= (unsigned char)content.at(pos + 1);
                                pos += 2;
                            } else {
                                break;
                            }
                        }
                        
                        if (pos + valueLen <= content.size()) {
                            QByteArray valueBytes = content.mid(pos, valueLen);
                            pos += valueLen;
                            dict.append(QString::fromUtf8(valueBytes));
                        }
                    }
                }
                
                // 读取数据
                QString csvContent;
                int row = 0;
                int col = 0;
                
                while (pos < content.size() && row < rowCount) {
                    int index;
                    if ((unsigned char)content.at(pos) < 128) {
                        // 1字节编码
                        index = (unsigned char)content.at(pos++);
                    } else {
                        // 2字节编码
                        if (pos + 1 < content.size()) {
                            index = ((unsigned char)content.at(pos) & 0x7F) << 8;
                            index |= (unsigned char)content.at(pos + 1);
                            pos += 2;
                        } else {
                            break;
                        }
                    }
                    
                    if (index < dict.size()) {
                        csvContent += dict[index];
                    }
                    
                    col++;
                    if (col >= colCount) {
                        csvContent += '\n';
                        col = 0;
                        row++;
                    } else {
                        csvContent += ',';
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理数字专用压缩格式
        if (flag == 13) {
            if (content.size() >= 1) {
                int pos = 0;
                int rowCount = (unsigned char)content.at(pos++);
                
                QString csvContent;
                int row = 0;
                
                while (pos < content.size() && row < rowCount) {
                    if (pos < content.size()) {
                        int fieldCount = (unsigned char)content.at(pos++) & 0x0F;
                        
                        for (int f = 0; f < fieldCount; f++) {
                            if (pos < content.size()) {
                                int fieldLen = (unsigned char)content.at(pos++) & 0x0F;
                                
                                QString field;
                                int remaining = fieldLen;
                                
                                while (pos < content.size() && remaining > 0) {
                                    char encoded = content.at(pos++);
                                    int digit = (encoded >> 4) & 0x0F;
                                    int count = (encoded & 0x0F) + 1;
                                    
                                    for (int c = 0; c < count && remaining > 0; c++) {
                                        field.append(QChar('0' + digit));
                                        remaining--;
                                    }
                                }
                                
                                csvContent += field;
                                if (f < fieldCount - 1) {
                                    csvContent += ',';
                                }
                            }
                        }
                        
                        csvContent += '\n';
                        row++;
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理超级压缩格式
        if (flag == 14) {
            if (content.size() >= 2) {
                int pos = 0;
                int rowCount = (unsigned char)content.at(pos++);
                int patternCount = (unsigned char)content.at(pos++);
                
                // 读取模式字典
                QStringList patterns;
                for (int i = 0; i < patternCount; i++) {
                    if (pos < content.size()) {
                        int patternLen = (unsigned char)content.at(pos++) & 0x0F;
                        if (pos + patternLen <= content.size()) {
                            QByteArray patternBytes = content.mid(pos, patternLen);
                            pos += patternLen;
                            patterns.append(QString::fromUtf8(patternBytes));
                        }
                    }
                }
                
                // 读取数据
                QString csvContent;
                int row = 0;
                
                while (pos < content.size() && row < rowCount) {
                    if (pos < content.size()) {
                        int fieldCount = (unsigned char)content.at(pos++) & 0x0F;
                        
                        for (int f = 0; f < fieldCount; f++) {
                            if (pos < content.size()) {
                                int patternIndex = (unsigned char)content.at(pos++) & 0x0F;
                                if (patternIndex < patterns.size()) {
                                    csvContent += patterns[patternIndex];
                                }
                                if (f < fieldCount - 1) {
                                    csvContent += ',';
                                }
                            }
                        }
                        
                        csvContent += '\n';
                        row++;
                    }
                }
                
                // 保存到临时文件并加载
                QTemporaryFile tempFile;
                if (tempFile.open()) {
                    tempFile.write(csvContent.toUtf8());
                    tempFile.close();
                    return loadCSVFile(tempFile.fileName());
                }
            }
            return false;
        }
        
        // 处理其他格式（保持兼容性）
        // 处理极致压缩格式（字典压缩）
        if (flag == 4) {
            if (content.size() < 3) {
                return false;
            }
            
            int pos = 0;
            // 列数（4位）
            int cols = (unsigned char)content.at(pos++) & 0x0F;
            // 行数（4位）
            int rows = (unsigned char)content.at(pos++) & 0x0F;
            // 字典大小（4位）
            int dictSize = (unsigned char)content.at(pos++) & 0x0F;
            
            // 读取字典
            QList<QString> dictItems;
            for (int i = 0; i < dictSize; ++i) {
                if (pos < content.size()) {
                    int itemLen = (unsigned char)content.at(pos++) & 0x0F;
                    if (pos + itemLen <= content.size()) {
                        QByteArray itemBytes = content.mid(pos, itemLen);
                        pos += itemLen;
                        dictItems.append(QString::fromUtf8(itemBytes));
                    }
                }
            }
            
            // 读取数据
            QString csvContent;
            int row = 0;
            int col = 0;
            
            while (pos < content.size() && row < rows) {
                int index = (unsigned char)content.at(pos++) & 0x0F;
                if (index < dictItems.size()) {
                    csvContent += dictItems[index];
                }
                
                col++;
                if (col >= cols) {
                    csvContent += '\n';
                    col = 0;
                    row++;
                } else {
                    csvContent += ',';
                }
            }
            
            // 保存到临时文件并加载
            QTemporaryFile tempFile;
            if (tempFile.open()) {
                tempFile.write(csvContent.toUtf8());
                tempFile.close();
                return loadCSVFile(tempFile.fileName());
            }
            return false;
        }
        
        // 处理位级压缩格式
        if (flag == 5) {
            if (content.size() < 2) {
                return false;
            }
            
            int pos = 0;
            // 列数（4位）
            int cols = (unsigned char)content.at(pos++) & 0x0F;
            // 行数（4位）
            int rows = (unsigned char)content.at(pos++) & 0x0F;
            
            // 转换为QBitArray
            QBitArray bits;
            QByteArray bitData = content.mid(pos);
            bits.resize(bitData.size() * 8);
            for (int i = 0; i < bitData.size() * 8; ++i) {
                bits.setBit(i, (bitData[i / 8] >> (i % 8)) & 1);
            }
            
            // 解析位数据
            QString csvContent;
            int bitPos = 0;
            int row = 0;
            int col = 0;
            
            while (bitPos < bits.size() && row < rows) {
                if (bitPos + 4 <= bits.size()) {
                    // 检查是否是数字（前4位）
                    bool isNumber = true;
                    for (int i = 0; i < 4; ++i) {
                        if (bits.testBit(bitPos + i)) {
                            isNumber = false;
                            break;
                        }
                    }
                    
                    if (isNumber) {
                        // 数字
                        int num = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (bits.testBit(bitPos + i)) {
                                num |= (1 << i);
                            }
                        }
                        csvContent += QString::number(num);
                        bitPos += 4;
                    } else {
                        // 字符串
                        bitPos += 1; // 跳过标志位
                        
                        // 读取长度（4位）
                        int textLen = 0;
                        if (bitPos + 4 <= bits.size()) {
                            for (int i = 0; i < 4; ++i) {
                                if (bits.testBit(bitPos + i)) {
                                    textLen |= (1 << i);
                                }
                            }
                            bitPos += 4;
                            
                            // 读取内容
                            QByteArray textBytes;
                            for (int i = 0; i < textLen && bitPos + 8 <= bits.size(); ++i) {
                                char c = 0;
                                for (int j = 0; j < 8; ++j) {
                                    if (bits.testBit(bitPos + j)) {
                                        c |= (1 << j);
                                    }
                                }
                                textBytes.append(c);
                                bitPos += 8;
                            }
                            csvContent += QString::fromUtf8(textBytes);
                        }
                    }
                }
                
                col++;
                if (col >= cols) {
                    csvContent += '\n';
                    col = 0;
                    row++;
                } else {
                    csvContent += ',';
                }
            }
            
            // 保存到临时文件并加载
            QTemporaryFile tempFile;
            if (tempFile.open()) {
                tempFile.write(csvContent.toUtf8());
                tempFile.close();
                return loadCSVFile(tempFile.fileName());
            }
            return false;
        }
        
        // 处理压缩或未压缩的二进制格式
        QByteArray data;
        if (flag == 1) {
            // 已压缩数据
            data = qUncompress(content);
        } else {
            // 未压缩数据
            data = content;
        }
        
        // 解析二进制格式
        if (data.size() < 7) { // 版本号(1) + 行数(2) + 列数(2) + 非空单元格数量(2)
            return false;
        }
        
        int pos = 0;
        
        // 版本号
        char version = data.at(pos++);
        if (version != 1) {
            return false;
        }
        
        // 行数
        int rows = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
        
        // 列数
        int cols = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
        
        // 非空单元格数量
        int count = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
        
        tableWidget->clearContents();
        tableWidget->setRowCount(rows);
        tableWidget->setColumnCount(cols);
        
        // 设置列标题
        for (int j = 0; j < cols; ++j) {
            tableWidget->setHorizontalHeaderItem(j, new QTableWidgetItem(QChar('A' + j)));
        }
        
        // 解析非空单元格数据
        for (int k = 0; k < count; ++k) {
            if (pos + 6 > data.size()) { // 行号(2) + 列号(2) + 文本长度(2)
                return false;
            }
            
            // 行号
            int i = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
            
            // 列号
            int j = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
            
            // 文本长度
            int textLen = (unsigned char)data.at(pos++) | ((unsigned char)data.at(pos++) << 8);
            
            if (pos + textLen > data.size()) {
                return false;
            }
            
            // 文本内容
            QByteArray textBytes = data.mid(pos, textLen);
            QString text = QString::fromUtf8(textBytes);
            pos += textLen;
            
            if (i >= 0 && i < rows && j >= 0 && j < cols) {
                QTableWidgetItem *item = new QTableWidgetItem(text);
                tableWidget->setItem(i, j, item);
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void MainWindow::updateInfoText()
{
    infoTextEdit->clear();
    infoTextEdit->append("CSV Compressor");
    infoTextEdit->append("功能:");
    infoTextEdit->append("1. 导入CSV文件");
    infoTextEdit->append("2. 显示表格内容");
    infoTextEdit->append("3. 压缩CSV文件（自动保存到build目录）");
    infoTextEdit->append("4. 自动解压并与原始文件比对");
    infoTextEdit->append("5. 在'还原内容'标签页查看还原结果");
    infoTextEdit->append("");
    infoTextEdit->append("请点击'导入文件'按钮开始操作");
}

bool MainWindow::compareFiles(const QString &file1, const QString &file2)
{
    QFile f1(file1);
    QFile f2(file2);
    
    if (!f1.open(QIODevice::ReadOnly)) {
        return false;
    }
    if (!f2.open(QIODevice::ReadOnly)) {
        f1.close();
        return false;
    }
    
    QByteArray data1 = f1.readAll();
    QByteArray data2 = f2.readAll();
    
    f1.close();
    f2.close();
    
    return data1 == data2;
}