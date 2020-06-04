// Dll.cpp : Îïðåäåëÿåò ýêñïîðòèðîâàííûå ôóíêöèè äëÿ ïðèëîæåíèÿ DLL.
//
#include "pch.h"
#include "DLL.h"

HANDLE EventCreate(BOOL bManulaReset, BOOL bInitialState, LPCTSTR lpName)
{
	HANDLE hEvent = CreateEvent(NULL, bManulaReset, bInitialState, lpName);
	if (!hEvent)
	{
		_ftprintf(stderr, TEXT("Can't create event! Error: %d\n"), GetLastError());
		return NULL;
	}

	return hEvent;
}

BOOL asyncFileRead(HANDLE hFile, LPTSTR lpBuffer, DWORD dwBytesCount, OVERLAPPED& overlapped)
{
	HANDLE hEvent = EventCreate(FALSE, FALSE, TEXT("EVENT"));
	overlapped.hEvent = hEvent;

	DWORD dwBytesRead;
	BOOL bResult = ReadFile(hFile, lpBuffer, dwBytesCount, &dwBytesRead, &overlapped);

	DWORD dwLastError = GetLastError();

	if (!bResult && dwLastError == ERROR_HANDLE_EOF)
	{
		CloseHandle(hEvent);
		return TRUE;
	}

	if (!bResult && dwLastError == ERROR_IO_PENDING)
	{
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);

		if (overlapped.Internal == ERROR_HANDLE_EOF || overlapped.InternalHigh < dwBytesCount)
		{
			return TRUE;
		}
	}
	else
	{
		CloseHandle(hEvent);
	}

	return FALSE;
}

void asyncFileWrite(HANDLE hFile, LPTSTR lpBuffer, DWORD dwBytesCount, OVERLAPPED& overlapped)
{
	HANDLE hEvent = EventCreate(FALSE, FALSE, TEXT("EVENT"));
	overlapped.hEvent = hEvent;

	DWORD dwBytesWritten;
	BOOL bResult = WriteFile(hFile, lpBuffer, dwBytesCount, &dwBytesWritten, &overlapped);

	DWORD dwLastError = GetLastError();

	if (!bResult && dwLastError == ERROR_IO_PENDING)
	{
		WaitForSingleObject(hEvent, INFINITE);
	}

	CloseHandle(hEvent);
}
