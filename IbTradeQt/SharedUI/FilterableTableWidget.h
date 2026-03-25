#ifndef SHAREDUI_FILTERABLETABLEWIDGET_H
#define SHAREDUI_FILTERABLETABLEWIDGET_H

#include <QWidget>
#include <QVector>

class QTableView;
class QLineEdit;
class QComboBox;

/// Two-column table with text search (both columns) and optional status-category filter.
/// Styling: set object name #FilterableTableWidget; see operations-console.qss.
class FilterableTableWidget : public QWidget {
public:
    struct Row {
        QString col0;
        QString col1;
        /// Filter bucket: fully_cached | needs_fetch | partial_gap | yahoo_error
        QString statusCategory;
    };

    explicit FilterableTableWidget(QWidget* parent = nullptr);

    void setColumnHeaders(const QString& col0Label, const QString& col1Label);
    void setRows(const QVector<Row>& rows);
    void clear();
    void setFilterPlaceholder(const QString& text);

    QTableView* tableView() const { return m_table; }

private:
    void rebuildStatusFilterOptions(const QVector<Row>& rows);

    QLineEdit*  m_filterEdit = nullptr;
    QComboBox*  m_statusCombo = nullptr;
    QTableView* m_table      = nullptr;
};

#endif
