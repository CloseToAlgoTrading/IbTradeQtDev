/* Copyright (C) 2023 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"
#include "Decimal.h"

Decimal DecimalFunctions::add(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_add(decimal1, decimal2, 0, &flags);
}

Decimal DecimalFunctions::sub(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_sub(decimal1, decimal2, 0, &flags);
}

Decimal DecimalFunctions::mul(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_mul(decimal1, decimal2, 0, &flags);
}

Decimal DecimalFunctions::div(Decimal decimal1, Decimal decimal2) {
    unsigned int flags;
    return __bid64_div(decimal1, decimal2, 0, &flags);
}

double DecimalFunctions::decimalToDouble(Decimal decimal) {
    unsigned int flags;
    return __bid64_to_binary64(decimal, 0, &flags);
}

Decimal DecimalFunctions::doubleToDecimal(double d) {
    unsigned int flags;
    return __binary64_to_bid64(d, 0, &flags);
}

Decimal DecimalFunctions::stringToDecimal(std::string str) {
    unsigned int flags;
    if (str.compare(std::string{ "2147483647" }) == 0 || str.compare(std::string{ "9223372036854775807" }) == 0 || str.compare(std::string{ "1.7976931348623157E308" }) == 0) {
        str.clear();
    }
    return __bid64_from_string(const_cast<char*>(str.c_str()), 0, &flags);
}

std::string DecimalFunctions::decimalToString(Decimal value) {
    char buf[64];
    unsigned int flags;
    __bid64_to_string(buf, value, &flags); // convert Decimal value to string using bid64_to_string function
    return buf;
}

std::string DecimalFunctions::decimalStringToDisplay(Decimal value) {
    // convert string with scientific notation to string with decimal notation (e.g. +1E-2 to 0.01)
    std::string tempStr = decimalToString(value);

    if (tempStr.compare(std::string{ "+NaN" }) == 0 || tempStr.compare(std::string{ "-SNaN" }) == 0) {
        return ""; // if is invalid, then return empty string
    }

    int expPos = tempStr.find("E"); // find position of 'E' char (e.g. 2)
    if (expPos < 0) {
        return tempStr; // if 'E' char is missing, then return string as-is
    }
    std::string expStr = tempStr.substr(expPos); // extract exp string (e.g. E-2)

    // calculate exp
    int exp = 0;
    for (unsigned int i = 2; i < expStr.size(); i++) {
        exp = exp * 10 + (expStr[i] - '0');
    }
    if (expStr[1] == '-') {
        exp *= -1;
    }
    int numLength = tempStr.size() - expStr.size() - 1; // length of numbers substring

    // check sign
    bool isNegative = false;
    if (tempStr.substr(0, 1).compare(std::string{ "-" }) == 0) {
        isNegative = true;
    }

    std::string numbers = tempStr.substr(1, numLength); // extract numbers (e.g. 1)
    if (exp == 0) {
        return isNegative ? '-' + numbers : numbers; // if exp is zero, then return numbers as-is
    }

    std::string result = isNegative ? "-" : "";
    bool decPtAdded = false;

    // add zero(s) and decimal point
    for (int i = numLength; i <= (-exp); i++) {
        result += '0';
        if (i == numLength) {
            result += '.';
            decPtAdded = true;
        }
    }

    // add numbers and decimal point
    for (int i = 0; i < numLength; i++) {
        if (numLength - i == (-exp) && !decPtAdded) {
            result += '.';
        }
        result += numbers[i];
    }
    return result;
}