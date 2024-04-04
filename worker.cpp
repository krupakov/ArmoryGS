#include "worker.h"

QMap<QString, int> Shards {
    {"Нить судьбы", 1},
    {"Молодая Гвардия", 2},
    {"Наследие Богов", 3},
    {"Вечный Зов", 4},
    {"Звезда Удачи", 5}
};

QMap<Shard, QString> ShardIds {
    {THREAD_OF_FATE, "Нить судьбы"},
    {YOUNG_GUARD, "Молодая Гвардия"},
    {GODS_LEGACY, "Наследие Богов"},
    {ETERNAL_CALL, "Вечный Зов"},
    {STAR_OF_FORTUNE, "Звезда Удачи"}
};
Class ClassIds[] = {BARD, WARRIOR, MAGE, HEALER, ENGINEER, PSIONICIST, SUMMONER, SCOUT, PALADIN, WARDEN, WARLOCK};

Worker::Worker(QString threadname, QString folder, bool *enabled, QMap<QString, QMap<QString, Player>> *ArmoryGS)
    : threadname(threadname)
    , folder(folder)
    , enabled(enabled)
    , ArmoryGS(ArmoryGS)
    , url(QUrl(QStringLiteral("https://api.allodswiki.ru/api/v1/armory/avatars")))
{}
Worker::~Worker() {}

void Worker::run() {
    QFile ArmoryFile(folder + "\\ArmoryGS.txt");
    QFile cfg(QString(folder).replace("Addons\\ArmoryGS", "").replace("Addons/ArmoryGS", "") + "Configs\\ArmoryGS\\user.cfg");

    (*ArmoryGS).clear();
    for (auto it = Shards.begin(); it != Shards.end(); it++) {
        (*ArmoryGS)[it.key()] = {};
    }

    while(!ArmoryFile.open(QIODevice::ExistingOnly | QIODevice::WriteOnly | QIODevice::Text)) {
        if (!(*enabled)) {
            return;
        }
        this->thread()->sleep(1);
    }

    QTextStream out(&ArmoryFile);
    out.setCodec("Windows-1251");
    for (auto it = Shards.begin(); it != Shards.end(); it++) {
        out << "ArmoryGS[\"" + it.key() + "\"] = {}\n";
    }
    out.flush();
    ArmoryFile.close();

    while(*enabled) {
        while(!cfg.open(QIODevice::ExistingOnly | QIODevice::ReadOnly | QIODevice::Text)) {
            if (!(*enabled)) {
                return;
            }
            this->thread()->sleep(1);
        }

        QTextStream in(&cfg);
        QStringList players;
        QStringList lines = in.readAll().split('\n');
        int offset = 0;
        for (int i = 0; i < lines.size(); i++) {
            if ((i + offset) < lines.size() && lines[i + offset].trimmed() == "t_b ScriptUserMods_ArmoryGS_buffer") {
                offset++;
                if ((i + offset) < lines.size() && lines[i + offset].trimmed() == "t_b data") {
                    offset++;
                    if ((i + offset) >= lines.size()) break;
                    QString line = lines[i + offset];
                    while (line.trimmed() != "t_e data") {
                        if ((i + offset + 4) >= lines.size()) break;
                        QString name = line.trimmed().remove(0, 4).remove(QRegExp("[^(а-яёa-z)]"));
                        offset++;
                        line = lines[i + offset];
                        QString shard = line.trimmed().remove(0, 7).replace("\"", "");
                        offset++;
                        offset++;
                        offset++;
                        line = lines[i + offset];

                        if (name == "") continue;
                        if ((*ArmoryGS)[shard].contains(name)) continue;
                        players << name;
                        players << shard;
                    }
                    break;
                }
            }
        }
        fetchData(&players);
        in.flush();
        cfg.close();

        this->thread()->sleep(1);
    }
}

void Worker::fetchData(QStringList *players)
{
    QFile ArmoryFile(folder + "\\ArmoryGS.txt");

    while(!ArmoryFile.open(QIODevice::ExistingOnly | QIODevice::Append | QIODevice::Text)) {
        if (!(*enabled)) {
            return;
        }
        this->thread()->sleep(1);
    }

    QTextStream out(&ArmoryFile);

    for (int i = 0; i < players->size(); i += 2) {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject filter;
        filter["name"] = players->at(i);
        filter["server"] = QString::number(Shards.value(players->at(i + 1)));
        QJsonObject postData;
        postData["filter"] = filter;

        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.post(request, QJsonDocument(postData).toJson(QJsonDocument::Compact));

        QTimer timer;
        timer.setSingleShot(true);

        QEventLoop loop;
        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        timer.start(10000);
        loop.exec();

        if (timer.isActive()) {
            timer.stop();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray r(reply->readAll());
                QJsonObject jsonDocument = QJsonDocument::fromJson(r).object();
                QJsonArray dataArray = jsonDocument["data"].toArray();
                QJsonObject data;

                for (int j = 0; j < dataArray.size(); j++) {
                    data = dataArray[j].toObject();
                    if (data.value("name").toString().toLower() == players->at(i)) {
                        (*ArmoryGS)[players->at(i + 1)][data.value("name").toString().toLower()] = {QString::number(round(data.value("gear_score").toDouble(0))), data.value("guild").toString(), data.value("name").toString()};

                        qDebug() << ((*ArmoryGS)[players->at(i + 1)][players->at(i)].exactname + " | " + (*ArmoryGS)[players->at(i + 1)][players->at(i)].guild + " | " + (*ArmoryGS)[players->at(i + 1)][players->at(i)].gearscore + " | " + players->at(i + 1));

                        out << "ArmoryGS[\"" + players->at(i + 1) +
                                   "\"][\"" + data.value("name").toString().toLower() + "\"] = {\"" +
                                   data.value("name").toString() + "\", \"" +
                                   data.value("guild").toString() + "\", " +
                                   QString::number(round(data.value("gear_score").toDouble(0))) + "}\n";
                    }
                }
            }
        } else {
            disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            reply->abort();
        }

        if (!(*ArmoryGS)[players->at(i + 1)].contains(players->at(i))) {
            (*ArmoryGS)[players->at(i + 1)][players->at(i)] = {"0", "", ""};
            out << "ArmoryGS[\"" + players->at(i + 1) + "\"][\"" + players->at(i) + "\"] = {\"" + players->at(i) + "\", \"\", -1}\n";
        }

        reply->deleteLater();
    }

    out.flush();
    ArmoryFile.close();
}
