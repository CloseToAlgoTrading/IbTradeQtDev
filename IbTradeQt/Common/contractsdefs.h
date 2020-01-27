#ifndef CONTRACTSDEFS_H
#define CONTRACTSDEFS_H

#include <QString>
struct Contract;

class ContractsDefs
{

    public:
        static Contract IBMBond();
        static Contract IBKRStk();
        static Contract HKStk();
        static Contract EurGbpFx();
        static Contract Index(const QString & _symbol);
        static Contract CFD(const QString & _symbol);
        static Contract USStockCFD(const QString & _symbol);
        static Contract EuropeanStockCFD(const QString & _symbol);
        static Contract CashCFD(const QString & _symbol);
        static Contract EuropeanStock(const QString & _symbol);
        static Contract OptionAtIse(const QString & _symbol);
        static Contract USStock(const QString & _symbol);
        static Contract USStockAtSmart(const QString & _symbol);
        static Contract IBMUSStockAtSmart(const QString & _symbol);
        static Contract USStockWithPrimaryExch(const QString & _symbol);
        static Contract BondWithCusip(const QString & _symbol);
        static Contract Bond(const QString & _symbol);
        static Contract MutualFund(const QString & _symbol);
        static Contract Commodity(const QString & _symbol);
        static Contract USOptionContract(const QString & _symbol, const QString &_date, const qreal _strike, bool isCall);
        static Contract OptionAtBox(const QString & _symbol);
        static Contract NormalOption(const QString & _symbol);
        static Contract OptionWithTradingClass(const QString & _symbol);
        static Contract OptionWithLocalSymbol(const QString & _symbol);
        static Contract DutchWarrant(const QString & _symbol);
        static Contract SimpleFuture(const QString & _symbol);
        static Contract FutureWithLocalSymbol(const QString & _symbol);
        static Contract FutureWithMultiplier(const QString & _symbol);
        static Contract WrongContract(const QString & _symbol);
        static Contract FuturesOnOptions(const QString & _symbol);
        static Contract ByISIN(const QString & _symbol);
        static Contract ByConId(const QString & _symbol);
        static Contract OptionForQuery(const QString & _symbol);
        static Contract StockComboContract(const QString & _symbol);
        static Contract FutureComboContract(const QString & _symbol);
        static Contract SmartFutureComboContract(const QString & _symbol);
        static Contract OptionComboContract(const QString & _symbol);
        static Contract InterCmdtyFuturesContract(const QString & _symbol);
        static Contract NewsFeedForQuery(const QString & _symbol);
        static Contract BTbroadtapeNewsFeed(const QString & _symbol);
        static Contract BZbroadtapeNewsFeed(const QString & _symbol);
        static Contract FLYbroadtapeNewsFeed(const QString & _symbol);
        static Contract MTbroadtapeNewsFeed(const QString & _symbol);
        static Contract ContFut(const QString & _symbol);
        static Contract ContAndExpiringFut(const QString & _symbol);
        static Contract JefferiesContract(const QString & _symbol);
        static Contract CSFBContract(const QString & _symbol);
};

#endif // CONTRACTSDEFS_H
