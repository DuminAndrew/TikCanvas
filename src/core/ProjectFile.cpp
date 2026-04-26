#include "ProjectFile.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

bool ProjectFile::save(const QString &path, const QString &pdfPath,
                       const QVector<DeviceNode> &devices,
                       const QVector<DeviceLink> &links)
{
    QJsonObject root;
    root["version"] = 1;
    root["pdf"] = pdfPath;

    QJsonArray dev;
    for (const auto &d : devices) {
        QJsonObject o;
        o["id"]   = d.id;
        o["name"] = d.name;
        o["ip"]   = d.ip;
        o["x"]    = d.pos.x();
        o["y"]    = d.pos.y();
        dev.append(o);
    }
    root["devices"] = dev;

    QJsonArray lnk;
    for (const auto &l : links) {
        QJsonObject o;
        o["from"] = l.from;
        o["to"]   = l.to;
        lnk.append(o);
    }
    root["links"] = lnk;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ProjectFile::load(const QString &path, QString &pdfPath,
                       QVector<DeviceNode> &devices,
                       QVector<DeviceLink> &links)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    const QJsonObject root = doc.object();

    pdfPath = root.value("pdf").toString();
    devices.clear();
    for (const auto &v : root.value("devices").toArray()) {
        const QJsonObject o = v.toObject();
        DeviceNode d;
        d.id   = o.value("id").toString();
        d.name = o.value("name").toString();
        d.ip   = o.value("ip").toString();
        d.pos  = QPointF(o.value("x").toDouble(), o.value("y").toDouble());
        devices.push_back(d);
    }
    links.clear();
    for (const auto &v : root.value("links").toArray()) {
        const QJsonObject o = v.toObject();
        links.push_back({o.value("from").toInt(-1), o.value("to").toInt(-1)});
    }
    return true;
}
