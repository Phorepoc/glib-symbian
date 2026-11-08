// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phegmodule_rlib.cpp -- the RLibrary half of the GModule backend (overlay/phe_gmodule_symbian.c): load a DLL by name, look an export up by ordinal, close it.
#include <e32std.h>
#include <string.h>

extern "C" int phe_lib_load(const char* aName, void** aHandle)
	{
	TBuf<128> name;
	name.Copy(TPtrC8((const TUint8*)aName, strlen(aName)));
	RLibrary* lib = new RLibrary;
	if (!lib)
		return KErrNoMemory;
	TInt r = lib->Load(name);
	if (r != KErrNone)
		{
		delete lib;
		return r;
		}
	*aHandle = lib;
	return KErrNone;
	}

extern "C" void* phe_lib_ordinal(void* aHandle, int aOrdinal)
	{
	return (void*)static_cast<RLibrary*>(aHandle)->Lookup(aOrdinal);
	}

extern "C" void phe_lib_close(void* aHandle)
	{
	RLibrary* lib = static_cast<RLibrary*>(aHandle);
	lib->Close();
	delete lib;
	}
