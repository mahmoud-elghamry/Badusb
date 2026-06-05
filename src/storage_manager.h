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

/// Validate a payload filename. Names must be simple basenames, not paths.
bool isValidPayloadName(const String &name);

/// Check whether a payload exists.
bool payloadExists(const String &name);

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

/// Get custom USB identity (VID and PID).
void getUsbIdentity(uint16_t &vid, uint16_t &pid);

/// Set custom USB identity (VID and PID).
bool setUsbIdentity(uint16_t vid, uint16_t pid);

/// Return whether the web admin token has been initialized.
bool isAdminTokenConfigured();

/// Store a new web admin token hash.
bool setAdminToken(const String &token);

/// Validate a provided web admin token.
bool validateAdminToken(const String &token);
