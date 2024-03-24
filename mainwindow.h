#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QAction>
#include <QMenu>
#include <QStyle>
#include "variables.h"
#include "toploader.h"
#include "worker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool enabled;

    QMap<QString, QMap<QString, Player>> GS100;

    QMap<QString, QMap<QString, Player>> ArmoryGS;


protected:
    void closeEvent(QCloseEvent * event);

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

    void closeForever();

    void on_browseButton_clicked();

    void on_startButton_clicked();

    void on_autoStartupCheckbox_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;

    QSystemTrayIcon *trayIcon;

    QPalette btnPalette;

    Toploader *toploader;

    Worker *worker;
};
#endif // MAINWINDOW_H
