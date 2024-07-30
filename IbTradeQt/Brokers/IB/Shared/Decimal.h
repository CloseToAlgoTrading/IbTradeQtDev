/* Copyright (C) 2023 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_DECIMAL_H
#define TWS_API_CLIENT_DECIMAL_H

#include <string>
#include "limits.h"
// Decimal type
typedef unsigned long long Decimal;

#define UNSET_DECIMAL ULLONG_MAX

//// external functions
extern "C" Decimal __bid64_add(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_sub(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_mul(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_div(Decimal, Decimal, unsigned int, unsigned int*);
extern "C" Decimal __bid64_from_string(char*, unsigned int, unsigned int*);
extern "C" void __bid64_to_string(char*, Decimal, unsigned int*);
extern "C" double __bid64_to_binary64(Decimal, unsigned int, unsigned int*);
extern "C" Decimal __binary64_to_bid64(double, unsigned int, unsigned int*);

class DecimalFunctions {

public:
    static Decimal add(Decimal decimal1, Decimal decimal2);
    static Decimal sub(Decimal decimal1, Decimal decimal2);
    static Decimal mul(Decimal decimal1, Decimal decimal2);
    static Decimal div(Decimal decimal1, Decimal decimal2);
    static double decimalToDouble(Decimal decimal);
    static Decimal doubleToDecimal(double d);
    static Decimal stringToDecimal(std::string str);
    static std::string decimalToString(Decimal value);
    static std::string decimalStringToDisplay(Decimal value);
};

#endif
