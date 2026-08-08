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

std::filesystem::path RegistryManager::getTempFilePath() const
{
    return std::filesystem::temp_directory_path() / "hpr-store-registry.json";
}

// ── Normal browsing ──────────────────────────────────────────────────────────

void RegistryManager::fetchRegistryToTemp()
{
    ix::HttpClient httpClient;
    std::string fullUrl = "https://" + std::string(REGISTRY_URL) + std::string(REGISTRY_PATH);

    auto args = httpClient.createRequest();
    args->extraHeaders["User-Agent"] = "HPR";
    args->extraHeaders["Accept"] = "application/vnd.github.v3.raw";

    auto response = httpClient.get(fullUrl, args);

    if (response->statusCode == 200 && !response->body.empty())
    {
        auto tempPath = getTempFilePath();
        std::ofstream out(tempPath, std::ios::trunc);
        if (out.is_open())
        {
            out << response->body;
            out.close();
            std::cout << "[RegistryManager] Saved registry.json to " << tempPath
                      << " (" << response->body.size() << " bytes)." << std::endl;
        }
        else
        {
            std::cerr << "[RegistryManager] Failed to write to temp file." << std::endl;
        }
    }
    else
    {
        std::cerr << "[RegistryManager] Failed to fetch registry from " << fullUrl
                  << ". Status: " << response->statusCode
                  << ", Error: " << response->errorMsg << std::endl;
    }
}

