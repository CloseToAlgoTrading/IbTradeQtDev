#include "contractsdefs.h"

#include "Contract.h"

    /*
     * Contracts can be defined in multiple ways. The TWS/IB Gateway will always perform a query on the available contracts
     * and find which one is the best candidate:
     *  - More than a single candidate will yield an ambiguity error message.
     *  - No suitable candidates will produce a "contract not found" message.
     *  How do I find my contract though?
     *  - Often the quickest way is by looking for it in the TWS and looking at its description there (double click).
     *  - The TWS' symbol corresponds to the API's localSymbol. Keep this in mind when defining Futures or Options.
     *  - The TWS' underlying's symbol can usually be mapped to the API's symbol.
     *
     * Any stock or option symbols displayed are for illustrative purposes only and are not intended to portray a recommendation.
     */

    /*
     * Usually, the easiest way to define a Stock/CASH contract is through these four attributes.
     */
Contract ContractsDefs::IBMBond(){
    //! [IBM bond contract]
    Contract contract;
    contract.symbol = "IBM";
    contract.secType = "BOND";
    contract.currency = "USD";
    contract.exchange = "SMART";
    //! [IBM bond contract]
    return contract;
}

Contract ContractsDefs::IBKRStk(){
    //! [IBKR contract]
    Contract contract;
    contract.symbol = "IBKR";
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    //! [IBKR contract]
    return contract;
}

Contract ContractsDefs::HKStk(){
    //! [1@SEHK contract]
    Contract contract;
    contract.symbol = "1";
    contract.secType = "STK";
    contract.currency = "HKD";
    contract.exchange = "SEHK";
    //! [1@SEHK contract]
    return contract;
}

Contract ContractsDefs::EurGbpFx(){
    //! [cashcontract]
    Contract contract;
    contract.symbol = "EUR";
    contract.secType = "CASH";
    contract.currency = "GBP";
    contract.exchange = "IDEALPRO";
    //! [cashcontract]
    return contract;
}

Contract ContractsDefs::Index(const QString & _symbol){
    //! [indcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "DAX";
    contract.secType = "IND";
    contract.currency = "EUR";
    contract.exchange = "DTB";
    //! [indcontract]
    return contract;
}

Contract ContractsDefs::CFD(const QString & _symbol){
    //! [cfdcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBDE30";
    contract.secType = "CFD";
    contract.currency = "EUR";
    contract.exchange = "SMART";
    //! [cfdcontract]
    return contract;
}

Contract ContractsDefs::USStockCFD(const QString & _symbol){
    //! [usstockcfdcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBM";
    contract.secType = "CFD";
    contract.currency = "USD";
    contract.exchange = "SMART";
    //! [usstockcfdcontract]
    return contract;
}

Contract ContractsDefs::EuropeanStockCFD(const QString & _symbol){
    //! [europeanstockcfdcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "BMW";
    contract.secType = "CFD";
    contract.currency = "EUR";
    contract.exchange = "SMART";
    //! [europeanstockcfdcontract]
    return contract;
}

Contract ContractsDefs::CashCFD(const QString & _symbol){
    //! [cashcfdcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "EUR";
    contract.secType = "CFD";
    contract.currency = "USD";
    contract.exchange = "SMART";
    //! [cashcfdcontract]
    return contract;
}

Contract ContractsDefs::EuropeanStock(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "NOKIA";
    contract.secType = "STK";
    contract.currency = "EUR";
    contract.exchange = "SMART";
    contract.primaryExchange = "HEX";
    return contract;
}

Contract ContractsDefs::OptionAtIse(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "BPX";
    contract.secType = "OPT";
    contract.currency = "USD";
    contract.exchange = "ISE";
    contract.lastTradeDateOrContractMonth = "20160916";
    contract.right = "C";
    contract.strike = 65;
    contract.multiplier = "100";
    return contract;
}

