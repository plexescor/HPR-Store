#include "registryManager.hpp"
#include "network.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <algorithm>
#include <random>
#include <numeric>
#include <thread>
#include <mutex>
#include <print>

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

NLOHMANN_JSON_SERIALIZE_ENUM(StoreItemType, {
    {StoreItemType::EXTENSION, "EXTENSION"},
    {StoreItemType::THEME, "THEME"}
})

inline void from_json(const nlohmann::json& j, StoreItem& item) {
    if (j.contains("id")) j.at("id").get_to(item.id);
    if (j.contains("name")) j.at("name").get_to(item.name);
    if (j.contains("author")) j.at("author").get_to(item.author);
    if (j.contains("authorGithub")) j.at("authorGithub").get_to(item.authorGithub);
    if (j.contains("description")) j.at("description").get_to(item.description);
    if (j.contains("longDescription")) j.at("longDescription").get_to(item.longDescription);
    if (j.contains("version")) j.at("version").get_to(item.version);
    if (j.contains("downloadUrl")) j.at("downloadUrl").get_to(item.downloadUrl);
    if (j.contains("sourceUrl")) j.at("sourceUrl").get_to(item.sourceUrl);
    if (j.contains("type")) j.at("type").get_to(item.type);
    if (j.contains("tags")) j.at("tags").get_to(item.tags);
    if (j.contains("previewImages")) j.at("previewImages").get_to(item.previewImages);
    if (j.contains("supportedHPRVersions")) j.at("supportedHPRVersions").get_to(item.supportedHPRVersions);
    if (j.contains("downloadCount")) j.at("downloadCount").get_to(item.downloadCount);
    if (j.contains("starCount")) j.at("starCount").get_to(item.starCount);
    if (j.contains("lastUpdated")) j.at("lastUpdated").get_to(item.lastUpdated);
}

RegistryManager::RegistryManager() = default;
RegistryManager::~RegistryManager() = default;

std::optional<StoreItem> RegistryManager::getItemById(const std::string& id) const
{
    for (const auto& item : allItems)
    {
        if (item.id == id)
            return item;
    }
    // Fallback: also check items (current page)
    for (const auto& item : items)
    {
        if (item.id == id)
            return item;
    }
    return std::nullopt;
}

std::filesystem::path RegistryManager::getLocalRegistryPath() const
{
    void (*pFromJson)(const nlohmann::json&, StoreItem&) = &from_json;
#ifndef _WIN32
    Dl_info info;
    if (dladdr((const void*)pFromJson, &info) && info.dli_fname)
    {
        auto path = std::filesystem::path(info.dli_fname).parent_path() / "registry.json";
        if (std::filesystem::exists(path)) return path;
    }
#else
    union {
        void (*funcPtr)(const nlohmann::json&, StoreItem&);
        LPCWSTR rawPtr;
    } castUnion;
    castUnion.funcPtr = pFromJson;

    wchar_t path[MAX_PATH];
    HMODULE hm = NULL;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          castUnion.rawPtr, &hm))
    {
        GetModuleFileName(hm, path, MAX_PATH);
        auto winPath = std::filesystem::path(path).parent_path() / "registry.json";
        if (std::filesystem::exists(winPath)) return winPath;
    }
#endif
    // Fallbacks
    std::vector<std::string> fallbacks = { "registry.json", "../registry.json", "../../registry.json" };
    for (const auto& f : fallbacks)
    {
        if (std::filesystem::exists(f)) return std::filesystem::absolute(f);
    }
    return "registry.json";
}

