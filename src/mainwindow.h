#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QSortFilterProxyModel>
#include <QVector>
#include "wallpaperdata.h"
#include "directoryscanner.h"
#include "filterpanel.h"
#include "wallpapercarddelegate.h"

class WallpaperModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit WallpaperModel(QObject *parent = nullptr);
    void setItems(const QVector<WallpaperData> &items);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QVector<WallpaperData> m_items;
};

class WallpaperFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit WallpaperFilterModel(QObject *parent = nullptr);

    void setSearchText(const QString &text);
    void setRating(const QString &rating); // "All", "Everyone", "Mature"
    void setSelectedTags(const QStringList &tags);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_searchText;
    QString m_rating = "All";
    QStringList m_selectedTags;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onScanFinished();
    void onFiltersChanged();
    void toggleDrawer();
    void onRefresh();
    void onItemDoubleClicked(const QModelIndex &index);
    void onContextMenu(const QPoint &pos);
    void updateVisibleGifs();

private:
    void loadData();
    void setupUI();

    // left drawer
    QWidget        *m_drawerContainer = nullptr;
    FilterPanel    *m_filterPanel     = nullptr;
    QPushButton    *m_toggleBtn       = nullptr;
    QPropertyAnimation *m_drawerAnim = nullptr;
    bool            m_drawerOpen      = true;

    // right grid
    QListView       *m_gridView       = nullptr;
    WallpaperCardDelegate *m_delegate = nullptr;

    // data
    DirectoryScanner    *m_scanner    = nullptr;
    WallpaperModel      *m_model      = nullptr;
    WallpaperFilterModel *m_filter    = nullptr;
    GifDispatcher       *m_gifDisp    = nullptr;
};

#endif // MAINWINDOW_H
