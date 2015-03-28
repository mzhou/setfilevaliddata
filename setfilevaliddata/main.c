#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

static void PrintLastError(LPCTSTR prefix);

int _tmain(int argc, _TCHAR *argv[])
{
	if (argc != 3) {
		_ftprintf(stderr, _T("Usage: %s <file> <size>\n"), argv[0]);
		return 1;
	}

	_TCHAR *endPtr;
	LONGLONG desiredLength = _tcstoll(argv[2], &endPtr, 10);
	if (endPtr[0] != '\0') {
		_ftprintf(stderr, _T("Invalid size\n"));
		return 2;
	}

	HANDLE hToken;
	BOOL openResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (!openResult) {
		PrintLastError(_T("OpenProcessToken"));
		return 3;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	BOOL lookupResult = LookupPrivilegeValue(NULL, SE_MANAGE_VOLUME_NAME, &tp.Privileges[0].Luid);
	if (!lookupResult) {
		PrintLastError(_T("LookupPrivilegeValue"));
		return 4;
	}

	BOOL adjustResult = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
	if (!adjustResult) {
		PrintLastError(_T("AdjustTokenPrivileges"));
		return 5;
	}

	HANDLE hFile = CreateFile(
		argv[1],
		GENERIC_WRITE,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		PrintLastError(_T("CreateFile"));
		return 6;
	}

	LARGE_INTEGER distanceToMove = { .QuadPart = desiredLength };
	BOOL pointerResult = SetFilePointerEx(hFile, distanceToMove, NULL, FILE_BEGIN);
	if (!pointerResult) {
		PrintLastError(_T("SetFilePointerEx"));
		return 7;
	}

	BOOL endOfFileResult = SetEndOfFile(hFile);
	if (!endOfFileResult) {
		PrintLastError(_T("SetEndOfFile"));
		return 8;
	}

	BOOL validDataResult = SetFileValidData(hFile, desiredLength);
	if (!validDataResult) {
		PrintLastError(_T("SetFileValidData"));
		return 9;
	}

	return 0;
}

static void PrintLastError(LPCTSTR prefix)
{
	DWORD lastError = GetLastError();
	LPTSTR buf = NULL;
	DWORD msgSize = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		lastError,
		LANG_NEUTRAL,
		(LPTSTR)&buf,
		0,
		NULL);
	if (!buf || !msgSize) {
		return;
	}
	_ftprintf(stderr, _T("%s: %u: %s\n"), prefix, lastError, buf);
}