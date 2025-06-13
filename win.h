#ifndef WIN_H
#define WIN_H
#pragma once

#ifdef _WIN32
char* realpath(const char* path, char* resolved_path);

char* strptime(const char* s, const char* format, struct tm* tm);
#endif // _WIN32 || _WIN64

#endif // WIN_H
