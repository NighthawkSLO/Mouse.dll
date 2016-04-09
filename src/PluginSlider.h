/*
Copyright (C) 2016 NighthawkSLO <jon.kuhar99@gmail.com>

This work is licensed under the Creative Commons Attribution 4.0 International License.
To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or
send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*/

#ifndef __PLUGIN_HOTKEY_H__
#define __PLUGIN_HOTKEY_H__

#include "Stdafx.h" // all other includes

struct Measure
{
	std::wstring Type; // what will be read from skin
	std::wstring PluginAction;

	int PluginX;
	int PluginY;
	int PluginW;
	int PluginH;

	int Value;

	bool OutOfBoundsX;
	bool OutOfBoundsY;

	void* skin;
	void* rm;

	Measure() :
		Type(), PluginAction(),

		PluginX(), PluginY(), PluginW(), PluginH(),

		Value(),

		OutOfBoundsX(TRUE), OutOfBoundsY(TRUE),

		skin(), rm()
	{ }
};

static std::vector<Measure*> g_Measures; // "array with an exception that it automatically handles its own storage requirements in case it grows"
static HINSTANCE g_Instance = nullptr;
static HHOOK g_Hook = nullptr;
static bool g_IsHookActive = false;

void RemoveMeasure(Measure* measure); // definitions for 2 functions
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam); // definition for the keyboard hook

LPCWSTR g_ErrEmpty = L"Invalid Type: %s"; // standard errors
LPCWSTR g_ErrHook = L"Could not %s the keyboard hook.";

#endif