Contract ContractsDefs::USStock(const QString & _symbol){
    //! [stkcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBKR";
    contract.secType = "STK";
    contract.currency = "USD";
    //In the API side, NASDAQ is always defined as ISLAND
    contract.exchange = "ISLAND";
    //! [stkcontract]
    return contract;
}

Contract ContractsDefs::USStockAtSmart(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBM";
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    return contract;
}

Contract ContractsDefs::IBMUSStockAtSmart(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBM";
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    return contract;
}

Contract ContractsDefs::USStockWithPrimaryExch(const QString & _symbol){
    //! [stkcontractwithprimary]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "AAPL";
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    // Specify the Primary Exchange attribute to avoid contract ambiguity
    // (there is an ambiguity because there is also a MSFT contract with primary exchange = "AEB")
    contract.primaryExchange = "ISLAND";
    //! [stkcontractwithprimary]
    return contract;
}

Contract ContractsDefs::BondWithCusip(const QString & _symbol) {
    //! [bondwithcusip]
    Contract contract;
    // enter CUSIP as symbol
    contract.symbol= "912828C57";
    contract.secType = "BOND";
    contract.exchange = "SMART";
    contract.currency = "USD";
    //! [bondwithcusip]
    return contract;
}

Contract ContractsDefs::Bond(const QString & _symbol) {
    //! [bond]
    Contract contract;
    contract.conId = 285191782;
    contract.exchange = "SMART";
    //! [bond]
    return contract;
}

Contract ContractsDefs::MutualFund(const QString & _symbol) {
    //! [fundcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "VINIX";
    contract.secType = "FUND";
    contract.exchange = "FUNDSERV";
    contract.currency = "USD";
    //! [fundcontract]
    return contract;
}

Contract ContractsDefs::Commodity(const QString & _symbol) {
    //! [commoditycontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "XAUUSD";
    contract.secType = "CMDTY";
    contract.exchange = "SMART";
    contract.currency = "USD";
    //! [commoditycontract]
    return contract;
}

Contract ContractsDefs::USOptionContract(const QString & _symbol,
                                         const QString & _date,
                                         const qreal _strike,
                                         bool isCall){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "GOOG";
    contract.secType = "OPT";
    contract.exchange = "SMART";
    contract.currency = "USD";
    contract.lastTradeDateOrContractMonth = _date.toStdString();//"20170120";
    contract.strike = _strike; //615;
    if(true == isCall)
    {
        contract.right = "C";
    }
    else {
        contract.right = "P";
    }
    contract.multiplier = "100";
    return contract;
}

Contract ContractsDefs::OptionAtBox(const QString & _symbol){
    //! [optcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "GOOG";
    contract.secType = "OPT";
    contract.exchange = "BOX";
    contract.currency = "USD";
    contract.lastTradeDateOrContractMonth = "20170120";
    contract.strike = 615;
    contract.right = "C";
    contract.multiplier = "100";
    //! [optcontract]
    return contract;
}

    /*
     *Option contracts require far more information since there are many contracts having the exact same
     *attributes such as symbol, currency, strike, etc.
     */
Contract ContractsDefs::NormalOption(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "BAYN";
    contract.secType = "OPT";
    contract.exchange = "DTB";
    contract.currency = "EUR";
    contract.lastTradeDateOrContractMonth = "20161216";
    contract.strike = 100;
    contract.right = "C";
    contract.multiplier = "100";
    //Often, contracts will also require a trading class to rule out ambiguities
    contract.tradingClass = "BAY";
    return contract;
}

    /*
     * Option contracts require far more information since there are many contracts having the exact same
     * attributes such as symbol, currency, strike, etc.
     */
Contract ContractsDefs::OptionWithTradingClass(const QString & _symbol){
    //! [optcontract_tradingclass]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "SANT";
    contract.secType = "OPT";
    contract.exchange = "MEFFRV";
    contract.currency = "EUR";
    contract.lastTradeDateOrContractMonth = "20190621";
    contract.strike = 7.5;
    contract.right = "C";
    contract.multiplier = "100";
    contract.tradingClass = "SANEU";
    //! [optcontract_tradingclass]
    return contract;
}

