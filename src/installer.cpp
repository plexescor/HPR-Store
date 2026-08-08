#include "installer.hpp"

#include <ixwebsocket/IXHttpClient.h>
#include <archive.h>
#include <archive_entry.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

std::filesystem::path Installer::hprBasePath()
{
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return {};
    return std::filesystem::path(appdata) / "HPR" / "HPR_Config";
#else
    const char* home = std::getenv("HOME");
    if (!home) return {};
    return std::filesystem::path(home) / ".config" / "HPR";
#endif
}

std::filesystem::path Installer::storeBasePath()
{
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (!appdata) return {};
    return std::filesystem::path(appdata) / "HPR-Store";
#else
    const char* home = std::getenv("HOME");
    if (!home) return {};
    return std::filesystem::path(home) / ".config" / "HPR-Store";
#endif
}

std::unordered_map<std::string, std::string> Installer::loadInstalledItems()
{
    std::unordered_map<std::string, std::string> installed;
    auto base = storeBasePath();
    if (base.empty()) return installed;

    auto dbPath = base / "installed.json";
    if (!std::filesystem::exists(dbPath)) return installed;

    try
    {
        std::ifstream file(dbPath);
        if (file.is_open())
        {
            nlohmann::json j;
            file >> j;
            if (j.is_object())
            {
                for (auto& el : j.items())
                {
                    if (el.value().is_string())
                    {
                        installed[el.key()] = el.value().get<std::string>();
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Installer] Failed to load installed database: " << e.what() << std::endl;
    }
    return installed;
}

void Installer::saveInstalledItems(const std::unordered_map<std::string, std::string>& items)
{
    auto base = storeBasePath();
    if (base.empty()) return;

    try
    {
        std::filesystem::create_directories(base);
        auto dbPath = base / "installed.json";
        nlohmann::json j = nlohmann::json::object();
        for (const auto& pair : items)
        {
            j[pair.first] = pair.second;
        }
        std::ofstream file(dbPath, std::ios::trunc);
        if (file.is_open())
        {
            file << j.dump(4);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Installer] Failed to save installed database: " << e.what() << std::endl;
    }
}

bool Installer::uninstall(const std::string& id, StoreItemType type)
{
    auto installed = loadInstalledItems();
    auto it = installed.find(id);
    if (it == installed.end())
    {
        std::cerr << "[Installer] Uninstall failed: Item " << id << " not registered as installed." << std::endl;
        return false;
    }

    auto folderName = it->second;
    auto base = hprBasePath();
    if (base.empty()) return false;

    std::filesystem::path targetDir;
    if (type == StoreItemType::THEME)
        targetDir = base / "themes" / folderName;
    else
        targetDir = base / "extensions" / folderName;

    std::cout << "[Installer] Uninstalling: deleting " << targetDir << std::endl;

    try
    {
        if (std::filesystem::exists(targetDir))
        {
            std::filesystem::remove_all(targetDir);
        }
        installed.erase(it);
        saveInstalledItems(installed);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Installer] Failed to remove directory: " << e.what() << std::endl;
        return false;
    }
}

std::filesystem::path Installer::downloadArchive(
    const std::string& url,
    const std::filesystem::path& tempDir,
    std::string& outError)
{
    ix::HttpClient client;
    auto args = client.createRequest();
    args->extraHeaders["User-Agent"] = "HPR-Store";
    args->followRedirects = true;
    args->maxRedirects = 10;
    args->connectTimeout = 30;
    args->transferTimeout = 120;

    std::cout << "[Installer] Downloading: " << url << std::endl;

    auto response = client.get(url, args);
    if (!response || response->statusCode != 200)
    {
        outError = "Download failed (HTTP " + std::to_string(response ? response->statusCode : 0) + ")";
        if (response && !response->errorMsg.empty())
            outError += ": " + response->errorMsg;
        return {};
    }

    if (response->body.empty())
    {
        outError = "Downloaded archive is empty";
        return {};
    }

    // Guess extension from URL
    std::string filename = "archive.zip";
    auto lastSlash = url.rfind('/');
    if (lastSlash != std::string::npos)
    {
        std::string name = url.substr(lastSlash + 1);
        // Strip query string
        auto q = name.find('?');
        if (q != std::string::npos) name = name.substr(0, q);
        if (!name.empty()) filename = name;
    }

    auto archivePath = tempDir / filename;
    std::ofstream out(archivePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        outError = "Failed to open temp file for writing: " + archivePath.string();
        return {};
    }
    out.write(response->body.data(), static_cast<std::streamsize>(response->body.size()));
    out.close();

    std::cout << "[Installer] Saved archive to: " << archivePath << std::endl;
    return archivePath;
}

bool Installer::extractArchive(
    const std::filesystem::path& archivePath,
    const std::filesystem::path& destDir,
    std::string& outError)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_FFLAGS);
    archive_write_disk_set_standard_lookup(ext);

    int r = archive_read_open_filename(a, archivePath.string().c_str(), 16384);
    if (r != ARCHIVE_OK)
    {
        outError = "Failed to open archive: " + std::string(archive_error_string(a));
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }

    struct archive_entry* entry;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        // Prepend destDir to each entry path
        std::filesystem::path entryPath = destDir / archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, entryPath.string().c_str());

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK)
        {
            std::cerr << "[Installer] Warning writing header: " << archive_error_string(ext) << std::endl;
            continue;
        }

        // Copy data blocks
        const void* buff;
        size_t size;
        la_int64_t offset;
        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK)
        {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK)
            {
                std::cerr << "[Installer] Warning writing data: " << archive_error_string(ext) << std::endl;
                break;
            }
        }
        if (r != ARCHIVE_EOF && r != ARCHIVE_OK)
        {
            outError = "Error reading archive data: " + std::string(archive_error_string(a));
            archive_read_close(a);
            archive_read_free(a);
            archive_write_close(ext);
            archive_write_free(ext);
            return false;
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    if (r != ARCHIVE_EOF)
    {
        outError = "Archive extraction ended unexpectedly";
        return false;
    }

    return true;
}

