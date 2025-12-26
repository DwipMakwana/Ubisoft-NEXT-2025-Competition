//-----------------------------------------------------------------------------
// Logger.h
// Simple logging utility for debugging
//-----------------------------------------------------------------------------
#ifndef LOGGER_H
#define LOGGER_H

#include <Windows.h>
#include <cstdio>
#include <cstdarg>

class Logger {
public:
    // Log a simple string
    static void Log(const char* message) {
        OutputDebugStringA(message);
    }

    // Log with printf-style formatting
    static void LogFormat(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        OutputDebugStringA(buffer);
    }

    // Log with automatic newline
    static void LogLine(const char* message) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "%s\n", message);
        OutputDebugStringA(buffer);
    }

    // Log error with prefix
    static void LogError(const char* message) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "ERROR: %s\n", message);
        OutputDebugStringA(buffer);
    }

    // Log warning with prefix
    static void LogWarning(const char* message) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "WARNING: %s\n", message);
        OutputDebugStringA(buffer);
    }

    // Log info with prefix
    static void LogInfo(const char* message) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "INFO: %s\n", message);
        OutputDebugStringA(buffer);
    }
};

#endif // LOGGER_H
