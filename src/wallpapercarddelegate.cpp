#include "wallpapercarddelegate.h"
#include "wallpaperdata.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QApplication>

// ─── GifDispatcher ──────────────────────────────────────────────

GifDispatcher::GifDispatcher(QObject *parent)
    : QObject(parent)
{
    // Single fixed-rate tick at 50ms (20 fps).
    // This is NOT a Qtimer-based animation — we manually advance GIF
    // frames according to each GIF's own frame-delay metadata, so the
    // system timer resolution has NO effect on our CPU usage.
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(50);
    connect(m_tickTimer, &QTimer::timeout, this, [this]() {
        if (m_gifCache.isEmpty()) return;

        // advance each visible GIF by one tick (frames track in memory)
        for (auto it = m_gifCache.begin(); it != m_gifCache.end(); ++it) {
            GifData &gd = it.value();
            if (gd.frames.size() <= 1) continue;

            gd.accumulator += 50; // one tick
            int delay = gd.delays[gd.current];
            if (gd.accumulator >= delay) {
                gd.accumulator -= delay;
                gd.current = (gd.current + 1) % gd.frames.size();
                m_dirtyItems.insert(it.key());
            }
        }

        bool foreground = (QApplication::activeWindow() != nullptr);
        bool hasDirty = !m_dirtyItems.isEmpty();

        // flush dirty rects only when window is foreground
        if (hasDirty && m_viewport && foreground) {
            QRegion region;
            for (const auto &path : m_dirtyItems) {
                auto rit = m_itemRects.find(path);
                if (rit != m_itemRects.end())
                    region = region.united(QRegion(rit.value()));
            }
            m_dirtyItems.clear();
            if (!region.isEmpty())
                m_viewport->update(region);
        } else if (hasDirty && !foreground) {
            m_dirtyItems.clear();
        }
    });
}

QPixmap GifDispatcher::frameFor(const QString &path)
{
    if (m_invalidGifs.contains(path))
        return {};

    // cache hit?
    auto it = m_gifCache.find(path);
    if (it != m_gifCache.end()) {
        const GifData &gd = it.value();
        if (gd.frames.isEmpty())
            return {};
        return QPixmap::fromImage(gd.frames[gd.current]);
    }

    // load it
    GifData *gd = loadGif(path);
    if (!gd) {
        m_invalidGifs.insert(path);
        return {};
    }

    // first GIF loaded → start the tick timer
    if (!m_tickTimer->isActive())
        m_tickTimer->start();

    return gd->frames.isEmpty() ? QPixmap() : QPixmap::fromImage(gd->frames[0]);
}

GifDispatcher::GifData *GifDispatcher::loadGif(const QString &path)
{
    QImageReader reader(path, "gif");
    if (!reader.canRead()) return nullptr;

    int count = reader.imageCount();
    if (count <= 0) return nullptr;

    GifData gd;
    gd.frames.reserve(count);
    gd.delays.reserve(count);

    for (int i = 0; i < count; ++i) {
        reader.jumpToImage(i);
        QImage frame = reader.read();
        if (frame.isNull()) continue;
        gd.frames.append(std::move(frame));

        // GIF delay is stored in hundredths of a second; default to 100ms
        int delay = reader.nextImageDelay();
        gd.delays.append(delay > 0 ? delay : 100);
    }

    if (gd.frames.isEmpty()) return nullptr;

    // single-frame GIFs don't need the timer
    auto it = m_gifCache.insert(path, std::move(gd));
    return &it.value();
}

void GifDispatcher::setVisiblePaths(const QSet<QString> &paths)
{
    Q_UNUSED(paths);
    // Don't prune on scroll — keeping <100 loaded GIFs costs negligible
    // memory and avoids the jank from destroying / reloading on scroll-back.
    // stop timer if nothing left to animate
    if (m_gifCache.isEmpty())
        m_tickTimer->stop();
}

// ─── WallpaperCardDelegate ──────────────────────────────────────

WallpaperCardDelegate::WallpaperCardDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize WallpaperCardDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(CardWidth, CardHeight);
}

void WallpaperCardDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool hovered = option.state & QStyle::State_MouseOver;
    QRect r = option.rect;
    int pad = 6;

    QRect bgRect = r.adjusted(pad/2, pad/2, -pad/2, -pad/2);
    drawCardBackground(painter, bgRect, hovered);

    QRect thumbRect(bgRect.x() + pad, bgRect.y() + pad,
                    bgRect.width() - pad*2, ThumbH - pad);
    QString previewPath = index.data(Qt::UserRole + 1).toString();
    drawThumbnail(painter, thumbRect, previewPath);

    // register position for targeted GIF repaints
    if (m_gifDispatcher && previewPath.endsWith(".gif", Qt::CaseInsensitive))
        m_gifDispatcher->setItemRect(previewPath, r);

    QString rating = index.data(Qt::UserRole + 3).toString();
    drawRatingBadge(painter, thumbRect, rating);

    QRect titleRect(bgRect.x() + pad, thumbRect.bottom() + 4,
                    bgRect.width() - pad*2, 22);
    QString title = index.data(Qt::DisplayRole).toString();
    drawTitle(painter, titleRect, title);

    QStringList tags = index.data(Qt::UserRole + 2).toStringList();
    QRect tagsRect(bgRect.x() + pad, titleRect.bottom() + 2,
                   bgRect.width() - pad*2, 20);
    drawTags(painter, tagsRect, tags);

    painter->restore();
}

void WallpaperCardDelegate::drawCardBackground(QPainter *p, const QRect &rect, bool hovered) const
{
    p->setPen(Qt::NoPen);
    p->setBrush(hovered ? QColor(55, 55, 60) : QColor(38, 38, 42));
    p->drawRoundedRect(rect, 8, 8);
}

void WallpaperCardDelegate::drawThumbnail(QPainter *p, const QRect &rect, const QString &path) const
{
    if (path.isEmpty()) {
        p->setBrush(QColor(50, 50, 55));
        p->drawRoundedRect(rect, 4, 4);
        return;
    }

    QPixmap px;

    if (path.endsWith(".gif", Qt::CaseInsensitive) && m_gifDispatcher) {
        px = m_gifDispatcher->frameFor(path);
    }

    if (px.isNull()) {
        auto it = m_pixCache.find(path);
        if (it != m_pixCache.end()) {
            px = it.value();
        } else {
            QImage img(path);
            if (!img.isNull()) {
                px = QPixmap::fromImage(img);
                m_pixCache[path] = px;
            }
        }
    }

    if (px.isNull()) {
        p->setBrush(QColor(50, 50, 55));
        p->drawRoundedRect(rect, 4, 4);
        return;
    }

    QPixmap scaled = px.scaled(rect.size(), Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);
    int x = rect.x() + (rect.width() - scaled.width()) / 2;
    int y = rect.y() + (rect.height() - scaled.height()) / 2;

    p->save();
    p->setClipRect(rect);
    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(rect), 4, 4);
    p->setClipPath(clipPath);
    p->drawPixmap(x, y, scaled);
    p->restore();
}

void WallpaperCardDelegate::drawTitle(QPainter *p, const QRect &rect, const QString &title) const
{
    p->setPen(QColor(220, 220, 222));
    QFont f = p->font();
    f.setPointSize(9);
    p->setFont(f);
    p->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::ElideRight, title);
}

void WallpaperCardDelegate::drawTags(QPainter *p, const QRect &rect, const QStringList &tags) const
{
    if (tags.isEmpty()) return;

    int x = rect.x();
    int y = rect.y();
    int h = 18;

    QFont f = p->font();
    f.setPointSize(7);
    p->setFont(f);

    for (const auto &tag : tags) {
        QString t = tag.trimmed();
        if (t.isEmpty()) continue;

        QFontMetrics fm(f);
        int tw = fm.horizontalAdvance(t) + 10;
        if (x + tw > rect.right()) break;

        p->setPen(Qt::NoPen);
        p->setBrush(QColor(70, 70, 80));
        p->drawRoundedRect(x, y, tw, h, 4, 4);

        p->setPen(QColor(180, 180, 190));
        p->drawText(x + 5, y, tw - 10, h, Qt::AlignVCenter | Qt::AlignLeft, t);

        x += tw + 4;
    }
}

void WallpaperCardDelegate::drawRatingBadge(QPainter *p, const QRect &rect, const QString &rating) const
{
    if (rating.isEmpty()) return;

    QColor bg = (rating == "Mature") ? QColor(200, 70, 70, 200) : QColor(70, 160, 70, 200);
    QString label = (rating == "Mature") ? "M" : "E";

    QFont f = p->font();
    f.setPointSize(7);
    f.setBold(true);
    p->setFont(f);
    QFontMetrics fm(f);
    int w = fm.horizontalAdvance(label) + 10;
    int h = 18;
    int x = rect.right() - w - 4;
    int y = rect.y() + 4;

    p->setPen(Qt::NoPen);
    p->setBrush(bg);
    p->drawRoundedRect(x, y, w, h, 4, 4);

    p->setPen(Qt::white);
    p->drawText(x, y, w, h, Qt::AlignCenter, label);
}