/*
 * Using the contract's own symbol (localSymbol) can greatly simplify a contract description
 */
Contract ContractsDefs::OptionWithLocalSymbol(const QString & _symbol){
    //! [optcontract_localsymbol]
    Contract contract;
    //Watch out for the spaces within the local symbol!
    contract.localSymbol = "C DBK  DEC 20  1600";
    contract.secType = "OPT";
    contract.exchange = "DTB";
    contract.currency = "EUR";
    //! [optcontract_localsymbol]
    return contract;
}

/*
 * Dutch Warrants (IOPTs) can be defined using the local symbol or conid
 */

Contract ContractsDefs::DutchWarrant(const QString & _symbol){
    //! [ioptcontract]
    Contract contract;
    contract.localSymbol = "B881G";
    contract.secType = "IOPT";
    contract.exchange = "SBF";
    contract.currency = "EUR";
    //! [ioptcontract]
    return contract;
}

    /*
     * Future contracts also require an expiration date but are less complicated than options.
     */
Contract ContractsDefs::SimpleFuture(const QString & _symbol){
    //! [futcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "ES";
    contract.secType = "FUT";
    contract.exchange = "GLOBEX";
    contract.currency = "USD";
    contract.lastTradeDateOrContractMonth = "201803";
    //! [futcontract]
    return contract;
}

    /*
     * Rather than giving expiration dates we can also provide the local symbol
     * attributes such as symbol, currency, strike, etc.
     */
Contract ContractsDefs::FutureWithLocalSymbol(const QString & _symbol){
    //! [futcontract_local_symbol]
    Contract contract;
    contract.secType = "FUT";
    contract.exchange = "GLOBEX";
    contract.currency = "USD";
    contract.localSymbol = "ESZ6";
    //! [futcontract_local_symbol]
    return contract;
}

Contract ContractsDefs::FutureWithMultiplier(const QString & _symbol){
    //! [futcontract_multiplier]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "DAX";
    contract.secType = "FUT";
    contract.exchange = "DTB";
    contract.currency = "EUR";
    contract.lastTradeDateOrContractMonth = "201609";
    contract.multiplier = "5";
    //! [futcontract_multiplier]
    return contract;
}

    /*
     * Note the space in the symbol!
     */
Contract ContractsDefs::WrongContract(const QString & _symbol){
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // " IJR ";
    contract.conId = 9579976;
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";
    return contract;
}

Contract ContractsDefs::FuturesOnOptions(const QString & _symbol){
    //! [fopcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "ES";
    contract.secType = "FOP";
    contract.exchange = "GLOBEX";
    contract.currency = "USD";
    contract.lastTradeDateOrContractMonth = "20180316";
    contract.strike = 2800;
    contract.right = "C";
    contract.multiplier = "50";
    //! [fopcontract]
    return contract;
}

    /*
     * It is also possible to define contracts based on their ISIN (IBKR STK sample).
     *
     */
Contract ContractsDefs::ByISIN(const QString & _symbol){
    Contract contract;
    contract.secIdType = "ISIN";
    contract.secId = "US45841N1072";
    contract.exchange = "SMART";
    contract.currency = "USD";
    contract.secType = "STK";
    return contract;
}

    /*
     * Or their conId (EUR.USD sample).
     * Note: passing a contract containing the conId can cause problems if one of the other provided
     * attributes does not match 100% with what is in IB's database. This is particularly important
     * for contracts such as Bonds which may change their description from one day to another.
     * If the conId is provided, it is best not to give too much information as in the example below.
     */
Contract ContractsDefs::ByConId(const QString & _symbol){
    Contract contract;
    contract.conId = 12087792;
    contract.exchange = "IDEALPRO";
    contract.secType = "CASH";
    return contract;
}

    /*
     * Ambiguous contracts are great to use with reqContractDetails. This way you can
     * query the whole option chain for an underlying. Bear in mind that there are
     * pacing mechanisms in place which will delay any further responses from the TWS
     * to prevent abuse.
     */
