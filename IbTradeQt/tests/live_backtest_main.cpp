#include <QCoreApplication>
#include <QtTest>

#include "backtest/tst_live_backtest.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    int status = 0;
    { TestLiveBacktest tc; status |= QTest::qExec(&tc, argc, argv); }
    return status;
}
