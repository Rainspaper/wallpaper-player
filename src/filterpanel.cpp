#include "filterpanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

static const char *SECTION_STYLE =
    "font-size:11px; font-weight:700; color:#888; letter-spacing:0.5px;"
    "padding:6px 0 2px 0;";

// ── Section header with left accent bar ──
static QWidget *makeSection(QWidget *parent, const QString &text)
{
    auto *w = new QWidget(parent);
    auto *lo = new QHBoxLayout(w);
    lo->setContentsMargins(0, 0, 0, 0);
    lo->setSpacing(8);

    auto *bar = new QFrame(w);
    bar->setFixedSize(3, 14);
    bar->setStyleSheet("background:#5a8; border-radius:1px;");

    auto *label = new QLabel(text, w);
    label->setStyleSheet(SECTION_STYLE);

    lo->addWidget(bar);
    lo->addWidget(label);
    lo->addStretch();
    return w;
}

// ── Separator ──
static QFrame *makeSep(QWidget *parent)
{
    auto *s = new QFrame(parent);
    s->setFixedHeight(1);
    s->setStyleSheet("background:#2e2e32;");
    return s;
}

FilterPanel::FilterPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(220);
    setStyleSheet(
        "FilterPanel { background:#252529; border-right:1px solid #1e1e22; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 16, 14, 14);
    root->setSpacing(8);

    // ═══ Title ═══
    auto *titleRow = new QHBoxLayout;
    auto *titleIcon = new QLabel("▶");
    titleIcon->setStyleSheet("font-size:10px; color:#5a8;");
    auto *titleLabel = new QLabel("筛选");
    titleLabel->setStyleSheet(
        "font-size:14px; font-weight:700; color:#eee; letter-spacing:0.5px;");
    titleRow->addWidget(titleIcon);
    titleRow->addSpacing(6);
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();
    root->addLayout(titleRow);
    root->addSpacing(4);

    // ═══ Search ═══
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("搜索标题...");
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "  background:#1e1e22; border:1px solid #333; border-radius:8px;"
        "  padding:8px 12px; color:#ddd; font-size:13px;"
        "}"
        "QLineEdit:focus { border-color:#5a8; }"
        "QLineEdit:hover:!focus { border-color:#444; }");
    root->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &FilterPanel::filtersChanged);

    root->addWidget(makeSep(this));

    // ═══ Rating ═══
    root->addWidget(makeSection(this, "分 级"));

    m_ratingGroup = new QButtonGroup(this);
    m_ratingContainer = new QWidget;
    m_ratingContainer->setLayout(new QVBoxLayout);
    m_ratingContainer->layout()->setContentsMargins(0, 0, 0, 0);
    m_ratingContainer->layout()->setSpacing(0);
    root->addWidget(m_ratingContainer);

    // start with just "全部" — setRatings() will add the rest
    setRatings({});

    root->addWidget(makeSep(this));

    // ═══ Tags ═══
    auto *tagHeaderRow = new QHBoxLayout;
    tagHeaderRow->addWidget(makeSection(this, "标签筛选"));
    tagHeaderRow->addStretch();
    auto *resetBtn = new QPushButton("重置");
    resetBtn->setFixedHeight(20);
    resetBtn->setStyleSheet(
        "QPushButton {"
        "  background:transparent; border:none;"
        "  color:#5a8; font-size:11px; padding:0 4px;"
        "}"
        "QPushButton:hover { color:#7dc9a0; }"
        "QPushButton:pressed { color:#4a7; }");
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_tagList->count(); ++i)
            m_tagList->item(i)->setCheckState(Qt::Unchecked);
    });
    tagHeaderRow->addWidget(resetBtn);
    root->addLayout(tagHeaderRow);

    m_tagList = new QListWidget;
    m_tagList->setStyleSheet(
        "QListWidget {"
        "  background:transparent; border:none;"
        "  font-size:12px; outline:none;"
        "}"
        "QListWidget::item {"
        "  padding:3px 4px; color:#bbb; border-radius:4px;"
        "}"
        "QListWidget::item:hover { background:#2a2a2e; }"
        "QListWidget::item:selected { background:transparent; color:#bbb; }"
        "QListWidget::indicator {"
        "  width:14px; height:14px; border-radius:3px;"
        "  border:2px solid #4a4a4e; background:#1e1e22;"
        "  margin-right:4px;"
        "}"
        "QListWidget::indicator:hover { border-color:#666; }"
        "QListWidget::indicator:checked {"
        "  background:#5a8; border-color:#5a8;"
        "}");
    m_tagList->setMaximumHeight(200);
    m_tagList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tagList->setSelectionMode(QAbstractItemView::NoSelection);
    connect(m_tagList, &QListWidget::itemChanged,
            this, &FilterPanel::filtersChanged);
    connect(m_tagList, &QListWidget::itemClicked,
            [](QListWidgetItem *item) {
        item->setCheckState(
            item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    });
    root->addWidget(m_tagList);

    root->addStretch();

    // ═══ Count ═══
    m_countLabel = new QLabel("共 0 项");
    m_countLabel->setStyleSheet(
        "font-size:11px; color:#666; padding:6px 0 0 0;"
        "border-top:1px solid #2e2e32;");
    root->addWidget(m_countLabel);
}

