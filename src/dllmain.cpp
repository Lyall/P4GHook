#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>

// proxy.cpp
bool Proxy_Attach();
void Proxy_Detach();

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

float originalAspect = 1.777777791f;
float newAspect;
float aspectMulti;

template<typename T>
void WriteMemory(DWORD writeAddress, T value)
{
	DWORD oldProtect;
	VirtualProtect((LPVOID)(writeAddress), sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
	*(reinterpret_cast<T*>(writeAddress)) = value;
	VirtualProtect((LPVOID)(writeAddress), sizeof(T), oldProtect, &oldProtect);
}


void AspectRatio()
{
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);

	float newAspect = (float)desktop.right / (float)desktop.bottom;
	float aspectMulti = newAspect / originalAspect;

	// Aspect ratio
	WriteMemory(0x99A970, newAspect);

	// UI reference resolution
	WriteMemory(0x99BC50, 1920 * aspectMulti);
}

void SkipIntro()
{
	// https://github.com/zarroboogs/p4gpc.tinyfixes/blob/master/tinyfixes/Fixes/IntroSkipPatch.cs
	// "5E 5B 5D C3 B9 01 00 00 00"
	memcpy((LPVOID)((intptr_t)baseModule + 0x2535E1C8), "\x5E\x5B\x5D\xC3\xE9\x30\x00\x00\x00", 9);
}

void ChangeResolutions()
{
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);

	// 2560x1440 Windowed
	WriteMemory(0xA66BC8, (int32_t)desktop.right);
	WriteMemory(0xA66BCC, (int32_t)desktop.bottom);

	// 2560x1440 Fullscreen/Borderless
	WriteMemory(0xA66BF8, (int32_t)desktop.right);
	WriteMemory(0xA66BFC, (int32_t)desktop.bottom);

}

void CRTEffects()
{
	// TV Static
	// "B4 BB 99 00 66 0F 6E C0 F3 0F"
	memcpy((LPVOID)((intptr_t)baseModule + 0x2467DFB2), "\x90\x90\x90\x90", 4);

	// TV Scanlines
	// "BB FF FF FF 3C" 
	memcpy((LPVOID)((intptr_t)baseModule + 0x24680481), "\xBB\x00\x00\x00\x00", 5);
}

void Patch_Init()
{
	ChangeResolutions();
	SkipIntro();
	AspectRatio();
	CRTEffects();
	Sleep(5000);
}

void Patch_Uninit()
{

}

HMODULE ourModule = 0;

BOOL APIENTRY DllMain(HMODULE hModule, int ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		ourModule = hModule;
		Proxy_Attach();

		Patch_Init();
	}
	if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		Patch_Uninit();

		Proxy_Detach();
	}

	return TRUE;
}
