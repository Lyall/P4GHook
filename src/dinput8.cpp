#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#define PLUGIN_API extern "C" __declspec(dllexport)

typedef HRESULT(WINAPI* DirectInput8Create_ptr)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter);

DirectInput8Create_ptr DirectInput8Create_orig;

PLUGIN_API HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter)
{
	return DirectInput8Create_orig(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

HMODULE origModule = NULL;

bool dinput8_proxy_init()
{
	extern HMODULE ourModule;

	WCHAR modulePath[MAX_PATH] = { 0 };
	if (!GetSystemDirectoryW(modulePath, _countof(modulePath)))
		return false;

	WCHAR ourModulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(ourModule, ourModulePath, _countof(ourModulePath));

	WCHAR exeName[MAX_PATH] = { 0 };
	WCHAR extName[MAX_PATH] = { 0 };
	_wsplitpath_s(ourModulePath, NULL, NULL, NULL, NULL, exeName, MAX_PATH, extName, MAX_PATH);

	swprintf_s(modulePath, MAX_PATH, L"%ws\\%ws%ws", modulePath, exeName, extName);

	origModule = LoadLibraryW(modulePath);
	if (!origModule)
		return false;

	DirectInput8Create_orig = (DirectInput8Create_ptr)GetProcAddress(origModule, "DirectInput8Create");

	return true;
}