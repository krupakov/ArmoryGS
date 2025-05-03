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

    if (!openFile(ArmoryFile, QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&ArmoryFile);
    out.setCodec("Windows-1251");
    for (auto it = Shards.begin(); it != Shards.end(); it++) {
        out << "ArmoryGS[\"" + it.key() + "\"] = {}\n";
    }
    out.flush();
    ArmoryFile.close();

    while(*enabled) {
        if (!openFile(cfg, QIODevice::ExistingOnly | QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&cfg);
        QStringList players = parseBuffer(in);
        in.flush();
        cfg.close();

        if (!players.isEmpty()) {
            fetchData(&players);
        }

        this->thread()->sleep(1);
    }
}

bool Worker::openFile(QFile& file, QIODevice::OpenMode mode) {
    while (true) {
        if (!(*enabled)) {
            return false;
        }

        if (file.open(mode)) {
            return true;
        }

        this->thread()->sleep(1);
    }
}

QStringList Worker::parseBuffer(QTextStream& in) {
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

                    if (name.isEmpty()) continue;
                    if ((*ArmoryGS)[shard].contains(name)) continue;
                    players << name;
                    players << shard;
                }
                break;
            }
        }
    }
    return players;
}

void Worker::fetchData(QStringList *players) {
    QFile ArmoryFile(folder + "\\ArmoryGS.txt");

    if (!openFile(ArmoryFile, QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream out(&ArmoryFile);
    out.setCodec("Windows-1251");

    for (int i = 0; i < players->size(); i += 2) {
        QString playerName = players->at(i);
        QString shardName = players->at(i + 1);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject filter;
        filter["name"] = playerName;
        filter["server"] = QString::number(Shards.value(shardName));
        QJsonObject postData;
        postData["filter"] = filter;

        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.post(request, QJsonDocument(postData).toJson(QJsonDocument::Compact));

        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        timeoutTimer.start(10000);
        loop.exec();

        bool requestSuccess = false;
        if (timeoutTimer.isActive()) {
            timeoutTimer.stop();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray responseData = reply->readAll();
                QJsonParseError parseError;
                QJsonDocument jsonDocument = QJsonDocument::fromJson(responseData, &parseError);

                if (parseError.error == QJsonParseError::NoError && jsonDocument.isObject()) {
                    QJsonObject jsonObject = jsonDocument.object();
                    if (jsonObject.contains("data") && jsonObject["data"].isArray()) {
                        QJsonArray dataArray = jsonObject["data"].toArray();

                        for (const QJsonValue& value : dataArray) {
                            if (value.isObject()) {
                                QJsonObject data = value.toObject();
                                QString name = data.value("name").toString();

                                if (name.compare(playerName, Qt::CaseInsensitive) == 0) {
                                    double gearScore = data.value("gear_score").toDouble(0);
                                    QString guild = data.value("guild").toString("");

                                    (*ArmoryGS)[shardName][name.toLower()] = {
                                        QString::number(round(gearScore)),
                                        guild,
                                        name
                                    };

                                    qDebug() << ((*ArmoryGS)[shardName][playerName].exactname + " | " +
                                                 (*ArmoryGS)[shardName][playerName].guild + " | " +
                                                 (*ArmoryGS)[shardName][playerName].gearscore + " | " +
                                                 shardName);

                                    out << "ArmoryGS[\"" + shardName +
                                               "\"][\"" + name.toLower() + "\"] = {\"" +
                                               name + "\", \"" +
                                               guild + "\", " +
                                               QString::number(round(gearScore)) + "}\n";
                                    requestSuccess = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            reply->abort();
        }

        if (!requestSuccess && !(*ArmoryGS)[shardName].contains(playerName)) {
            (*ArmoryGS)[shardName][playerName] = {"0", "", ""};
            out << "ArmoryGS[\"" + shardName + "\"][\"" + playerName + "\"] = {\"" + playerName + "\", \"\", -1}\n";
        }

        reply->deleteLater();
    }

    out.flush();
    ArmoryFile.close();
}
