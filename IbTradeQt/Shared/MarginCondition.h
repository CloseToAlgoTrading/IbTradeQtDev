/* Copyright (C) 2018 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_MARGINCONDITION_H
#define TWS_API_CLIENT_MARGINCONDITION_H

#include "standartincludes.h"
#include "OperatorCondition.h"

class TWSAPIDLLEXP MarginCondition : public OperatorCondition {
	friend OrderCondition;

	int m_percent;

public:
	static const OrderConditionType conditionType = OrderConditionType::Margin;

protected:
	MarginCondition() { }

	virtual std::string valueToString() const;
	virtual void valueFromString(const std::string &v);

public:
	virtual std::string toString();

	int percent();
	void percent(int percent);
};

#endif
