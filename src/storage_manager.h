#pragma once

// ============================================================
//  Storage Manager — LittleFS Payload CRUD
// ============================================================

#include <Arduino.h>
#include <vector>

/// Initialize LittleFS and create required directories.
bool storageInit();

/// List all payload filenames in PAYLOAD_DIR.
std::vector<String> listPayloads();

/// Read a payload's content by name.
String readPayload(const String &name);

/// Save (create/overwrite) a payload.
bool savePayload(const String &name, const String &content);

/// Delete a payload by name.
bool deletePayload(const String &name);

/// Get the autorun payload filename (empty string if none).
String getAutoRunPayload();

/// Set the autorun payload filename (empty string to disable).
bool setAutoRunPayload(const String &name);

/// Get total and used bytes on LittleFS.
void getStorageInfo(size_t &totalBytes, size_t &usedBytes);
