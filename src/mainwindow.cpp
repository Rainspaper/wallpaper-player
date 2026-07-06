#include "mainwindow.h"
#include <windows.h>
#include <shellapi.h>
#include <QPainter>
#include <QPolygonF>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDesktopServices>
#include <QMenu>
#include <QAction>
#include <QUrl>
#include <QLabel>
#include <QApplication>
#include <QIcon>
#include <QScreen>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QScrollBar>
#include <QTimer>

// ─── WallpaperModel ──────────────────────────────────────────────

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent) {}

void WallpaperModel::setItems(const QVector<WallpaperData> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

int WallpaperModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QVariant WallpaperModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto &item = m_items[index.row()];

    switch (role) {
    case Qt::DisplayRole:    return item.title;
    case Qt::UserRole + 1:   return item.previewPath();
    case Qt::UserRole + 2:   return item.tags;
    case Qt::UserRole + 3:   return item.contentRating;
    case Qt::UserRole + 4:   return item.videoPath();
    case Qt::UserRole + 5:   return item.dirPath;
    case Qt::ToolTipRole:    return item.title;
    default:                 return {};
    }
}

// ─── WallpaperFilterModel ────────────────────────────────────────

WallpaperFilterModel::WallpaperFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setRecursiveFilteringEnabled(false);
}

void WallpaperFilterModel::setSearchText(const QString &text)
{
    m_searchText = text;
    invalidateFilter();
}

void WallpaperFilterModel::setRating(const QString &rating)
{
    m_rating = rating;
    invalidateFilter();
}

void WallpaperFilterModel::setSelectedTags(const QStringList &tags)
{
    m_selectedTags = tags;
    invalidateFilter();
}

bool WallpaperFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto srcIdx = sourceModel()->index(sourceRow, 0, sourceParent);

    // title search
    if (!m_searchText.isEmpty()) {
        QString title = srcIdx.data(Qt::DisplayRole).toString();
        if (!title.contains(m_searchText, Qt::CaseInsensitive))
            return false;
    }

    // rating filter
    if (m_rating != "All") {
        QString rating = srcIdx.data(Qt::UserRole + 3).toString();
        if (rating != m_rating)
            return false;
    }

    // tag filter (AND logic)
    if (!m_selectedTags.isEmpty()) {
        QStringList itemTags = srcIdx.data(Qt::UserRole + 2).toStringList();
        for (const auto &tag : m_selectedTags) {
            if (!itemTags.contains(tag))
                return false;
        }
    }

    return true;
}

// ─── MainWindow ──────────────────────────────────────────────────

static const QString kScanPath =
    QStringLiteral("E:/Apps/PlayGame/steam/steamapps/workshop/content/431960");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Wallpaper Player");
    setWindowIcon(QIcon(":/ico1.ico"));
    setMinimumSize(900, 600);
    resize(1280, 800);

    // dark theme
    setStyleSheet("QMainWindow { background:#1e1e22; }");

    setupUI();
    loadData();
}

