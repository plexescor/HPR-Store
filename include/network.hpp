#pragma once
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Minimal libcurl HTTP wrapper used across the project.
// Only network.cpp touches raw CURL* handles.
// ─────────────────────────────────────────────────────────────────────────────

struct HttpResponse
{
    long        statusCode = 0;
    std::string body;
    std::string errorMsg;
};

// Blocking GET — downloads the response body into memory.
// Use for small payloads (JSON, text).
HttpResponse httpGet(
    const std::string& url,
    const std::string& userAgent       = "HPR-Store",
    int  connectTimeoutSec             = 15,
    int  transferTimeoutSec            = 60,
    bool followRedirects               = true,
    int  maxRedirects                  = 10);

// Blocking GET — streams response directly to a file on disk.
// Use for large binary downloads (archives, images) to avoid RAM spikes.
// Returns true on success; sets outError on failure.
bool httpGetToFile(
    const std::string& url,
    const std::string& destPath,
    std::string&       outError,
    const std::string& userAgent       = "HPR-Store",
    int  connectTimeoutSec             = 30,
    int  transferTimeoutSec            = 120,
    bool followRedirects               = true,
    int  maxRedirects                  = 10);