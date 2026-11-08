// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phegmoduletest.cpp -- entry point of the GModule backend test (phegmoduletest_main.c).
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>

extern "C" void phe_log(const char* aMsg);
extern "C" int phe_gmodule_test_main(void);

extern "C" void phe_log(const char* aMsg) { RDebug::Printf("%s", aMsg); }

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TInt rc = 0;
	TRAPD(err, rc = phe_gmodule_test_main());
	RDebug::Printf("[phegmoduletest] exit rc=%d err=%d", rc, err);
	delete cleanup;
	return rc;
	}
