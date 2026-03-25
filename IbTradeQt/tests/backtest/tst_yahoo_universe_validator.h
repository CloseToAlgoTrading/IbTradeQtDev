#ifndef TST_YAHOO_UNIVERSE_VALIDATOR_H
#define TST_YAHOO_UNIVERSE_VALIDATOR_H

#include <QObject>

class TestYahooUniverseValidator : public QObject {
    Q_OBJECT
private slots:
    void validate_successAndFailureSplit();
};

#endif
