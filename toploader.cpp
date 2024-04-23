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
        fetchData(it.key(), &result);
    }

    QFile GS100File(folder + "\\GS100.txt");

    while(!GS100File.open(QIODevice::ExistingOnly | QIODevice::WriteOnly | QIODevice::Text)) {
        if (!(*enabled)) {
            return;
        }
        this->thread()->sleep(1);
    }

    QTextStream writeStream(&GS100File);
    writeStream.setCodec("Windows-1251");
    writeStream << result;
    writeStream.flush();

    GS100File.close();
}

void Toploader::fetchData(int shard, QString *result) {
    QList<QUrl> urls;

    for (Class classId : ClassId) {
        urls.append(apiLink + QString::number(shard) + "/" + QString::number(classId));
    }

    QNetworkAccessManager manager;
    QList<QNetworkReply*> replies;
    QEventLoop loop;

    int total = urls.length();
    foreach (const QUrl &url, urls) {
        QNetworkReply *reply = manager.get(QNetworkRequest(url));
        QObject::connect(reply, &QNetworkReply::finished, this, [&total, &loop]() {
            total--;
            if (total < 1) {
                loop.quit();
            }
        });
        replies << reply;
    }
    loop.exec();

    *result += "GS100[\"" + ShardId.value(static_cast<Shard>(shard)) + "\"] = {}\n";

    foreach (QNetworkReply *reply, replies) {
        QByteArray r(reply->readAll());
        QJsonDocument jsonDocument = QJsonDocument::fromJson(r);
        QJsonArray dataArray = jsonDocument.array();

        foreach (QJsonValue val, dataArray) {
            QJsonObject data = val.toObject();

            (*GS100)[ShardId.value(static_cast<Shard>(shard))][data.value("name").toString().toLower()] = {data.value("gearscore").toString().mid(0, data.value("gearscore").toString().indexOf('.')), data.value("guild").toString(), data.value("name").toString()};

            *result += "GS100[\"" + ShardId.value(static_cast<Shard>(shard)) +
                       "\"][\"" + data.value("name").toString().toLower() + "\"] = {\"" +
                       data.value("name").toString() + "\", \"" +
                       data.value("guild").toString() + "\", " +
                       data.value("gearscore").toString().mid(0, data.value("gearscore").toString().indexOf('.')) + "}\n";
        }
    }

    qDeleteAll(replies.begin(), replies.end());
    replies.clear();
}
