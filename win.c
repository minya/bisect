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
	if (s == NULL || format == NULL || tm == NULL) {
		return NULL;
	}

	int year, month, day, hour, minute, second;
	char* scanf_format = "%4d-%2d-%2d %2d:%2d:%2d";

	if (sscanf(s, scanf_format, &year, &month, &day, &hour, &minute, &second) != 6) {
		return NULL;
	}

	if (month < 1 || month > 12 ||
		day < 1 || day > 31 ||
		hour < 0 || hour > 23 ||
		minute < 0 || minute > 59 ||
		second < 0 || second > 59) {
		return NULL;
	}

	int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
		days_in_month[2] = 29;
	}

	if (day > days_in_month[month]) {
		return NULL;
	}

	tm->tm_year = year - 1900;
	tm->tm_mon = month - 1;
	tm->tm_mday = day;
	tm->tm_hour = hour;
	tm->tm_min = minute;
	tm->tm_sec = second;
	tm->tm_isdst = -1; // Let mktime determine DST

	return (char*)s + strlen(s);
}
#endif //_WIN32 || _WIN64