void RegistryManager::parseRegistryFromTemp()
{
    allItems.clear();
    pageOrder.clear();
    seenIds.clear();
    totalItemCount = 0;

    auto tempPath = getTempFilePath();
    if (!std::filesystem::exists(tempPath))
    {
        std::cerr << "[RegistryManager] Temp registry file not found: " << tempPath << std::endl;
        return;
    }

    std::ifstream file(tempPath);
    if (!file.is_open())
    {
        std::cerr << "[RegistryManager] Failed to open temp registry file." << std::endl;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    try
    {
        auto j = nlohmann::json::parse(content);
        allItems = j.get<std::vector<StoreItem>>();
        totalItemCount = static_cast<int>(allItems.size());

        // Build shuffled index order
        pageOrder.resize(totalItemCount);
        std::iota(pageOrder.begin(), pageOrder.end(), 0);

        std::mt19937 rng(std::random_device{}());
        std::shuffle(pageOrder.begin(), pageOrder.end(), rng);

        std::cout << "[RegistryManager] Parsed " << totalItemCount
                  << " items from temp file (shuffled)." << std::endl;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[RegistryManager] JSON parsing error: " << e.what() << std::endl;
    }
}

void RegistryManager::fetchAndParseToTemp()
{
    currentSort = SortMode::NONE;
    fetchRegistryToTemp();
    parseRegistryFromTemp();
    enrichItemsFromGitHub();
}

// ── Sort mode (full in-memory) ───────────────────────────────────────────────

void RegistryManager::fetchRegistry()
{
    ix::HttpClient httpClient;
    std::string fullUrl = "https://" + std::string(REGISTRY_URL) + std::string(REGISTRY_PATH);

    auto args = httpClient.createRequest();
    args->extraHeaders["User-Agent"] = "HPR";
    args->extraHeaders["Accept"] = "application/vnd.github.v3.raw";

    auto response = httpClient.get(fullUrl, args);

    if (response->statusCode == 200 && !response->body.empty())
    {
        REGISTRY_CONTENT = response->body;
        std::cout << "[RegistryManager] Fetched remote registry.json ("
                  << response->body.size() << " bytes)." << std::endl;
    }
    else
    {
        std::cerr << "[RegistryManager] Failed to fetch registry. Status: "
                  << response->statusCode << ", Error: " << response->errorMsg << std::endl;
        REGISTRY_CONTENT.clear();
        items.clear();
    }
}

void RegistryManager::parseRegistry()
{
    allItems.clear();
    if (REGISTRY_CONTENT.empty()) return;

    try
    {
        auto j = nlohmann::json::parse(REGISTRY_CONTENT);
        allItems = j.get<std::vector<StoreItem>>();
        totalItemCount = static_cast<int>(allItems.size());

        // Build identity page order (no shuffle — sort will reorder)
        pageOrder.resize(totalItemCount);
        std::iota(pageOrder.begin(), pageOrder.end(), 0);

        std::cout << "[RegistryManager] Parsed " << totalItemCount
                  << " items for sort mode." << std::endl;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[RegistryManager] JSON parsing error: " << e.what() << std::endl;
    }
}

void RegistryManager::fetchAndParseRegistry()
{
    fetchRegistry();
    parseRegistry();
    enrichItemsFromGitHub();
}

void RegistryManager::sortItems(SortMode mode)
{
    currentSort = mode;

    if (mode == SortMode::STARS_DESC)
    {
        std::stable_sort(allItems.begin(), allItems.end(),
            [](const StoreItem& a, const StoreItem& b) {
                return a.starCount > b.starCount;
            });
        std::cout << "[RegistryManager] Sorted by stars (desc)." << std::endl;
    }
    else if (mode == SortMode::DOWNLOADS_DESC)
    {
        std::stable_sort(allItems.begin(), allItems.end(),
            [](const StoreItem& a, const StoreItem& b) {
                return a.downloadCount > b.downloadCount;
            });
        std::cout << "[RegistryManager] Sorted by downloads (desc)." << std::endl;
    }

    // Rebuild page order after sort
    pageOrder.resize(totalItemCount);
    std::iota(pageOrder.begin(), pageOrder.end(), 0);
}

// ── Paged slice ──────────────────────────────────────────────────────────────

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

// ── GitHub enrichment ────────────────────────────────────────────────────────

static std::pair<std::string, std::string> parseGithubOwnerRepo(const std::string& url)
{
    // Extract owner/repo from https://github.com/owner/repo or https://github.com/owner/repo.git
    const std::string prefix = "github.com/";
    auto pos = url.find(prefix);
    if (pos == std::string::npos) return {};

    std::string path = url.substr(pos + prefix.size());
    // Remove trailing .git if present
    if (path.size() > 4 && path.substr(path.size() - 4) == ".git")
        path = path.substr(0, path.size() - 4);

    auto slash = path.find('/');
    if (slash == std::string::npos) return {};

    return { path.substr(0, slash), path.substr(slash + 1) };
}

void RegistryManager::enrichItemsFromGitHub()
{
    if (allItems.empty()) return;

    std::cout << "[RegistryManager] Enriching " << allItems.size() << " items from GitHub..." << std::endl;

    std::mutex itemsMutex;
    std::vector<std::thread> threads;
    threads.reserve(allItems.size());

    for (int idx = 0; idx < static_cast<int>(allItems.size()); ++idx)
    {
        StoreItem& item = allItems[idx];

        bool needsStars   = (item.starCount == 0);
        bool needsVersion = item.version.empty();
        bool needsDate    = item.lastUpdated.empty();

        if (!needsStars && !needsVersion && !needsDate) continue;
        if (item.sourceUrl.find("github.com") == std::string::npos) continue;

        threads.emplace_back([&itemsMutex, &item, needsStars, needsVersion, needsDate]()
        {
            auto [owner, repo] = parseGithubOwnerRepo(item.sourceUrl);
            if (owner.empty() || repo.empty())
            {
                std::lock_guard lock(itemsMutex);
                if (item.starCount == 0)    item.starCount = -1;  // Unknown sentinel
                if (item.version.empty())   item.version = "Unknown";
                if (item.lastUpdated.empty()) item.lastUpdated = "Unknown";
                return;
            }

            ix::HttpClient httpClient;
            auto args = httpClient.createRequest();
            args->extraHeaders["User-Agent"] = "HPR";
            args->extraHeaders["Accept"] = "application/vnd.github.v3+json";

            int    fetchedStars   = -1;
            std::string fetchedVersion  = "Unknown";
            std::string fetchedDate     = "Unknown";

            // ── Repo info (stars) ────────────────────────────────────────
            if (needsStars)
            {
                std::string repoUrl = "https://api.github.com/repos/" + owner + "/" + repo;
                auto res = httpClient.get(repoUrl, args);
                if (res->statusCode == 200 && !res->body.empty())
                {
                    try {
                        auto j = nlohmann::json::parse(res->body);
                        fetchedStars = j.value("stargazers_count", -1);
                    } catch (...) {}
                }
            }

            // ── Latest release (version + date) ──────────────────────────
            if (needsVersion || needsDate)
            {
                std::string releaseUrl = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest";
                auto res = httpClient.get(releaseUrl, args);
                if (res->statusCode == 200 && !res->body.empty())
                {
                    try {
                        auto j = nlohmann::json::parse(res->body);
                        if (needsVersion)
                        {
                            std::string tag = j.value("tag_name", "");
                            // Strip leading 'v' from tag
                            if (!tag.empty() && tag[0] == 'v') tag = tag.substr(1);
                            if (!tag.empty()) fetchedVersion = tag;
                        }
                        if (needsDate)
                        {
                            std::string published = j.value("published_at", "");
                            // Trim to YYYY-MM-DD
                            if (published.size() >= 10) published = published.substr(0, 10);
                            if (!published.empty()) fetchedDate = published;
                        }
                    } catch (...) {}
                }
                else if (res->statusCode == 404)
                {
                    // No releases — fall back to repo pushed_at date
                    if (needsDate)
                    {
                        std::string repoUrl2 = "https://api.github.com/repos/" + owner + "/" + repo;
                        auto res2 = httpClient.get(repoUrl2, args);
                        if (res2->statusCode == 200 && !res2->body.empty())
                        {
                            try {
                                auto j2 = nlohmann::json::parse(res2->body);
                                std::string pushed = j2.value("pushed_at", "");
                                if (pushed.size() >= 10) pushed = pushed.substr(0, 10);
                                if (!pushed.empty()) fetchedDate = pushed;
                            } catch (...) {}
                        }
                    }
                    if (needsVersion) fetchedVersion = "Unknown";
                }
            }

            // Write back under lock
            {
                std::lock_guard lock(itemsMutex);
                if (needsStars)   item.starCount   = fetchedStars;
                if (needsVersion) item.version     = fetchedVersion;
                if (needsDate)    item.lastUpdated = fetchedDate;

                std::cout << "[GitHub] " << item.id
                          << " -> stars=" << item.starCount
                          << " ver=" << item.version
                          << " date=" << item.lastUpdated << std::endl;
            }
        });
    }

    // Join all threads
    for (auto& t : threads) t.join();

    std::cout << "[RegistryManager] GitHub enrichment complete." << std::endl;
}