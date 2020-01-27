#ifndef STANDARTINCLUDES_H
#define STANDARTINCLUDES_H

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


//typedef void *PVOID;
//typedef PVOID HANDLE;
//#ifndef HWND
//typedef HANDLE HWND;
//#endif


#endif // STANDARTINCLUDES_H
