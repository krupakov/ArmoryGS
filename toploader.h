#ifndef TOPLOADER_H
#define TOPLOADER_H

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
#include "variables.h"

class Toploader : public QThread
{

public:
    Toploader(QString threadname, QString folder, bool *enabled, QMap<QString, QMap<QString, Player>> *GS100);
    ~Toploader();

    void run();

private:
    QString threadname;
    QString folder;
    bool *enabled;
    QMap<QString, QMap<QString, Player>> *GS100;
    QString apiLink;

    bool saveResultToFile(const QString& result);
    bool fetchData(int shard, QString *result);
    void fetchDataWithRetry(int shard, QString *result);
};

#endif // TOPLOADER_H
