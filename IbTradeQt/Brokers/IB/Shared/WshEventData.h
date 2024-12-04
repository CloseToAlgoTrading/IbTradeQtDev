/* Copyright (C) 2022 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_WSHEVENTDATA_H
#define TWS_API_CLIENT_WSHEVENTDATA_H

#include <string>

struct WshEventData
{
    int conId;
    std::string filter;
    bool fillWatchlist;
    bool fillPortfolio;
    bool fillCompetitors;
    std::string startDate;
    std::string endDate;
    int totalLimit;

	WshEventData(int conId_, bool fillWatchlist_, bool fillPortfolio_, bool fillCompetitors_, std::string startDate_, std::string endDate_, int totalLimit_)
	{
        this->conId = conId_;
        this->filter = "";
        this->fillWatchlist = fillWatchlist_;
        this->fillPortfolio = fillPortfolio_;
        this->fillCompetitors = fillCompetitors_;
        this->startDate = startDate_;
        this->endDate = endDate_;
        this->totalLimit = totalLimit_;
    }

    WshEventData(std::string filter_, bool fillWatchlist_, bool fillPortfolio_, bool fillCompetitors_, std::string startDate_, std::string endDate_, int totalLimit_)
    {
        this->conId = INT_MAX;
        this->filter = filter_;
        this->fillWatchlist = fillWatchlist_;
        this->fillPortfolio = fillPortfolio_;
        this->fillCompetitors = fillCompetitors_;
        this->startDate = startDate_;
        this->endDate = endDate_;
        this->totalLimit = totalLimit_;
    }
};

#endif // wsheventdata_def