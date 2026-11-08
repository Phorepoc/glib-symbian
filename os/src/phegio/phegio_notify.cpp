// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phegio_notify.cpp -- the platform half of the GIO file monitor (overlay/phe_filemonitor.c): a thread that waits for the file server to report a change below a directory
// (RFs::NotifyChange) and calls back. Stopping wakes the thread with RThread::RequestComplete (a file server session may only be used by the thread that connected it), the thread cancels
// its outstanding request and ends, and phe_notify_stop waits for it, so no callback arrives after phe_notify_stop returns.
#include <e32std.h>
#include <f32file.h>
#include <pthread.h>
#include <string.h>

struct TWatch
	{
	pthread_t iThread;
	volatile int iReady;
	volatile int iDone;
	TThreadId iTid;
	TRequestStatus* iStopPtr;
	void (*iCb)(void*);
	void* iUser;
	TBuf<256> iPath;
	};

static void* WatchThread(void* aArg)
	{
	TWatch* w = static_cast<TWatch*>(aArg);
	RFs fs;
	TRequestStatus stop(KRequestPending);
	if (fs.Connect() == KErrNone)
		{
		w->iTid = RThread().Id();
		w->iStopPtr = &stop;
		w->iReady = 1;
		for (;;)
			{
			TRequestStatus st;
			fs.NotifyChange(ENotifyAll, st, w->iPath);
			User::WaitForAnyRequest();                                // the file server reported a change, or phe_notify_stop completed `stop`
			if (stop != KRequestPending)
				{
				fs.NotifyChangeCancel();
				User::WaitForRequest(st);                             // the cancelled request completes too
				break;
				}
			w->iCb(w->iUser);
			}
		fs.Close();
		}
	else
		w->iReady = 1;
	w->iDone = 1;
	return NULL;
	}

extern "C" void* phe_notify_start(const char* aDir, void (*aCb)(void*), void* aUser)
	{
	TWatch* w = new TWatch;
	if (!w)
		return NULL;
	w->iReady = 0; w->iDone = 0; w->iStopPtr = NULL; w->iCb = aCb; w->iUser = aUser;
	TBuf8<256> d8;
	d8.Copy(TPtrC8((const TUint8*)aDir, Min((TInt)strlen(aDir), 254)));
	w->iPath.Copy(d8);
	for (TInt i = 0; i < w->iPath.Length(); i++)
		if (w->iPath[i] == '/')
			w->iPath[i] = '\\';
	if (w->iPath.Length() == 0 || w->iPath[w->iPath.Length() - 1] != '\\')
		w->iPath.Append('\\');
	pthread_attr_t at;
	pthread_attr_init(&at);
	pthread_attr_setstacksize(&at, 0x10000);
	if (pthread_create(&w->iThread, &at, WatchThread, w) != 0)
		{
		delete w;
		return NULL;
		}
	return w;
	}

extern "C" void phe_notify_stop(void* aHandle)
	{
	TWatch* w = static_cast<TWatch*>(aHandle);
	if (!w)
		return;
	while (!w->iReady)
		User::After(2000);
	if (!w->iDone && w->iStopPtr)
		{
		RThread th;
		if (th.Open(w->iTid) == KErrNone)
			{
			th.RequestComplete(w->iStopPtr, KErrNone);
			th.Close();
			}
		}
	pthread_join(w->iThread, NULL);
	delete w;
	}
