// Copyright The Phorepoc Authors
// SPDX-License-Identifier: BSD-2-Clause
//
// phegmoduleplugin.cpp -- a plug-in DLL for the GModule backend test: two exports, looked up by NAME through the symbol table generated from phegmoduleplugin.def (recipes/gen_symtab.py).
#include <e32std.h>

extern "C" EXPORT_C int plugin_add(int a, int b)
	{
	return a + b;
	}

extern "C" EXPORT_C const char* plugin_name(void)
	{
	return "phegmoduleplugin";
	}

GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}