void MainWindow::setupUI()
{
    auto *central = new QWidget;
    auto *rootLayout  = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    setCentralWidget(central);

    // ── top bar (always visible) ─────────────────────────
    auto *topBar = new QWidget;
    topBar->setFixedHeight(38);
    topBar->setStyleSheet("background:#25252a;");
    auto *topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(6, 4, 8, 4);
    topL->setSpacing(4);

    auto btnStyle = QString(
        "QPushButton { background:#2a2a2e; border:none; border-radius:6px; "
        "padding:4px 10px; color:#aaa; font-size:13px; }"
        "QPushButton:hover { background:#3a3a3e; color:#fff; }");

    m_toggleBtn = new QPushButton("☰  隐藏");
    m_toggleBtn->setFixedHeight(28);
    m_toggleBtn->setStyleSheet(btnStyle);
    m_toggleBtn->setToolTip("切换筛选面板");
    connect(m_toggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDrawer);
    topL->addWidget(m_toggleBtn);

    auto *refreshBtn = new QPushButton("🔄 刷新");
    refreshBtn->setFixedHeight(28);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background:#2a2a2e; border:1px solid #3a3a3e; border-radius:6px;"
        "  padding:4px 12px; color:#aaa; font-size:13px;"
        "}"
        "QPushButton:hover {"
        "  background:#2e3a33; border-color:#3a6a4e; color:#8dcaa0;"
        "}"
        "QPushButton:pressed {"
        "  background:#1e2a23; border-color:#2a5a3e; color:#6aaa80;"
        "}");
    refreshBtn->setToolTip("重新扫描目录");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefresh);
    topL->addWidget(refreshBtn);

    topL->addStretch();

    auto *titleLabel = new QLabel("Wallpaper Player");
    titleLabel->setStyleSheet("color:#666; font-size:12px;");
    topL->addWidget(titleLabel);

    rootLayout->addWidget(topBar);

    // ── body: drawer + grid ──────────────────────────────
    auto *bodyLayout = new QHBoxLayout;
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // drawer container
    m_drawerContainer = new QWidget;
    m_drawerContainer->setMaximumWidth(220);
    m_drawerContainer->setMinimumWidth(0);
    auto *drawerLayout = new QVBoxLayout(m_drawerContainer);
    drawerLayout->setContentsMargins(0, 0, 0, 0);
    drawerLayout->setSpacing(0);
    m_filterPanel = new FilterPanel;
    drawerLayout->addWidget(m_filterPanel, 1);
    bodyLayout->addWidget(m_drawerContainer);

    // grid view
    m_gridView = new QListView;
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setIconSize(QSize(220, 200));
    m_gridView->setGridSize(QSize(232, 212));
    m_gridView->setWrapping(true);
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setSpacing(4);
    m_gridView->setFlow(QListView::LeftToRight);
    m_gridView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_gridView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_gridView->setMouseTracking(true);
    m_gridView->setStyleSheet(
        "QListView { background:#1e1e22; border:none; outline:none; }"
        "QListView::item { border:none; }"
        "QListView::item:selected { background:transparent; }");
    m_gridView->setFrameShape(QFrame::NoFrame);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_gifDisp = new GifDispatcher(this);
    m_gifDisp->setViewport(m_gridView->viewport());

    m_delegate = new WallpaperCardDelegate(this);
    m_delegate->setGifDispatcher(m_gifDisp);
    m_gridView->setItemDelegate(m_delegate);

    bodyLayout->addWidget(m_gridView, 1);
    rootLayout->addLayout(bodyLayout, 1);

    // double-click to play
    connect(m_gridView, &QListView::doubleClicked, this, &MainWindow::onItemDoubleClicked);
    // right-click context menu
    connect(m_gridView, &QListView::customContextMenuRequested, this, &MainWindow::onContextMenu);
    // stop off-screen GIFs on scroll — skipped: all loaded GIFs stay
    // cached (avoids jank from destroy/reload on scroll-back).
    // connect(m_gridView->verticalScrollBar(), &QScrollBar::valueChanged,
    //         this, &MainWindow::updateVisibleGifs);
}

void MainWindow::loadData()
{
    m_scanner = new DirectoryScanner(this);
    connect(m_scanner, &DirectoryScanner::scanFinished, this, &MainWindow::onScanFinished);

    m_model  = new WallpaperModel(this);
    m_filter = new WallpaperFilterModel(this);
    m_filter->setSourceModel(m_model);
    m_gridView->setModel(m_filter);

    connect(m_filterPanel, &FilterPanel::filtersChanged, this, &MainWindow::onFiltersChanged);

    // scan in background
    m_scanner->setScanPath(kScanPath);
    m_scanner->scan();
}

void MainWindow::onRefresh()
{
    m_scanner->scan();
}

void MainWindow::onScanFinished()
{
    m_model->setItems(m_scanner->items());
    m_filterPanel->setTags(m_scanner->allTags());
    onFiltersChanged();
}

void MainWindow::onFiltersChanged()
{
    m_filter->setSearchText(m_filterPanel->searchText());
    m_filter->setRating(m_filterPanel->selectedRating());
    m_filter->setSelectedTags(m_filterPanel->selectedTags());
    m_filterPanel->updateResultCount(m_filter->rowCount(), m_model->rowCount());
    // defer so the view finishes layout before we check visibility
    QTimer::singleShot(0, this, &MainWindow::updateVisibleGifs);
}

