#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , enabled(true)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    trayIcon = new QSystemTrayIcon(QIcon(":/icon.ico"), this);
    trayIcon->setToolTip("ArmoryGS Updater");
    QMenu *menu = new QMenu(this);
    QAction *viewWindow = new QAction(QString("Развернуть").toUtf8(), this);
    QAction *quitAction = new QAction(QString("Выход").toUtf8(), this);

    connect(viewWindow, SIGNAL(triggered()), this, SLOT(show()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(closeForever()));

    menu->addAction(viewWindow);
    menu->addAction(quitAction);

    trayIcon->setContextMenu(menu);
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    QSettings startup("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    if (startup.contains("ArmoryGS Updater")) {
        QString path = QCoreApplication::applicationFilePath();
        path.replace('/', '\\');
        if (startup.value("ArmoryGS Updater") != ("\"" + path + "\" --hide")) {
            startup.setValue("ArmoryGS Updater", "\"" + path + "\" --hide");
        }
        ui->autoStartupCheckbox->setChecked(true);
    }

    GS100 = {
        {"Нить судьбы", {}},
        {"Молодая Гвардия", {}},
        {"Наследие Богов", {}},
        {"Вечный Зов", {}},
        {"Звезда Удачи", {}}
    };

    ArmoryGS = {
        {"Нить судьбы", {}},
        {"Молодая Гвардия", {}},
        {"Наследие Богов", {}},
        {"Вечный Зов", {}},
        {"Звезда Удачи", {}}
    };

    btnPalette = ui->startButton->palette();

    btnPalette.setColor(QPalette::ButtonText, Qt::red);
    ui->startButton->setPalette(btnPalette);
    ui->startButton->update();

    QSettings settings("ArmoryGS", "Updater");
    if (settings.contains("browseLineEdit")) {
        ui->browseLineEdit->setText(settings.value("browseLineEdit").toString());
    } else {
        QSettings m("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", QSettings::NativeFormat);
        QString path = m.value("Аллоды Онлайн/InstallLocation").toString();
        if (path.isEmpty()) {
            path = m.value("gcgame_0.359/InstallLocation").toString();
        }
        ui->browseLineEdit->setText(path.isEmpty() ? "" : path + "data\\Mods\\Addons\\ArmoryGS");
    }

    toploader = new Toploader("GS100 Top Loader", ui->browseLineEdit->text(), &enabled, &GS100);
    toploader->start();

    worker = new Worker("ArmoryGS Update Cycle", ui->browseLineEdit->text(), &enabled, &ArmoryGS);
    worker->start();

    QStringList arguments = qApp->arguments();
    if (arguments.size() > 1) {
        if (arguments.at(1) == "--hide") {
            QTimer timer;
            timer.singleShot(0, this, SLOT(hide()));
        }
    }
}

MainWindow::~MainWindow()
{
    enabled = false;
    toploader->terminate();
    if (toploader->isRunning()) toploader->wait();
    toploader->deleteLater();
    worker->terminate();
    if (worker->isRunning()) worker->wait();
    worker->deleteLater();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->isVisible()) {
        event->ignore();
        this->hide();
        trayIcon->showMessage("ArmoryGS Updater", QString("Свернуто в трей").toUtf8(), QIcon(":/icon.ico"), 2000);
    }
}

void MainWindow::closeForever()
{
    this->setVisible(false);
    this->close();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            if (!this->isVisible()) {
                this->show();
            } else {
                this->hide();
            }
            break;
        default:
            break;
    }
}

void MainWindow::on_browseButton_clicked()
{
    QString folderName = QFileDialog::getExistingDirectory(this, "Выбор папки", ui->browseLineEdit->text(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (folderName != "") {
        QSettings settings("ArmoryGS", "Updater");
        ui->browseLineEdit->setText(folderName);
        settings.setValue("browseLineEdit", folderName);
        btnPalette.setColor(QPalette::ButtonText, Qt::darkGreen);
        ui->startButton->setText("Включить обновление рейтингов");
        ui->startButton->setPalette(btnPalette);
        ui->startButton->update();
        enabled = false;
    }
}

void MainWindow::on_startButton_clicked()
{
    enabled = !enabled;

    if (enabled) {
        btnPalette.setColor(QPalette::ButtonText, Qt::red);
        ui->startButton->setText("Выключить обновление рейтингов");
        ui->startButton->setPalette(btnPalette);
        ui->startButton->update();

        toploader = new Toploader("GS100 Top Loader", ui->browseLineEdit->text(), &enabled, &GS100);
        toploader->start();

        worker = new Worker("ArmoryGS Update Cycle", ui->browseLineEdit->text(), &enabled, &ArmoryGS);
        worker->start();

        return;
    }

    btnPalette.setColor(QPalette::ButtonText, Qt::darkGreen);
    ui->startButton->setText("Включить обновление рейтингов");
    ui->startButton->setPalette(btnPalette);
    ui->startButton->update();

    toploader->terminate();
    if (toploader->isRunning()) toploader->wait();
    worker->terminate();
    if (worker->isRunning()) worker->wait();
}

void MainWindow::on_autoStartupCheckbox_stateChanged(int state)
{
    QSettings startup("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (ui->autoStartupCheckbox->isChecked()) {
        if (!startup.contains("ArmoryGS Updater")) {
            QString path = QCoreApplication::applicationFilePath();
            path.replace('/', '\\');
            startup.setValue("ArmoryGS Updater", "\"" + path + "\" --hide");
        }
    } else {
        startup.remove("ArmoryGS Updater");
    }
}

