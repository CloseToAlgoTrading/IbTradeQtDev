#ifndef TST_EXPECTED_H
#define TST_EXPECTED_H

#include <QtTest>
#include "Common/Expected.h"

class TestExpected : public QObject
{
    Q_OBJECT

private slots:
    void successValue()
    {
        Expected<int> result = 42;
        QVERIFY(result.has_value());
        QCOMPARE(*result, 42);
    }

    void errorValue()
    {
        Expected<int> result = make_unexpected(
            Error{ErrorCode::InvalidArgument, "bad input", "test"});
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::InvalidArgument);
        QCOMPARE(result.error().message, std::string("bad input"));
        QCOMPARE(result.error().context, std::string("test"));
    }

    void errorToString()
    {
        Error err{ErrorCode::BrokerConnectionFailed, "connection refused", "IBAdapter"};
        std::string str = err.toString();
        QVERIFY(str.find("connection refused") != std::string::npos);
        QVERIFY(str.find("IBAdapter") != std::string::npos);
    }

    void voidExpectedSuccess()
    {
        Expected<void> result;
        QVERIFY(result.has_value());
    }

    void voidExpectedError()
    {
        Expected<void> result = make_unexpected(
            Error{ErrorCode::DatabaseError, "insert failed", "DBAdapter"});
        QVERIFY(!result.has_value());
        QCOMPARE(result.error().code, ErrorCode::DatabaseError);
    }

    void stringExpected()
    {
        Expected<std::string> result = std::string("hello");
        QVERIFY(result.has_value());
        QCOMPARE(*result, std::string("hello"));
    }

    void errorCodeValues()
    {
        QCOMPARE(static_cast<int>(ErrorCode::Success), 0);
        QCOMPARE(static_cast<int>(ErrorCode::InvalidArgument), 1);
        QCOMPARE(static_cast<int>(ErrorCode::NotFound), 2);
        QCOMPARE(static_cast<int>(ErrorCode::BrokerConnectionFailed), 3);
        QCOMPARE(static_cast<int>(ErrorCode::DatabaseError), 4);
        QCOMPARE(static_cast<int>(ErrorCode::OrderRejected), 5);
        QCOMPARE(static_cast<int>(ErrorCode::InsufficientFunds), 6);
        QCOMPARE(static_cast<int>(ErrorCode::ConfigurationError), 7);
        QCOMPARE(static_cast<int>(ErrorCode::Timeout), 8);
        QCOMPARE(static_cast<int>(ErrorCode::UnknownError), 9);
    }

    void mapOnSuccess()
    {
        Expected<int> result = 5;
        auto mapped = result.map([](int v) { return v * 2; });
        QVERIFY(mapped.has_value());
        QCOMPARE(*mapped, 10);
    }

    void mapOnError()
    {
        Expected<int> result = make_unexpected(
            Error{ErrorCode::NotFound, "missing", ""});
        auto mapped = result.map([](int v) { return v * 2; });
        QVERIFY(!mapped.has_value());
        QCOMPARE(mapped.error().code, ErrorCode::NotFound);
    }

    void valueOr()
    {
        Expected<int> success = 42;
        QCOMPARE(success.value_or(0), 42);

        Expected<int> failure = make_unexpected(
            Error{ErrorCode::UnknownError, "err", ""});
        QCOMPARE(failure.value_or(-1), -1);
    }

    void complexTypeExpected()
    {
        struct Payload {
            int id;
            std::string name;
        };

        Expected<Payload> result = Payload{1, "test"};
        QVERIFY(result.has_value());
        QCOMPARE(result->id, 1);
        QCOMPARE(result->name, std::string("test"));
    }
};

#endif // TST_EXPECTED_H