void MainWindow::updateVisibleGifs()
{
    if (!m_gifDisp) return;

    QRect visibleRect = m_gridView->viewport()->rect();
    QSet<QString> visible;
    bool anyValid = false;
    for (int i = 0; i < m_filter->rowCount(); ++i) {
        QModelIndex idx = m_filter->index(i, 0);
        QRect itemRect = m_gridView->visualRect(idx);
        if (itemRect.isEmpty()) continue;
        anyValid = true;
        if (visibleRect.intersects(itemRect)) {
            QString path = idx.data(Qt::UserRole + 1).toString();
            if (path.endsWith(".gif", Qt::CaseInsensitive))
                visible.insert(path);
        }
    }
    // only prune off-screen movies if we actually have valid layout
    if (anyValid)
        m_gifDisp->setVisiblePaths(visible);
}

void MainWindow::toggleDrawer()
{
    int targetWidth = m_drawerOpen ? 0 : 220;

    // Clean up previous animation (safe even if still running)
    delete m_drawerAnim;
    m_drawerAnim = nullptr;

    m_drawerAnim = new QPropertyAnimation(m_drawerContainer, "maximumWidth", this);
    m_drawerAnim->setDuration(180);
    m_drawerAnim->setStartValue(m_drawerContainer->width());
    m_drawerAnim->setEndValue(targetWidth);
    m_drawerAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_drawerAnim, &QPropertyAnimation::finished, this, [this]() {
        m_drawerAnim = nullptr;
    });
    m_drawerAnim->start(QAbstractAnimation::KeepWhenStopped);

    m_drawerOpen = !m_drawerOpen;
    m_toggleBtn->setText(m_drawerOpen ? "☰  隐藏" : "☰  展开");
}

void MainWindow::onItemDoubleClicked(const QModelIndex &index)
{
    QString videoPath = index.data(Qt::UserRole + 4).toString();
    if (!videoPath.isEmpty()) {
        std::wstring wpath = QDir::toNativeSeparators(videoPath).toStdWString();
        ShellExecuteW(0, L"open", wpath.c_str(), 0, 0, SW_SHOWNORMAL);
    }
}

void MainWindow::onContextMenu(const QPoint &pos)
{
    QModelIndex index = m_gridView->indexAt(pos);
    if (!index.isValid()) return;

    QString titleVal = index.data(Qt::DisplayRole).toString();
    QString dirPath  = index.data(Qt::UserRole + 5).toString();
    QString videoPath = index.data(Qt::UserRole + 4).toString();

    QMenu menu;
    menu.setStyleSheet(
        "QMenu {"
        "  background:#1e1e22;"
        "  border:1px solid #3a3a3e;"
        "  border-radius:8px;"
        "  padding:6px 4px;"
        "}"
        "QMenu::item {"
        "  color:#ccc;"
        "  padding:8px 28px 8px 12px;"
        "  border-radius:6px;"
        "  margin:1px 4px;"
        "  font-size:13px;"
        "}"
        "QMenu::item:selected {"
        "  background:#3a3a3e;"
        "  color:#fff;"
        "}"
        "QMenu::separator {"
        "  height:1px;"
        "  background:#2e2e32;"
        "  margin:4px 12px;"
        "}");

    // title as disabled header
    QAction *titleAct = menu.addAction(titleVal);
    titleAct->setEnabled(false);
    QFont titleFont = titleAct->font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    titleAct->setFont(titleFont);

    menu.addSeparator();

    if (!videoPath.isEmpty()) {
        QAction *playAct = menu.addAction("播放");
        connect(playAct, &QAction::triggered, [videoPath]() {
            std::wstring wpath = QDir::toNativeSeparators(videoPath).toStdWString();
            ShellExecuteW(0, L"open", wpath.c_str(), 0, 0, SW_SHOWNORMAL);
        });
    }

    QAction *openFolderAct = menu.addAction("打开所在文件夹");
    connect(openFolderAct, &QAction::triggered, [=]() {
        if (!dirPath.isEmpty()) {
            QString target = videoPath.isEmpty() ? dirPath : videoPath;
            QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(target)});
        }
    });

    menu.exec(m_gridView->viewport()->mapToGlobal(pos));
}