void RegistryManager::readLocalRegistry()
{
    allItems.clear();
    pageOrder.clear();
    seenIds.clear();
    totalItemCount = 0;

    auto localPath = getLocalRegistryPath();
    std::cout << "[RegistryManager] Loading local database from: " << localPath << std::endl;

    if (!std::filesystem::exists(localPath))
    {
        std::cerr << "[RegistryManager] Local registry.json not found." << std::endl;
        return;
    }

    std::ifstream file(localPath);
    if (!file.is_open())
    {
        std::cerr << "[RegistryManager] Failed to open local registry.json" << std::endl;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    try
    {
        auto j = nlohmann::json::parse(content);
        allItems = j.get<std::vector<StoreItem>>();
        rebuildPageOrder();
        std::cout << "[RegistryManager] Loaded " << allItems.size() << " items successfully." << std::endl;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[RegistryManager] JSON parse error: " << e.what() << std::endl;
    }
}

void RegistryManager::updateDatabase()
{
    ix::HttpClient httpClient;
    // Use raw.githubusercontent.com to bypass API 403 rate limits and fetch the raw file directly
    std::string fullUrl = "https://raw.githubusercontent.com/plexescor/HPR-Store/main/registry.json";

    auto args = httpClient.createRequest();
    args->extraHeaders["User-Agent"] = "HPR";

    std::cout << "[RegistryManager] Fetching remote update from: " << fullUrl << std::endl;
    auto response = httpClient.get(fullUrl, args);

    if (response->statusCode == 200 && !response->body.empty())
    {
        auto localPath = getLocalRegistryPath();
        std::ofstream out(localPath, std::ios::binary | std::ios::trunc);
        if (out.is_open())
        {
            out << response->body;
            out.close();
            std::cout << "[RegistryManager] Successfully updated local database at " << localPath << std::endl;
        }
        else
        {
            std::cerr << "[RegistryManager] Failed to overwrite local registry.json at: " << localPath << std::endl;
        }
    }
    else
    {
        std::cerr << "[RegistryManager] Failed to fetch remote update. Status: "
                  << response->statusCode << ", Error: " << response->errorMsg << std::endl;
    }

    // Reload whichever file is now on disk
    readLocalRegistry();
}

void RegistryManager::sortItems(SortMode mode)
{
    currentSort = mode;
    rebuildPageOrder();
}

std::vector<StoreItem> RegistryManager::getPage(int page) const
{
    std::vector<StoreItem> result;
    if (allItems.empty()) return result;

    int start = page * PAGE_SIZE;
    int end = std::min(start + PAGE_SIZE, totalItemCount);

    for (int i = start; i < end; ++i)
    {
        result.push_back(allItems[pageOrder[i]]);
    }

    return result;
}

#include "installer.hpp"
#include <unordered_set>

void RegistryManager::rebuildPageOrder()
{
    pageOrder.clear();

    auto installedItems = Installer::loadInstalledItems();

    // 1. Gather indices of all items matching activeTypeFilter and activeInstalledFilter
    for (int i = 0; i < static_cast<int>(allItems.size()); ++i)
    {
        const auto& item = allItems[i];
        
        // Filter by Type (ALL/EXTENSION/THEME)
        bool matchesType = (activeTypeFilter == 0 ||
            (activeTypeFilter == 1 && item.type == StoreItemType::EXTENSION) ||
            (activeTypeFilter == 2 && item.type == StoreItemType::THEME));

        if (!matchesType) continue;

        // Filter by Installed status (ALL/INSTALLED/NOT INSTALLED)
        bool isInstalled = (installedItems.find(item.id) != installedItems.end());
        bool matchesInstalled = (activeInstalledFilter == 0 ||
            (activeInstalledFilter == 1 && isInstalled) ||
            (activeInstalledFilter == 2 && !isInstalled));

        if (matchesInstalled)
        {
            pageOrder.push_back(i);
        }
    }

    totalItemCount = static_cast<int>(pageOrder.size());

    // 2. If sorting is active, sort our pageOrder indices based on the values in allItems
    if (currentSort == SortMode::STARS_DESC)
    {
        std::stable_sort(pageOrder.begin(), pageOrder.end(),
            [this](int a, int b) {
                return allItems[a].starCount > allItems[b].starCount;
            });
        std::cout << "[RegistryManager] Sorted " << totalItemCount << " filtered items by stars (desc)." << std::endl;
    }
    else if (currentSort == SortMode::DOWNLOADS_DESC)
    {
        std::stable_sort(pageOrder.begin(), pageOrder.end(),
            [this](int a, int b) {
                return allItems[a].downloadCount > allItems[b].downloadCount;
            });
        std::cout << "[RegistryManager] Sorted " << totalItemCount << " filtered items by downloads (desc)." << std::endl;
    }
    else
    {
        // 3. Otherwise, shuffle the active indices randomly
        std::mt19937 rng(std::random_device{}());
        std::shuffle(pageOrder.begin(), pageOrder.end(), rng);
        std::cout << "[RegistryManager] Shuffled " << totalItemCount << " filtered items." << std::endl;
    }
}