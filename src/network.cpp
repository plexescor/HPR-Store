#include "network.hpp"

#include <curl/curl.h>
#include <fstream>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Internal write callbacks
// ─────────────────────────────────────────────────────────────────────────────

// Accumulates data into a std::string (used by httpGet)
static size_t writeToString(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* out = static_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

// Writes chunks directly to an open ofstream (used by httpGetToFile)
static size_t writeToFile(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* file = static_cast<std::ofstream*>(userdata);
    file->write(ptr, static_cast<std::streamsize>(size * nmemb));
    return size * nmemb;
}

// ─────────────────────────────────────────────────────────────────────────────
// Shared CURL handle setup
// ─────────────────────────────────────────────────────────────────────────────

static void applyCommonOptions(
    CURL*              curl,
    const std::string& url,
    const std::string& userAgent,
    int                connectTimeoutSec,
    int                transferTimeoutSec,
    bool               followRedirects,
    int                maxRedirects)
{
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT,      userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(connectTimeoutSec));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        static_cast<long>(transferTimeoutSec));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS,      static_cast<long>(maxRedirects));
    // Use the OS certificate store on Windows (Schannel), or the system bundle on Linux
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

HttpResponse httpGet(
    const std::string& url,
    const std::string& userAgent,
    int  connectTimeoutSec,
    int  transferTimeoutSec,
    bool followRedirects,
    int  maxRedirects)
{
    HttpResponse result;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        result.errorMsg = "curl_easy_init() failed";
        return result;
    }

    applyCommonOptions(curl, url, userAgent, connectTimeoutSec, transferTimeoutSec, followRedirects, maxRedirects);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &result.body);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        result.errorMsg = curl_easy_strerror(res);
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);
    }

    curl_easy_cleanup(curl);
    return result;
}

bool httpGetToFile(
    const std::string& url,
    const std::string& destPath,
    std::string&       outError,
    const std::string& userAgent,
    int  connectTimeoutSec,
    int  transferTimeoutSec,
    bool followRedirects,
    int  maxRedirects)
{
    std::ofstream file(destPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        outError = "Failed to open destination file for writing: " + destPath;
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        outError = "curl_easy_init() failed";
        return false;
    }

    applyCommonOptions(curl, url, userAgent, connectTimeoutSec, transferTimeoutSec, followRedirects, maxRedirects);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &file);

    CURLcode res = curl_easy_perform(curl);

    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);
    file.close();

    if (res != CURLE_OK)
    {
        outError = "Download failed: " + std::string(curl_easy_strerror(res));
        return false;
    }
    if (statusCode != 200)
    {
        outError = "Download failed (HTTP " + std::to_string(statusCode) + ")";
        return false;
    }

    return true;
}