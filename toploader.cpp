#include "toploader.h"

QMap<Shard, QString> ShardId {
    {THREAD_OF_FATE, "Нить судьбы"},
    {YOUNG_GUARD, "Молодая Гвардия"},
    {GODS_LEGACY, "Наследие Богов"},
    {ETERNAL_CALL, "Вечный Зов"},
    {STAR_OF_FORTUNE, "Звезда Удачи"}
};
Class ClassId[] = {BARD, WARRIOR, MAGE, HEALER, ENGINEER, PSIONICIST, SUMMONER, SCOUT, PALADIN, WARDEN, WARLOCK};

Toploader::Toploader(QString threadname, QString folder, bool *enabled, QMap<QString, QMap<QString, Player>> *GS100)
    : threadname(threadname)
    , folder(folder)
    , enabled(enabled)
    , GS100(GS100)
    , apiLink("https://allods.ru/api/rating/gearscore/")
{}
Toploader::~Toploader() {}

void Toploader::run() {
    QString result = "";

    (*GS100).clear();
    for (auto it = ShardId.begin(); it != ShardId.end(); it++) {
        (*GS100)[it.value()] = {};
        fetchDataWithRetry(it.key(), &result);
    }

    if (!saveResultToFile(result)) {
        qWarning() << "Failed to save data to file";
    }
}

bool Toploader::saveResultToFile(const QString& result) {
    QFile GS100File(folder + "\\GS100.txt");

    while (true) {
        if (!(*enabled)) {
            return false;
        }

        if (GS100File.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream writeStream(&GS100File);
            writeStream.setCodec("Windows-1251");
            writeStream << result;
            writeStream.flush();
            GS100File.close();
            return true;
        }

        this->thread()->sleep(1);
    }
}

void Toploader::fetchDataWithRetry(int shard, QString *result) {
    while (true) {
        if (!(*enabled)) {
            return;
        }

        if (fetchData(shard, result)) {
            return;
        }

        this->thread()->sleep(1);
    }
}

bool Toploader::fetchData(int shard, QString *result) {
    QList<QUrl> urls;
    bool success = true;

    for (Class classId : ClassId) {
        urls.append(apiLink + QString::number(shard) + "/" + QString::number(classId));
    }

    QNetworkAccessManager manager;
    QList<QNetworkReply*> replies;
    QEventLoop loop;
    QTimer timeoutTimer;

    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeoutTimer.start(30000);

    int total = urls.length();
    foreach (const QUrl &url, urls) {
        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        QNetworkReply *reply = manager.get(request);

        QObject::connect(reply, &QNetworkReply::finished, this, [&total, &loop, &success, reply]() {
            if (reply->error() != QNetworkReply::NoError) {
                success = false;
            }
            total--;
            if (total < 1) {
                loop.quit();
            }
        });
        replies << reply;
    }

    loop.exec();

    if (!timeoutTimer.isActive()) {
        qDeleteAll(replies.begin(), replies.end());
        replies.clear();
        return false;
    }
    timeoutTimer.stop();

    if (!success) {
        qDeleteAll(replies.begin(), replies.end());
        replies.clear();
        return false;
    }

    QString shardName = ShardId.value(static_cast<Shard>(shard));
    *result += "GS100[\"" + shardName + "\"] = {}\n";

    foreach (QNetworkReply *reply, replies) {
        if (reply->error() != QNetworkReply::NoError) {
            continue;
        }

        QByteArray r = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            success = false;
            continue;
        }

        if (!jsonDocument.isArray()) {
            success = false;
            continue;
        }

        QJsonArray dataArray = jsonDocument.array();

        foreach (QJsonValue val, dataArray) {
            if (!val.isObject()) {
                continue;
            }

            QJsonObject data = val.toObject();
            QString name = data.value("name").toString();
            QString gearscore = data.value("gearscore").toString();
            QString guild = data.value("guild").toString();

            if (name.isEmpty() || gearscore.isEmpty()) {
                continue;
            }

            QString cleanGearscore = gearscore.mid(0, gearscore.indexOf('.'));

            (*GS100)[shardName][name.toLower()] = {cleanGearscore, guild, name};

            *result += "GS100[\"" + shardName +
                       "\"][\"" + name.toLower() + "\"] = {\"" +
                       name + "\", \"" +
                       guild + "\", " +
                       cleanGearscore + "}\n";
        }
    }

    qDeleteAll(replies.begin(), replies.end());
    replies.clear();

    return success;
}
