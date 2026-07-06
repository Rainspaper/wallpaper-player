#ifndef FILTERPANEL_H
#define FILTERPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QButtonGroup>
#include <QRadioButton>
#include <QListWidget>
#include <QLabel>

class FilterPanel : public QWidget {
    Q_OBJECT
public:
    explicit FilterPanel(QWidget *parent = nullptr);

    void setTags(const QStringList &tags);
    QString searchText() const;
    QString selectedRating() const; // "All", "Everyone", "Mature"
    QStringList selectedTags() const;

    void updateResultCount(int visible, int total);

signals:
    void filtersChanged();

private:
    QLineEdit   *m_searchEdit;
    QButtonGroup *m_ratingGroup;
    QListWidget *m_tagList;
    QLabel      *m_countLabel;
};

#endif // FILTERPANEL_H
