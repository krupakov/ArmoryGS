#ifndef WORKER_H
#define WORKER_H

#include <QThread>
#include <QTime>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtMath>
#include "variables.h"

class Worker : public QThread
{

public:
    Worker(QString threadname, QString folder, bool *enabled, QMap<QString, QMap<QString, Player>> *ArmoryGS);
    ~Worker();

    void run();

    void fetchData(QStringList *players);

private:
    QString threadname;

    QString folder;

    bool *enabled;

    QMap<QString, QMap<QString, Player>> *ArmoryGS;

    QUrl url;
};

#endif // WORKER_H