Contract ContractsDefs::OptionForQuery(const QString & _symbol){
    //! [optionforquery]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "FISV";
    contract.secType = "OPT";
    contract.exchange = "SMART";
    contract.currency = "USD";
    //! [optionforquery]
    return contract;
}

Contract ContractsDefs::OptionComboContract(const QString & _symbol){
    //! [bagoptcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "DBK";
    contract.secType = "BAG";
    contract.currency = "EUR";
    contract.exchange = "DTB";

    ComboLegSPtr leg1(new ComboLeg);
    leg1->conId = 197397509;
    leg1->action = "BUY";
    leg1->ratio = 1;
    leg1->exchange = "DTB";

    ComboLegSPtr leg2(new ComboLeg);
    leg2->conId = 197397584;
    leg2->action = "SELL";
    leg2->ratio = 1;
    leg2->exchange = "DTB";

    contract.comboLegs.reset(new Contract::ComboLegList());
    contract.comboLegs->push_back(leg1);
    contract.comboLegs->push_back(leg2);
    //! [bagoptcontract]
    return contract;
}

    /*
     * STK Combo contract
     * Leg 1: 43645865 - IBKR's STK
     * Leg 2: 9408 - McDonald's STK
     */
Contract ContractsDefs::StockComboContract(const QString & _symbol){
    //! [bagstkcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "MCD";
    contract.secType = "BAG";
    contract.currency = "USD";
    contract.exchange = "SMART";

    ComboLegSPtr leg1(new ComboLeg);
    leg1->conId = 43645865;
    leg1->action = "BUY";
    leg1->ratio = 1;
    leg1->exchange = "SMART";

    ComboLegSPtr leg2(new ComboLeg);
    leg2->conId = 9408;
    leg2->action = "SELL";
    leg2->ratio = 1;
    leg2->exchange = "SMART";

    contract.comboLegs.reset(new Contract::ComboLegList());
    contract.comboLegs->push_back(leg1);
    contract.comboLegs->push_back(leg2);
    //! [bagstkcontract]
    return contract;
}

    /*
     * CBOE Volatility Index Future combo contract
     * Leg 1: 195538625 - FUT expiring 2016/02/17
     * Leg 2: 197436571 - FUT expiring 2016/03/16
     */
Contract ContractsDefs::FutureComboContract(const QString & _symbol){
    //! [bagfutcontract]
    Contract contract;
    contract.symbol = "VIX";
    contract.secType = "BAG";
    contract.currency = "USD";
    contract.exchange = "CFE";

    ComboLegSPtr leg1(new ComboLeg);
    leg1->conId = 195538625;
    leg1->action = "BUY";
    leg1->ratio = 1;
    leg1->exchange = "CFE";

    ComboLegSPtr leg2(new ComboLeg);
    leg2->conId = 197436571;
    leg2->action = "SELL";
    leg2->ratio = 1;
    leg2->exchange = "CFE";

    contract.comboLegs.reset(new Contract::ComboLegList());
    contract.comboLegs->push_back(leg1);
    contract.comboLegs->push_back(leg2);
    //! [bagfutcontract]
    return contract;
}

Contract ContractsDefs::SmartFutureComboContract(const QString & _symbol){
    //! [smartfuturespread]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "WTI"; // WTI,COIL spread. Symbol can be defined as first leg symbol ("WTI") or currency ("USD").
    contract.secType = "BAG";
    contract.currency = "USD";
    contract.exchange = "SMART";

    ComboLegSPtr leg1(new ComboLeg);
    leg1->conId = 55928698; // WTI future June 2017
    leg1->action = "BUY";
    leg1->ratio = 1;
    leg1->exchange = "IPE";

    ComboLegSPtr leg2(new ComboLeg);
    leg2->conId = 55850663; // COIL future June 2017
    leg2->action = "SELL";
    leg2->ratio = 1;
    leg2->exchange = "IPE";

    contract.comboLegs.reset(new Contract::ComboLegList());
    contract.comboLegs->push_back(leg1);
    contract.comboLegs->push_back(leg2);
    //! [smartfuturespread]
    return contract;
}

