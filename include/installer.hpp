#pragma once
#include "registryEntry.hpp"
#include <string>
#include <filesystem>

struct InstallResult
{
    bool success = false;
    std::string errorMessage;
};

class Installer
{
public:
    // Download, extract and install an item into HPR's theme or extension directory.
    // Blocks the calling thread — run from a background thread.
    static InstallResult install(const StoreItem& item);

private:
    // Determine the HPR base config path (~/.config/HPR/ or %APPDATA%/HPR/HPR_Config/)
    static std::filesystem::path hprBasePath();

    // Download item.downloadUrl to a file inside tempDir.
    // Returns the path to the downloaded file, or empty on failure.
    static std::filesystem::path downloadArchive(
        const std::string& url,
        const std::filesystem::path& tempDir,
        std::string& outError);

    // Extract the archive at archivePath into destDir using libarchive.
    static bool extractArchive(
        const std::filesystem::path& archivePath,
        const std::filesystem::path& destDir,
        std::string& outError);

    // Recursively scan extractedDir and find the root folder to install.
    //   THEME     → dir containing both metadata.csv and app-window.slint
    //   EXTENSION → parent dir of the first .lua file found
    static std::filesystem::path findInstallRoot(
        const std::filesystem::path& extractedDir,
        StoreItemType type,
        std::string& outError);
};
