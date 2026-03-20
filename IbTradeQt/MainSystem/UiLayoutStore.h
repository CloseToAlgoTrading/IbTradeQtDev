#ifndef UILAYOUTSTORE_H
#define UILAYOUTSTORE_H

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QByteArray>
#include <QTimer>

class ModelTreeRepository;
class QMainWindow;
class QTabWidget;
class QSplitter;
class QAbstractItemView;
class CIBTradeSystemView;

// Single persistence path for main-window chrome: one JSON blob in app_metadata (key ui_layout_v1).
class UiLayoutStore : public QObject
{
    Q_OBJECT
public:
    static constexpr int kJsonSchema = 1;
    static constexpr int kQtStateVersion = 1;

    explicit UiLayoutStore(QObject* parent = nullptr);

    void setRepository(ModelTreeRepository* repo);

    // Registers splitters, tab widget, header views, and wires debounced save.
    void attachToView(CIBTradeSystemView* view);

    void load();
    void save();

    // Apply header states loaded from DB (call after models/columns are ready).
    void restorePendingHeaders();

    void scheduleSave();

    void clearPersistedLayout();

private slots:
    void onSaveTimer();

private:
    void registerSplitter(QSplitter* s);
    void registerHeaderView(QAbstractItemView* v);
    static QString bytesToJson(const QByteArray& b);
    static QByteArray bytesFromJson(const QString& s);

    ModelTreeRepository* m_repo = nullptr;

    QPointer<QMainWindow> m_mainWindow;
    QPointer<QTabWidget>  m_mainTabWidget;

    QList<QPointer<QSplitter>>          m_splitters;
    QList<QPointer<QAbstractItemView>> m_headerViews;

    QHash<QString, QByteArray> m_pendingHeaderStates;

    QTimer* m_saveTimer = nullptr;
};

#endif
