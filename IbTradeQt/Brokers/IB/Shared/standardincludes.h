#ifndef STANDARDINCLUDES_H
#define STANDARDINCLUDES_H

#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <QtGlobal>

#ifndef TWSAPIDLLEXP
#define TWSAPIDLLEXP
#endif

#ifdef Q_OS_WIN
#define IB_WIN32
#define atoll _atoi64
#endif


#endif // STANDARTINCLUDES_H
