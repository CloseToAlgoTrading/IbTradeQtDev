/* Copyright (C) 2018 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef TWS_API_CLIENT_EREADEROSSIGNAL_H
#define TWS_API_CLIENT_EREADEROSSIGNAL_H

#include "standartincludes.h"
//#include "StdAfx.h"
#include "EReaderSignal.h"
#include <stdexcept>

#if !defined(INFINITE)
#define INFINITE ((unsigned long)-1)
#endif

class TWSAPIDLLEXP EReaderOSSignal :
	public EReaderSignal
{
#if defined(IB_POSIX)
    pthread_condattr_t m_condattr;
    pthread_cond_t m_evMsgs;
    pthread_mutex_t m_mutex;
    bool open;
#elif defined(IB_WIN32)
	HANDLE m_evMsgs;
#else
#   error "Not implemented on this platform"
#endif
    unsigned long m_waitTimeout; // in milliseconds

public:
	EReaderOSSignal(unsigned long waitTimeout = INFINITE);
	virtual ~EReaderOSSignal(void);

	virtual void issueSignal();
	virtual void waitForSignal();
};

#endif