void FilterPanel::setTags(const QStringList &tags)
{
    m_tagList->clear();
    for (const auto &tag : tags) {
        auto *item = new QListWidgetItem(tag);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        // Custom checkbox styling via delegate is overkill; we style the
        // indicator globally and rely on the item text + check state.
        m_tagList->addItem(item);
    }
}

void FilterPanel::setRatings(const QStringList &ratings)
{
    // clear old radio buttons
    for (auto *b : m_ratingGroup->buttons()) {
        m_ratingGroup->removeButton(b);
        delete b;
    }

    auto rbStyle = QString(
        "QRadioButton {"
        "  color:#bbb; font-size:12px; spacing:8px;"
        "  padding:3px 0;"
        "}"
        "QRadioButton::indicator {"
        "  width:14px; height:14px; border-radius:7px;"
        "  border:2px solid #4a4a4e; background:#1e1e22;"
        "}"
        "QRadioButton::indicator:hover { border-color:#666; }"
        "QRadioButton::indicator:checked {"
        "  background:#5a8; border-color:#5a8;"
        "}");

    auto *lay = qobject_cast<QVBoxLayout*>(m_ratingContainer->layout());

    auto addRb = [&](const QString &text, const QString &val) {
        Q_UNUSED(val);
        auto *rb = new QRadioButton(text);
        rb->setStyleSheet(rbStyle);
        m_ratingGroup->addButton(rb);
        lay->addWidget(rb);
        connect(rb, &QRadioButton::toggled,
                this, &FilterPanel::filtersChanged);
    };

    addRb("全部", "All");

    // collect unique non-empty ratings, sorted
    QStringList unique;
    for (const auto &r : ratings) {
        if (!r.isEmpty() && !unique.contains(r))
            unique.append(r);
    }
    unique.sort(Qt::CaseInsensitive);
    for (const auto &r : unique)
        addRb(r, r);

    m_ratingGroup->buttons().first()->setChecked(true);
}

QString FilterPanel::searchText() const
{
    return m_searchEdit->text().trimmed();
}

QString FilterPanel::selectedRating() const
{
    auto *checked = m_ratingGroup->checkedButton();
    if (!checked) return "All";
    QString t = checked->text();
    if (t == "全部") return "All";
    return t;
}

QStringList FilterPanel::selectedTags() const
{
    QStringList tags;
    for (int i = 0; i < m_tagList->count(); ++i) {
        auto *item = m_tagList->item(i);
        if (item->checkState() == Qt::Checked)
            tags.append(item->text());
    }
    return tags;
}

void FilterPanel::updateResultCount(int visible, int total)
{
    m_countLabel->setText(
        QString("显示 %1 / %2 项").arg(visible).arg(total));
}
