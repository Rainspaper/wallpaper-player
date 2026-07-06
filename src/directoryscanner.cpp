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

void DirectoryScanner::setScanPaths(const QStringList &paths)
{
    m_scanPaths = paths;
}

void DirectoryScanner::scan()
{
    m_items.clear();

    if (m_scanPaths.isEmpty()) {
        emit scanFinished();
        return;
    }

    int done = 0, total = m_scanPaths.size();
    for (const auto &path : m_scanPaths) {
        ++done;

        QDir baseDir(path);
        if (!baseDir.exists()) {
            qWarning() << "Dir not found:" << path;
            emit scanError(QString::fromUtf8("目录不存在: ") + path);
            emit scanProgress(done, total);
            continue;
        }

        const auto entries = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &info : entries) {
            WallpaperData item = parseJson(info.absoluteFilePath());
            if (!item.id.isEmpty() && item.videoFile.endsWith(".mp4", Qt::CaseInsensitive)) {
                m_items.append(item);
            }
        }

        emit scanProgress(done, total);
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