Contract ContractsDefs::InterCmdtyFuturesContract(const QString & _symbol){
    //! [intcmdfutcontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "CL.BZ";
    contract.secType = "BAG";
    contract.currency = "USD";
    contract.exchange = "NYMEX";

    ComboLegSPtr leg1(new ComboLeg);
    leg1->conId = 47207310; //CL Dec'16 @NYMEX
    leg1->action = "BUY";
    leg1->ratio = 1;
    leg1->exchange = "NYMEX";

    ComboLegSPtr leg2(new ComboLeg);
    leg2->conId = 47195961; //BZ Dec'16 @NYMEX
    leg2->action = "SELL";
    leg2->ratio = 1;
    leg2->exchange = "NYMEX";

    contract.comboLegs.reset(new Contract::ComboLegList());
    contract.comboLegs->push_back(leg1);
    contract.comboLegs->push_back(leg2);
    //! [intcmdfutcontract]
    return contract;
}

Contract ContractsDefs::NewsFeedForQuery(const QString & _symbol)
{
    //! [newsfeedforquery]
    Contract contract;
    contract.secType = "NEWS";
    contract.exchange = "BRF"; //Briefing Trader
    //! [newsfeedforquery]
    return contract;
}

Contract ContractsDefs::BTbroadtapeNewsFeed(const QString & _symbol)
{
    //! [newscontractbt]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "BRF:BRF_ALL"; //BroadTape All News
    contract.secType = "NEWS";
    contract.exchange = "BRF"; //Briefing Trader
    //! [newscontractbt]
    return contract;
}

Contract ContractsDefs::BZbroadtapeNewsFeed(const QString & _symbol)
{
    //! [newscontractbz]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "BZ:BZ_ALL"; //BroadTape All News
    contract.secType = "NEWS";
    contract.exchange = "BZ"; //Benzinga Pro
    //! [newscontractbz]
    return contract;
}

Contract ContractsDefs::FLYbroadtapeNewsFeed(const QString & _symbol)
{
    //! [newscontractfly]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "FLY:FLY_ALL"; //BroadTape All News
    contract.secType = "NEWS";
    contract.exchange = "FLY"; //Fly on the Wall
                               //! [newscontractfly]
    return contract;
}

Contract ContractsDefs::MTbroadtapeNewsFeed(const QString & _symbol)
{
    //! [newscontractmt]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "MT:MT_ALL"; //BroadTape All News
    contract.secType = "NEWS";
    contract.exchange = "MT"; //Midnight Trader
    //! [newscontractmt]
    return contract;
}

Contract ContractsDefs::ContFut(const QString & _symbol)
{
    //! [continuousfuturescontract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "ES";
    contract.secType = "CONTFUT";
    contract.exchange = "GLOBEX";
    //! [continuousfuturescontract]
    return contract;
}

Contract ContractsDefs::ContAndExpiringFut(const QString & _symbol){
    //! [contandexpiringfut]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "ES";
    contract.secType = "FUT+CONTFUT";
    contract.exchange = "GLOBEX";
    //! [contandexpiringfut]
    return contract;
}

Contract ContractsDefs::JefferiesContract(const QString & _symbol){
    //! [jefferies_contract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "AAPL";
    contract.secType = "STK";
    contract.exchange = "JEFFALGO"; // must be direct-routed to JEFALGO
    contract.currency = "USD"; // only available for US stocks
    //! [jefferies_contract]
    return contract;
}

Contract ContractsDefs::CSFBContract(const QString & _symbol){
    //! [csfb_contract]
    Contract contract;
    contract.symbol = _symbol.toLocal8Bit().data(); // "IBKR";
    contract.secType = "STK";
    contract.exchange = "CSFBALGO";
    contract.currency = "USD";
    //! [csfb_contract]
    return contract;
}
