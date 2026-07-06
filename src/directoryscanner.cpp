#include "directoryscanner.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

DirectoryScanner::DirectoryScanner(QObject *parent)
    : QObject(parent)
{
}

void DirectoryScanner::setScanPath(const QString &path)
{
    m_scanPath = path;
}

void DirectoryScanner::scan()
{
    m_items.clear();

    QDir baseDir(m_scanPath);
    if (!baseDir.exists()) {
        emit scanError("目录不存在: " + m_scanPath);
        return;
    }

    const auto entries = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto &info : entries) {
        WallpaperData item = parseJson(info.absoluteFilePath());
        if (!item.id.isEmpty() && item.videoFile.endsWith(".mp4", Qt::CaseInsensitive)) {
            m_items.append(item);
        }
    }

    emit scanFinished();
}

QStringList DirectoryScanner::allTags() const
{
    QStringList tags;
    for (const auto &item : m_items) {
        for (const auto &tag : item.tags) {
            if (!tags.contains(tag))
                tags.append(tag);
        }
    }
    tags.sort(Qt::CaseInsensitive);
    return tags;
}

WallpaperData DirectoryScanner::parseJson(const QString &dirPath)
{
    WallpaperData data;
    QFile file(dirPath + "/project.json");
    if (!file.open(QIODevice::ReadOnly))
        return data;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return data;

    QJsonObject obj = doc.object();
    QDir dir(dirPath);
    data.id         = dir.dirName();
    data.title      = obj["title"].toString();
    data.description= obj["description"].toString();
    data.contentRating = obj["contentrating"].toString();
    data.videoFile  = obj["file"].toString();
    data.previewFile= obj["preview"].toString();
    data.dirPath    = dirPath;

    const QJsonArray tagArr = obj["tags"].toArray();
    for (const auto &t : tagArr)
        data.tags.append(t.toString());

    return data;
}
