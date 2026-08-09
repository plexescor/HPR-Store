#pragma once
#include "registryEntry.hpp"
#include <string>
#include <filesystem>
#include <functional>
#include <unordered_map>

struct InstallResult
{
    bool success = false;
    std::string errorMessage;
};

// Record stored per item in installed.json
struct InstalledRecord
{
    std::string folder;
    std::string version;
    std::string authorName;
    std::string extensionName;
};

class Installer
{
public:
    // Download, extract and install an item into HPR's theme or extension directory.
    // Blocks the calling thread — run from a background thread.
    static InstallResult install(
        const StoreItem& item,
        std::function<void(std::string)> progressCallback = [](std::string){});

    // Upgrade an already-installed item by merge-copying new files over the existing folder.
    // Preserves files that exist only in the destination (e.g. user config files).
    // Self-upgrade HPR Store by renaming active library binaries to .old before copying
    static InstallResult selfUpgrade(
        const StoreItem& item,
        std::function<void(std::string)> progressCallback = [](std::string){});

    static std::filesystem::path getSelfExtensionDir();
    static void cleanupOldFiles();

    static InstallResult upgrade(
        const StoreItem& item,
        std::function<void(std::string)> progressCallback = [](std::string){});

    // Uninstall an item from HPR and update database.
    static bool uninstall(const std::string& id, StoreItemType type);

    // Synchronously unload an extension via Lua if currently running
    static void unloadIfRunning(const std::string& itemId);

    // Installed status database helpers (schema: id → {folder, version, authorName, extensionName})
    static std::filesystem::path storeBasePath();
    static std::unordered_map<std::string, InstalledRecord> loadInstalledItems();
    static void saveInstalledItems(const std::unordered_map<std::string, InstalledRecord>& items);

    // Parse authorName and extensionName from the primary .lua file inside installRoot
    static void parseLuaMetadata(
        const std::filesystem::path& installRoot,
        std::string& outAuthor,
        std::string& outName);

    // Determine the HPR base config path (~/.config/HPR/ or %APPDATA%/HPR/HPR_Config/)
    static std::filesystem::path hprBasePath();

private:

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