std::filesystem::path Installer::findInstallRoot(
    const std::filesystem::path& extractedDir,
    StoreItemType type,
    std::string& outError)
{
    if (type == StoreItemType::THEME)
    {
        // Look for metadata.csv whose sibling is app-window.slint
        for (const auto& entry : std::filesystem::recursive_directory_iterator(extractedDir))
        {
            if (entry.is_regular_file() && entry.path().filename() == "metadata.csv")
            {
                auto parent = entry.path().parent_path();
                if (std::filesystem::exists(parent / "app-window.slint"))
                {
                    std::cout << "[Installer] Found theme root: " << parent << std::endl;
                    return parent;
                }
            }
        }
        outError = "Could not find a valid theme folder (needs metadata.csv + app-window.slint)";
        return {};
    }
    else // EXTENSION
    {
        // Find the first .lua file; its parent is the extension root
        for (const auto& entry : std::filesystem::recursive_directory_iterator(extractedDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".lua")
            {
                auto parent = entry.path().parent_path();
                std::cout << "[Installer] Found extension root: " << parent << std::endl;
                return parent;
            }
        }
        outError = "Could not find a valid extension folder (needs a .lua file)";
        return {};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public
// ─────────────────────────────────────────────────────────────────────────────

InstallResult Installer::install(const StoreItem& item, std::function<void(std::string)> progressCallback)
{
    InstallResult result;

    if (item.downloadUrl.empty())
    {
        result.errorMessage = "No download URL available for this item";
        return result;
    }

    // ── 1. Create temp dir ────────────────────────────────────────────────────
    progressCallback("DOWNLOADING ARCHIVE...");
    std::filesystem::path tempBase;
    try
    {
        tempBase = std::filesystem::temp_directory_path() / ("hpr-install-" + item.id);
        std::filesystem::create_directories(tempBase);
    }
    catch (const std::exception& e)
    {
        result.errorMessage = "Failed to create temp directory: " + std::string(e.what());
        return result;
    }

    // Ensure cleanup always runs
    auto cleanup = [&]()
    {
        progressCallback("CLEANING UP...");
        try { std::filesystem::remove_all(tempBase); }
        catch (...) {}
    };

    // ── 2. Download ───────────────────────────────────────────────────────────
    std::string dlError;
    auto archivePath = downloadArchive(item.downloadUrl, tempBase, dlError);
    if (archivePath.empty())
    {
        result.errorMessage = dlError;
        cleanup();
        return result;
    }

    // ── 3. Extract ────────────────────────────────────────────────────────────
    progressCallback("EXTRACTING ARCHIVE...");
    auto extractedDir = tempBase / "extracted";
    std::filesystem::create_directories(extractedDir);

    std::string extractError;
    if (!extractArchive(archivePath, extractedDir, extractError))
    {
        result.errorMessage = extractError;
        cleanup();
        return result;
    }

    // ── 4. Find install root ──────────────────────────────────────────────────
    progressCallback("LOCATING ROOT FOLDER...");
    std::string findError;
    auto installRoot = findInstallRoot(extractedDir, item.type, findError);
    if (installRoot.empty())
    {
        result.errorMessage = findError;
        cleanup();
        return result;
    }

    // ── 5. Rename if parent is named "extracted" ──────────────────────────────
    std::string targetFolderName = installRoot.filename().string();
    if (targetFolderName == "extracted")
    {
        if (item.type == StoreItemType::EXTENSION)
        {
            // Find the .lua file and use its stem
            for (const auto& entry : std::filesystem::recursive_directory_iterator(installRoot))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".lua")
                {
                    targetFolderName = entry.path().stem().string();
                    break;
                }
            }
        }
        else // THEME
        {
            // Parse metadata.csv name field
            auto csvPath = installRoot / "metadata.csv";
            if (std::filesystem::exists(csvPath))
            {
                std::ifstream csv(csvPath);
                std::string line;
                while (std::getline(csv, line))
                {
                    size_t comma = line.find(',');
                    if (comma != std::string::npos)
                    {
                        std::string key = line.substr(0, comma);
                        std::string val = line.substr(comma + 1);
                        // Trim carriage returns and whitespace
                        auto trim = [](std::string& s) {
                            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
                                return std::isspace(c) || c == '\r' || c == '\n';
                            }), s.end());
                        };
                        trim(key);
                        if (key == "name")
                        {
                            // Sanitize the theme name for directory usage (keep alphanumeric & hyphens)
                            std::string cleaned;
                            for (char c : val)
                            {
                                if (std::isalnum(static_cast<unsigned char>(c)))
                                    cleaned += std::tolower(static_cast<unsigned char>(c));
                                else if (std::isspace(static_cast<unsigned char>(c)) || c == '-' || c == '_')
                                    cleaned += '-';
                            }
                            // Trim duplicate hyphens
                            cleaned.erase(std::unique(cleaned.begin(), cleaned.end(), [](char a, char b) {
                                return a == '-' && b == '-';
                            }), cleaned.end());
                            if (!cleaned.empty())
                            {
                                targetFolderName = cleaned;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    // ── 6. Copy to HPR directory ──────────────────────────────────────────────
    progressCallback("COPYING FILES...");
    auto base = hprBasePath();
    if (base.empty())
    {
        result.errorMessage = "Could not determine HPR config directory";
        cleanup();
        return result;
    }

    std::filesystem::path destDir;
    if (item.type == StoreItemType::THEME)
        destDir = base / "themes" / targetFolderName;
    else
        destDir = base / "extensions" / targetFolderName;

    try
    {
        std::filesystem::create_directories(destDir);
        std::filesystem::copy(
            installRoot, destDir,
            std::filesystem::copy_options::recursive |
            std::filesystem::copy_options::overwrite_existing);
        std::cout << "[Installer] Installed to: " << destDir << std::endl;
    }
    catch (const std::exception& e)
    {
        result.errorMessage = "Failed to copy files: " + std::string(e.what());
        cleanup();
        return result;
    }

    // ── 7. Update local installed database ────────────────────────────────────
    auto installed = loadInstalledItems();
    installed[item.id] = targetFolderName;
    saveInstalledItems(installed);

    // ── 8. Cleanup ────────────────────────────────────────────────────────────
    cleanup();

    result.success = true;
    return result;
}
