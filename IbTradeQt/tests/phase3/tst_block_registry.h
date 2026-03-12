#ifndef TST_BLOCK_REGISTRY_H
#define TST_BLOCK_REGISTRY_H

#include <QtTest>

// Phase 3: Block registry + plugin system tests
// Implement when BlockRegistry and plugin loading are created.
//
// Planned tests:
//  - Register alpha block descriptor → discoverable by id
//  - Register multiple block types → list all
//  - Create block instance from descriptor
//  - Duplicate id registration fails
//  - Plugin loading: load .so → blocks registered
//  - Plugin loading: invalid .so → error, no crash
//  - Block configuration: setConfig persists across create
//  - Legacy adapter: CBasicAlphaModel wrapped as IAlphaBlock
//  - Legacy adapter: CBasicRiskModel wrapped as IRiskBlock
//  - Legacy adapter: CBasicExecutionModel wrapped as IExecutionBlock
//  - Block graph persistence: save/load block graph as JSON

class TestBlockRegistry : public QObject
{
    Q_OBJECT

private slots:
    void placeholder()
    {
        QSKIP("Phase 3 not yet implemented");
    }
};

#endif // TST_BLOCK_REGISTRY_H
