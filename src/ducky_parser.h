#pragma once

// ============================================================
//  DuckyScript Parser — Non-blocking FreeRTOS-based Interpreter
// ============================================================

#include <Arduino.h>
#include <functional>

/// Execution status reported via callback
enum class DuckyStatus { IDLE, RUNNING, PAUSED, FINISHED, ERROR, ABORTED };

/// Callback: (currentLine, totalLines, status)
using DuckyCallback = std::function<void(int, int, DuckyStatus)>;

/// Initialize the parser module (creates FreeRTOS task).
void duckyInit();

/// Execute a DuckyScript payload from a string.
/// Returns false if another script is already running.
bool duckyExecute(const String &script, DuckyCallback cb = nullptr);

/// Execute a DuckyScript payload from a file path on LittleFS.
/// Returns false if another script is already running.
bool duckyExecuteFile(const String &filePath, DuckyCallback cb = nullptr);

/// Abort the currently running script.
void duckyStop();

/// Check if a script is currently executing.
bool duckyIsRunning();

/// Get the current execution status.
DuckyStatus duckyGetStatus();
