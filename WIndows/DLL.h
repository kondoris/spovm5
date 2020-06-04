#pragma once

#include <stdio.h>
#include <windows.h>
#include <tchar.h>

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

extern "C" BOOL DLL_API asyncFileRead(HANDLE hFile, LPTSTR lpBuffer, DWORD dwBytesCount, OVERLAPPED & overlapped);

extern "C" void DLL_API asyncFileWrite(HANDLE hFile, LPTSTR lpBuffer, DWORD dwBytesCount, OVERLAPPED & overlapped);

extern "C" HANDLE DLL_API EventCreate(BOOL bManulaReset, BOOL bInitialState, LPCTSTR lpName);
