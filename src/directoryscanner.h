#ifndef DIRCTORYSCANNER_H
#define DIRCTORYSCANNER_H

#include <QObject>
#include <QVector>
#include "wallpaperdata.h"

class DirectoryScanner : public QObject {
    Q_OBJECT
public:
    explicit DirectoryScanner(QObject *parent = nullptr);

    void setScanPaths(const QStringList &paths);
    void scan(); // synchronous; call from a worker thread or at startup

    const QVector<WallpaperData> &items() const { return m_items; }
    QStringList allTags() const;

signals:
    void scanFinished();
    void scanProgress(int done, int total);
    void scanError(const QString &message);

private:
    WallpaperData parseJson(const QString &dirPath);
    QStringList m_scanPaths;
    QVector<WallpaperData> m_items;
};

#endif // DIRCTORYSCANNER_H
