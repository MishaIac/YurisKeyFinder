#include "util.h"
#include "util_detours.h"
#include <intrin.h>
#include <iostream>
#include <iomanip>
#include <windows.h>
using std::cout;
using std::endl;
using std::hex;
#pragma warning(disable:4311)
#pragma warning(disable:4312)  
#pragma warning(disable:4996) 

bool MemoryCompare(const byte* data, const byte* datamask, const char* mask)
{
	for (; *mask; ++data, ++datamask, ++mask)
	{
		if (!strcmp(mask, "xxxx"))
		{
			if (*(UINT32*)data != *(UINT32*)datamask)
			{
				return FALSE;
			}

			data += 3, datamask += 3, mask += 3;
			continue;
		}

		if (!strcmp(mask, "xx"))
		{
			if (*(UINT16*)data != *(UINT16*)datamask)
			{
				return FALSE;
			}

			data++, datamask++, mask++;
			continue;
		}

		if (*mask == 'x' && *data != *datamask)
		{
			return false;
		}
	}

	return (*mask) == 0;
}

UINT_PTR FindMemoryPattern(const char* mask, byte* datamask, UINT_PTR start, UINT_PTR length)
{
	UINT_PTR end = start + length;
	for (UINT_PTR i = start; i < end; i++)
	{
		if (MemoryCompare((byte*)i, datamask, mask))
		{
			return i;
		}
	}
	return 0;
}

template <typename I> std::string n2hexstr(I w, size_t hex_len = 2) {
	static const char* digits = "0123456789ABCDEF";
	std::string rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

BOOL HookGame()
{
	byte patternTexture60[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	UINT_PTR ptrTexture1 = FindMemoryPattern("xxxx????xxxx????????????????????????????xxxxxxxxxxxxxxxx", patternTexture60, 0x00750000, 0x125000);

	if (ptrTexture1 != 0)
	{
		int addr = ptrTexture1 + 4;
		cout << "Address: " << (int*)addr << endl;
		BYTE* b = (BYTE*)(UINT_PTR)ptrTexture1;
		cout << "XOR Key: ";
		for (int i = 4; i < 8; ++i)
			cout << n2hexstr((int)b[i]);
	}
	else
	{
		cout << "Please, close the game, to get XOR Key" << endl;
	}
	return TRUE;
}

static void make_console() {
	AllocConsole();
	std::ignore = freopen("CONOUT$", "w", stdout);
	std::ignore = freopen("CONIN$", "r", stdin);
	cout << "Yu-ris Key Finder by TotSamiyMisha v1.01 with Proxy" << endl;
}

#pragma region Proxy
struct dsound_dll {
	HMODULE dll;
	FARPROC oDirectSoundCreate;
	FARPROC oDirectSoundEnumerateA;
	FARPROC oDirectSoundEnumerateW;
} dsound;

extern "C" void __declspec(naked) fDirectSoundCreate(void)
{
	__asm jmp dsound.oDirectSoundCreate;
}

extern "C" void __declspec(naked) fDirectSoundEnumerateA(void)
{
	__asm jmp dsound.oDirectSoundEnumerateA;
}

extern "C" void __declspec(naked) fDirectSoundEnumerateW(void)
{
	__asm jmp dsound.oDirectSoundEnumerateW;
}

void setupFunctions() {
	dsound.oDirectSoundCreate = GetProcAddress(dsound.dll, "DirectSoundCreate");
	dsound.oDirectSoundEnumerateA = GetProcAddress(dsound.dll, "DirectSoundEnumerateA");
	dsound.oDirectSoundEnumerateW = GetProcAddress(dsound.dll, "DirectSoundEnumerateW");
}
#pragma endregion

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		wchar_t* SysDirPath = new wchar_t[MAX_PATH];
		GetSystemDirectoryW(SysDirPath, MAX_PATH);

		wchar_t* DSOUND_Path = new wchar_t[MAX_PATH + 12];
		wsprintfW(DSOUND_Path, L"%s\\DSOUND.dll", SysDirPath);
		dsound.dll = LoadLibraryW(DSOUND_Path);
		setupFunctions();

		make_console();

		return HookGame();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		HookGame();
		MessageBoxW(NULL, L"XOR Key found", L"Info", MB_OK);
	}
	return TRUE;
}