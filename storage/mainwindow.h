#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QStatusBar>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QBitArray>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportButtonClicked();
    void onCompressButtonClicked();

private:
    Ui::MainWindow *ui;
    QTableWidget *tableWidget;
    QTableWidget *restoredTableWidget;
    QTextEdit *infoTextEdit;
    QString currentFilePath;
    QByteArray compressedData;

    bool loadCSVFile(const QString &filePath, QTableWidget *table = nullptr);
    bool compressData();
    bool decompressData();
    void updateInfoText();
    bool compareFiles(const QString &file1, const QString &file2);
};

#endif // MAINWINDOW_H