#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "bisect.h"

char* realpath(const char* path, char* resolved_path) {
	if (GetFullPathNameA(path, MAX_PATH_LENGTH, resolved_path, NULL) == 0) {
		return NULL;
	}
	return resolved_path;
}

char* strptime(const char* s, const char* format, struct tm* tm) {
	SYSTEMTIME st;
	if (sscanf(s, format, &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute, &st.wSecond) != 6) {
		return NULL;
	}
	tm->tm_year = st.wYear - 1900;
	tm->tm_mon = st.wMonth - 1;
	tm->tm_mday = st.wDay;
	tm->tm_hour = st.wHour;
	tm->tm_min = st.wMinute;
	tm->tm_sec = st.wSecond;
	tm->tm_isdst = -1; // Let mktime determine DST
	return (char*)s + strlen(s);
}
#endif //_WIN32 || _WIN64
