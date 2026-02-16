#pragma once

// ============================================================
//  Web Server — REST API + Static File Serving
// ============================================================

#include <Arduino.h>

/// Initialize the AsyncWebServer, register all routes, and start listening.
void webServerInit();
