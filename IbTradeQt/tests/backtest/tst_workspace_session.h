#ifndef TST_WORKSPACE_SESSION_H
#define TST_WORKSPACE_SESSION_H

#include <QObject>
#include <QtTest>

class TestWorkspaceSession : public QObject {
    Q_OBJECT
private slots:
    void canonicalJson_isStableForKeyOrder();
    void sessionDirty_detectsPipelineChange();
    void sessionDirty_detectsRunFieldsChange();
    void sessionKey_qHash_equality();
};

#endif // TST_WORKSPACE_SESSION_H
