#ifndef TST_BLOCK_REGISTRY_H
#define TST_BLOCK_REGISTRY_H

#include <QtTest>
#include <QSignalSpy>
#include "Pipeline/BlockRegistry.h"
#include "Pipeline/IAlphaBlock.h"
#include "Pipeline/IRiskBlock.h"
#include "Pipeline/ISelectionBlock.h"
#include "Blocks/MomentumAlphaBlock.h"
#include "Blocks/MaxPositionRiskBlock.h"
#include "Blocks/MarketOrderExecutionBlock.h"

class TestBlockRegistry : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        Pipeline::BlockRegistry::instance().clear();
    }

    void registerAndLookup()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        Pipeline::BlockDescriptor desc;
        desc.id = "test-alpha-1";
        desc.name = "Test Alpha";
        desc.category = "Alpha";
        desc.description = "A test alpha block";
        desc.factory = []() -> QObject* {
            return new Blocks::MomentumAlphaBlock();
        };

        reg.registerBlock(desc);

        QVERIFY(reg.contains("test-alpha-1"));
        QCOMPARE(reg.blockCount(), 1);

        auto result = reg.descriptor("test-alpha-1");
        QVERIFY(result.has_value());
        QCOMPARE(result->name, QString("Test Alpha"));
        QCOMPARE(result->category, QString("Alpha"));
    }

    void createBlockFromFactory()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        Pipeline::BlockDescriptor desc;
        desc.id = "momentum-alpha";
        desc.name = "Momentum Alpha";
        desc.category = "Alpha";
        desc.factory = []() -> QObject* {
            return new Blocks::MomentumAlphaBlock();
        };
        reg.registerBlock(desc);

        auto result = reg.createBlock("momentum-alpha");
        QVERIFY(result.has_value());

        auto* alpha = qobject_cast<Pipeline::IAlphaBlock*>(*result);
        QVERIFY(alpha != nullptr);
        QCOMPARE(alpha->id(), QString("momentum-alpha"));

        delete *result;
    }

    void createBlockNotFound()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        auto result = reg.createBlock("nonexistent-block");
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::NotFound);
    }

    void blocksByCategory()
    {
        auto& reg = Pipeline::BlockRegistry::instance();

        reg.registerBlock({"alpha-1", "Alpha 1", "Alpha", "desc", Pipeline::Scope::Strategy, {}, []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});
        reg.registerBlock({"alpha-2", "Alpha 2", "Alpha", "desc", Pipeline::Scope::Strategy, {}, []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});
        reg.registerBlock({"risk-1", "Risk 1", "Risk", "desc", Pipeline::Scope::Strategy, {}, []() -> QObject* { return new Blocks::MaxPositionRiskBlock(); }});

        auto alphas = reg.blocksByCategory("Alpha");
        QCOMPARE(alphas.size(), 2);

        auto risks = reg.blocksByCategory("Risk");
        QCOMPARE(risks.size(), 1);

        auto empty = reg.blocksByCategory("Execution");
        QCOMPARE(empty.size(), 0);
    }

    void blockIdsByCategory()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"momentum-alpha", "M", "Alpha", "", Pipeline::Scope::Strategy, {}, []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }});
        reg.registerBlock({"max-risk", "R", "Risk", "", Pipeline::Scope::Strategy, {}, []() -> QObject* { return new Blocks::MaxPositionRiskBlock(); }});

        auto ids = reg.blockIdsByCategory("Alpha");
        QCOMPARE(ids.size(), 1);
        QVERIFY(ids.contains("momentum-alpha"));
    }

    void unregisterBlock()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"temp-block", "Temp", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});
        QVERIFY(reg.contains("temp-block"));

        bool removed = reg.unregisterBlock("temp-block");
        QVERIFY(removed);
        QVERIFY(!reg.contains("temp-block"));

        bool removedAgain = reg.unregisterBlock("temp-block");
        QVERIFY(!removedAgain);
    }

    void registrationEmitsSignal()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        QSignalSpy spy(&reg, &Pipeline::BlockRegistry::blockRegistered);

        reg.registerBlock({"sig-test", "Signal Test", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("sig-test"));
        QCOMPARE(spy.at(0).at(1).toString(), QString("Alpha"));
    }

    void noFactoryReturnsError()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"no-factory", "No Factory", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});

        auto result = reg.createBlock("no-factory");
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::ConfigurationError);
    }

    void allBlockIds()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"a", "A", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});
        reg.registerBlock({"b", "B", "Risk", "", Pipeline::Scope::Strategy, {}, nullptr});
        reg.registerBlock({"c", "C", "Execution", "", Pipeline::Scope::Strategy, {}, nullptr});

        auto ids = reg.allBlockIds();
        QCOMPARE(ids.size(), 3);
    }

    void clearRegistry()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"x", "X", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});
        reg.registerBlock({"y", "Y", "Risk", "", Pipeline::Scope::Strategy, {}, nullptr});
        QCOMPARE(reg.blockCount(), 2);

        reg.clear();
        QCOMPARE(reg.blockCount(), 0);
    }

    void defaultConfigStored()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        QJsonObject defCfg;
        defCfg["period"] = 20;
        defCfg["threshold"] = 0.02;

        reg.registerBlock({"cfg-test", "Cfg Test", "Alpha", "",
                          Pipeline::Scope::Strategy, defCfg, nullptr});

        auto desc = reg.descriptor("cfg-test");
        QVERIFY(desc.has_value());
        QCOMPARE(desc->defaultConfig["period"].toInt(), 20);
        QCOMPARE(desc->defaultConfig["threshold"].toDouble(), 0.02);
    }

    void registerMultipleBlocksSameCategory()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        for (int i = 0; i < 5; ++i) {
            reg.registerBlock({
                QString("alpha-%1").arg(i),
                QString("Alpha %1").arg(i),
                "Alpha", "", Pipeline::Scope::Strategy, {},
                []() -> QObject* { return new Blocks::MomentumAlphaBlock(); }
            });
        }

        QCOMPARE(reg.blockCount(), 5);
        QCOMPARE(reg.blocksByCategory("Alpha").size(), 5);
    }

    void overwriteExistingBlock()
    {
        auto& reg = Pipeline::BlockRegistry::instance();
        reg.registerBlock({"dup", "Original", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});
        reg.registerBlock({"dup", "Overwritten", "Alpha", "", Pipeline::Scope::Strategy, {}, nullptr});

        QCOMPARE(reg.blockCount(), 1);
        auto desc = reg.descriptor("dup");
        QCOMPARE(desc->name, QString("Overwritten"));
    }
};

#endif // TST_BLOCK_REGISTRY_H
