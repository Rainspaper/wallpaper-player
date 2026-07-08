#ifndef WALLPAPERDATA_H
#define WALLPAPERDATA_H

#include <QString>
#include <QStringList>
#include <QDateTime>

struct WallpaperData {
    QString id;             // numeric folder name
    QString title;
    QString description;
    QStringList tags;
    QString contentRating;  // "Everyone" or "Mature"
    QString videoFile;
    QString previewFile;
    QString dirPath;        // full path to the item folder
    QDateTime downloadTime; // when project.json was last modified

    QString videoPath()  const { return dirPath + "/" + videoFile; }
    QString previewPath() const { return dirPath + "/" + previewFile; }
};

#endif // WALLPAPERDATA_H
