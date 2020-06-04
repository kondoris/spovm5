#include <iostream>
#include <windows.h>
#include <tchar.h>


DWORD WINAPI WriterThreadProc(LPVOID lpParam);

LPTSTR concatenate(LPCTSTR lpFirst, LPCTSTR lpSecond);

LPTSTR getFullPath(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory);

LPTSTR formPathForSearhing(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory);

#define BUF_SIZE 1024

HANDLE hReadEvent = NULL;
HANDLE hWriteEvent = NULL;
HANDLE hTerminateEvent = NULL;

TCHAR lpBuffer[BUF_SIZE];

extern "C" typedef HANDLE(*lpfnDllCreateEvent)(BOOL, BOOL, LPCTSTR);
extern "C" typedef BOOL(*lpfnDllAsyncFileRead)(HANDLE, LPTSTR, DWORD, OVERLAPPED&);
extern "C" typedef void (*lpfnDllAsyncFileWrite)(HANDLE, LPTSTR, DWORD, OVERLAPPED&);

lpfnDllCreateEvent EventCreate = NULL;
lpfnDllAsyncFileRead asyncFileRead = NULL;
lpfnDllAsyncFileWrite asyncFileWrite = NULL;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3) // argv[1] - directory with files to concatenate, argv[2] - output file name
	{
		_ftprintf(stdout, TEXT("Invalid input!\n"));
		return 1;
	}

	HINSTANCE hDll;
	if (!(hDll = LoadLibrary(TEXT("D:\\4sem\\ÑÏÎÂÌ\\Dll\\Debug\\Dll.dll"))))
	{
		_ftprintf(stderr, TEXT("Can't load DLL\n"));
		system("pause");
		return 1;
	}

	EventCreate = (lpfnDllCreateEvent)GetProcAddress(hDll, TEXT("EventCreate")); //íàõîäèì ôóíêöèþ â äèíàìè÷åñêîé áèáëèîòåêå
	asyncFileRead = (lpfnDllAsyncFileRead)GetProcAddress(hDll, TEXT("asyncFileRead"));
	asyncFileWrite = (lpfnDllAsyncFileWrite)GetProcAddress(hDll, TEXT("asyncFileWrite"));
	if (!EventCreate || !asyncFileRead || !asyncFileWrite)
	{
		_ftprintf(stderr, TEXT("Can't load DLL function!\n"));
		FreeLibrary(hDll);
		system("pause");
		return 1;
	}

	hReadEvent = EventCreate(TRUE, FALSE, TEXT("ReadEvent"));
	hWriteEvent = EventCreate(TRUE, FALSE, TEXT("WriteEvent"));
	hTerminateEvent = EventCreate(FALSE, FALSE, TEXT("TerminateEvent"));

	WIN32_FIND_DATA ffd;
	HANDLE hFind;

	TCHAR lpCurrentDirectory[MAX_PATH]; //ïóòü ê òåêóùåé äèðåêòîðèè
	GetCurrentDirectory(MAX_PATH, lpCurrentDirectory);

	HANDLE hOutputFile = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (!hOutputFile)
	{
		_ftprintf(stderr, TEXT("Can't create output file\n"));
		SetCurrentDirectory(lpCurrentDirectory);
		FreeLibrary(hDll);
		return 1;
	}

	SetCurrentDirectory(argv[1]);

	HANDLE hWriterThread = CreateThread(NULL, 0, WriterThreadProc, hOutputFile, 0, NULL);
	if (!hWriterThread)
	{
		SetCurrentDirectory(lpCurrentDirectory);
		_ftprintf(stderr, TEXT("Can't create a thread! Error: %d\n"), GetLastError());
		FreeLibrary(hDll);
		return 1;
	}

	LPTSTR lpFindPath = formPathForSearhing(lpCurrentDirectory, argv[1]);

	hFind = FindFirstFile(lpFindPath, &ffd);
	delete lpFindPath;

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (_tcscmp(ffd.cFileName, TEXT(".")) && _tcscmp(ffd.cFileName, TEXT("..")))
			{
				HANDLE hFile = CreateFile(ffd.cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
				BOOL bEof = FALSE;
				OVERLAPPED overlapped = { 0 };
				ULONG ulBytesRead = 0;

				do
				{
					ZeroMemory(&overlapped, sizeof(OVERLAPPED));
					overlapped.Offset = ulBytesRead;

					if (bEof = asyncFileRead(hFile, lpBuffer, BUF_SIZE, overlapped))
					{
						lpBuffer[overlapped.InternalHigh] = '\0';
					}

					ulBytesRead += overlapped.InternalHigh;

					SetEvent(hReadEvent);
					WaitForSingleObject(hWriteEvent, INFINITE);
					ResetEvent(hWriteEvent);
				} while (!bEof);

				CloseHandle(hFile);
			}

		} while (FindNextFile(hFind, &ffd));

		SetEvent(hTerminateEvent);
	}

	FindClose(hFind);

	SetCurrentDirectory(lpCurrentDirectory);

	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);
	CloseHandle(hTerminateEvent);
	FreeLibrary(hDll);

	system("pause");
	return 0;
}

DWORD WINAPI WriterThreadProc(LPVOID lpParam)
{
	HANDLE hEvents[] = { hTerminateEvent, hReadEvent };
	HANDLE hOutputFile = lpParam;
	OVERLAPPED overlapped = { 0 };
	ULONG ulBytesWritten = 0;
	DWORD dwNumberOfBytesToWrite = 0;

	while (1)
	{
		DWORD dwWaitResult = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			return 0;
		case WAIT_OBJECT_0 + 1:
			ZeroMemory(&overlapped, sizeof(OVERLAPPED));
			overlapped.Offset = ulBytesWritten;

			_tcslen(lpBuffer) > BUF_SIZE ? dwNumberOfBytesToWrite = BUF_SIZE : dwNumberOfBytesToWrite = _tcslen(lpBuffer);

			asyncFileWrite(hOutputFile, lpBuffer, dwNumberOfBytesToWrite, overlapped);

			ulBytesWritten += overlapped.InternalHigh;

			ResetEvent(hReadEvent);
			SetEvent(hWriteEvent);
			break;
		default:
			_ftprintf(stderr, TEXT("Wait error! Error: %d\n"), GetLastError());
			return 1;
		}
	}
}

LPTSTR concatenate(LPCTSTR lpFirst, LPCTSTR lpSecond)
{
	LPTSTR lpNew = new TCHAR[_tcsclen(lpFirst) + _tcsclen(lpSecond) + 1];
	_tcsnccpy_s(lpNew, _tcslen(lpFirst) + 1, lpFirst, _tcslen(lpFirst) + 1);
	_tcscat_s(lpNew, _tcsclen(lpFirst) + _tcsclen(lpSecond) + 1, lpSecond);

	return lpNew;
}

LPTSTR getFullPath(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory)
{
	LPTSTR lpInbetween = concatenate(lpParentDirectory, TEXT("\\"));
	LPTSTR lpFullPath = concatenate(lpInbetween, lpChildDirectory);
	delete lpInbetween;

	return lpFullPath;
}

LPTSTR formPathForSearhing(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory)
{
	LPTSTR lpFullPath = getFullPath(lpParentDirectory, lpChildDirectory);
	LPTSTR lpPathForSearhing = concatenate(lpFullPath, TEXT("\\*"));
	delete lpFullPath;

	return lpPathForSearhing;
}
