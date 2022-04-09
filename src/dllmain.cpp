#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include "external/inih/INIReader.h"

bool dinput8_proxy_init();

using namespace std;

HMODULE ourModule = 0;
HMODULE baseModule = GetModuleHandle(NULL);

// Ini variables
bool bCRTEffects;
bool bCustomResolution;
int iCustomResX;
int iCustomResY;
bool bSkipIntro;
bool bCenteredUI;

// Variables
float originalAspect = 1.777777791f;
float aspectMulti;
float newAspect;
int resScale;
float resScaleMulti;

bool Hook(void* hookAddress, void* hookFunction, int hookLength) {
	if (hookLength < 5) {
		return false;
	}

	DWORD oldProtect;
	VirtualProtect(hookAddress, hookLength, PAGE_EXECUTE_WRITECOPY, &oldProtect);

	memset(hookAddress, 0x90, hookLength);

	DWORD relativeAddress = ((DWORD)hookFunction - (DWORD)hookAddress) - 5;

	*(BYTE*)hookAddress = 0xE9;
	*(DWORD*)((DWORD)hookAddress + 1) = relativeAddress;

	DWORD tmp;
	VirtualProtect(hookAddress, hookLength, oldProtect, &tmp);

	return true;
}

template<typename T>
void WriteMemory(DWORD writeAddress, T value)
{
	DWORD oldProtect;
	VirtualProtect((LPVOID)(writeAddress), sizeof(T), PAGE_EXECUTE_WRITECOPY, &oldProtect);
	*(reinterpret_cast<T*>(writeAddress)) = value;
	VirtualProtect((LPVOID)(writeAddress), sizeof(T), oldProtect, &oldProtect);
}

DWORD UIOffsetReturnJMP;
float UIOffsetValue;
void __declspec(naked) UIOffset_CC()
{
	__asm
	{
		subss xmm4, xmm0
		addss xmm0, xmm0
		movss xmm4, [UIOffsetValue]
		jmp[UIOffsetReturnJMP]
	}
}

void ReadIni()
{
	INIReader config("P4GHook.ini");

	bCRTEffects = config.GetBoolean("Remove CRT Effects", "Enabled", true);
	bCustomResolution = config.GetBoolean("Custom Resolution", "Enabled", true);
	iCustomResX = config.GetInteger("Custom Resolution", "Width", -1);
	iCustomResY = config.GetInteger("Custom Resolution", "Height", -1);
	bSkipIntro = config.GetBoolean("Skip Intro", "Enabled", true);
	bCenteredUI = config.GetBoolean("Centered UI", "Enabled", true);
}


void ChangeResolutions()
{
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	
	if (bCustomResolution && desktop.bottom >= 4320)
	{
		// 7680x4320
		WriteMemory(0xA66C10, iCustomResX);
		WriteMemory(0xA66C14, iCustomResY);
	}
	else if (bCustomResolution && desktop.bottom >= 2880)
	{
		// 5120x2880
		WriteMemory(0xA66C08, iCustomResX);
		WriteMemory(0xA66C0C, iCustomResY);
	}
	else if (bCustomResolution && desktop.bottom >= 2160)
	{
		// 3840x2160
		WriteMemory(0xA66C00, iCustomResX);
		WriteMemory(0xA66C04, iCustomResY);
	}
	else if (bCustomResolution && desktop.bottom >= 1440)
	{
		// 2560x1440
		WriteMemory(0xA66BF8, iCustomResX);
		WriteMemory(0xA66BFC, iCustomResY);
	}
	else if (bCustomResolution && desktop.bottom >= 1080)
	{
		// 1920x1080
		WriteMemory(0xA66BF0, iCustomResX);
		WriteMemory(0xA66BF4, iCustomResY);
	}
	else if (bCustomResolution && desktop.bottom >= 900)
	{
		// 1600x900
		WriteMemory(0xA66BE8, iCustomResX);
		WriteMemory(0xA66BEC, iCustomResY);
	}
	
}

void SkipIntro()
{
	if (bSkipIntro) {
		// https://github.com/zarroboogs/p4gpc.tinyfixes/blob/master/tinyfixes/Fixes/IntroSkipPatch.cs
		// "5E 5B 5D C3 B9 01 00 00 00"
		memcpy((LPVOID)((intptr_t)baseModule + 0x2535E1C8), "\x5E\x5B\x5D\xC3\xE9\x30\x00\x00\x00", 9);
	}
}

void AspectRatio()
{
	if (bCustomResolution) {
		float newAspect = (float)iCustomResX / iCustomResY;
		float aspectMulti = newAspect / originalAspect;

		// Aspect ratio
		WriteMemory(0x99A970, newAspect);

		// UI reference resolution
		WriteMemory(0x99BC50, 1920 * aspectMulti);
	}
}

void CRTEffects()
{
	if (bCRTEffects) {
		// TV Static
		// "B4 BB 99 00 66 0F 6E C0 F3 0F"
		memcpy((LPVOID)((intptr_t)baseModule + 0x2467DFB2), "\x90\x90\x90\x90", 4);

		// TV Scanlines
		// "BB FF FF FF 3C" 
		memcpy((LPVOID)((intptr_t)baseModule + 0x24680481), "\xBB\x00\x00\x00\x00", 5);
	}
}

DWORD __stdcall CenteredUI(void*)
{
	for (;;) // forever loop so that it updates the UI offset if the user changes resolution scale
	{
		// 0x60FB04 = Res scale (e.g 150)
		resScale = *(int32_t*)(0xA0FB04);
		resScaleMulti = resScale / (float)100;

		newAspect = (float)iCustomResX / iCustomResY;
		aspectMulti = newAspect / originalAspect;

		int UIOffsetHookLength = 8;
		DWORD UIOffsetAddress = 0x27CC0BF5;
		UIOffsetValue = (float)((iCustomResX - (iCustomResX / aspectMulti)) / 2) * resScaleMulti; // There has to be a better way to calculate the offset
		UIOffsetReturnJMP = UIOffsetAddress + UIOffsetHookLength;
		Hook((void*)UIOffsetAddress, UIOffset_CC, UIOffsetHookLength);

		// P4G.exe+27A57177 - C7 05 5C75A600 00007044 - mov [P4G.exe+66755C],44700000
		WriteMemory(0x27E5717D, (float)960 * aspectMulti);

		// P4G.exe+277C4570 - C7 83 E0000000 00007044 - mov [ebx+000000E0],44700000
		WriteMemory(0x27BC4576, (float)960 * aspectMulti);

		Sleep(1000); // run every second
	}
	return true;
}

void Main()
{
	ReadIni();
	ChangeResolutions();
	SkipIntro();
	AspectRatio();
	CRTEffects();
	if (bCenteredUI)
	{
		CreateThread(NULL, 0, CenteredUI, 0, NULL, 0); // This seems like a bad solution
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		ourModule = hModule;
		dinput8_proxy_init();
		Main();
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

