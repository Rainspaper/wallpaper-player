#ifndef WALLPAPERCARDDELEGATE_H
#define WALLPAPERCARDDELEGATE_H

#include <QStyledItemDelegate>
#include <QHash>
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QWidget>
#include <QSet>
#include <QRect>
#include <QVector>

// Drives GIF frame advancement using a single shared timer, decoupled
// from Windows system timer resolution (which media players change to
// 1ms, making QMovie-based approaches consume more CPU).
class GifDispatcher : public QObject {
    Q_OBJECT
public:
    explicit GifDispatcher(QObject *parent = nullptr);
    void setViewport(QWidget *vp) { m_viewport = vp; }
    QPixmap frameFor(const QString &path);
    void setVisiblePaths(const QSet<QString> &paths);
    void setItemRect(const QString &path, const QRect &rect) { m_itemRects[path] = rect; }

private:
    struct GifData {
        QVector<QImage> frames;
        QVector<int>    delays;
        int             current = 0;
        int             accumulator = 0;
    };

    GifData *loadGif(const QString &path);

    QHash<QString, GifData> m_gifCache;
    QSet<QString>           m_invalidGifs;
    QHash<QString, QRect>   m_itemRects;
    QSet<QString>           m_dirtyItems;
    QWidget                *m_viewport = nullptr;
    QTimer                 *m_tickTimer;
};

class WallpaperCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit WallpaperCardDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void setGifDispatcher(GifDispatcher *d) { m_gifDispatcher = d; }

    static constexpr int CardWidth  = 220;
    static constexpr int CardHeight = 200;
    static constexpr int ThumbH     = 130;

private:
    void drawCardBackground(QPainter *p, const QRect &rect, bool hovered) const;
    void drawThumbnail(QPainter *p, const QRect &rect, const QString &path) const;
    void drawTitle(QPainter *p, const QRect &rect, const QString &title) const;
    void drawTags(QPainter *p, const QRect &rect, const QStringList &tags) const;
    void drawRatingBadge(QPainter *p, const QRect &rect, const QString &rating) const;

    mutable QHash<QString, QPixmap> m_pixCache;
    GifDispatcher *m_gifDispatcher = nullptr;
};

#endif // WALLPAPERCARDDELEGATE_H
