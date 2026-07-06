#ifndef DIRCTORYSCANNER_H
#define DIRCTORYSCANNER_H

#include <QObject>
#include <QVector>
#include "wallpaperdata.h"

class DirectoryScanner : public QObject {
    Q_OBJECT
public:
    explicit DirectoryScanner(QObject *parent = nullptr);

    void setScanPath(const QString &path);
    void scan(); // synchronous; call from a worker thread or at startup

    const QVector<WallpaperData> &items() const { return m_items; }
    QStringList allTags() const;

signals:
    void scanFinished();
    void scanError(const QString &message);

private:
    WallpaperData parseJson(const QString &dirPath);
    QString m_scanPath;
    QVector<WallpaperData> m_items;
};

#endif // DIRCTORYSCANNER_H
