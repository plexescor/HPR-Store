#pragma once
#include "registryEntry.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

enum class SortMode { NONE, STARS_DESC, DOWNLOADS_DESC };

class RegistryManager
{
    public:
        RegistryManager();
        ~RegistryManager();

    public:
        // Normal browsing: fetch to temp file, shuffle, page through
        void fetchRegistryToTemp();
        void parseRegistryFromTemp();
        void fetchAndParseToTemp();

        // Sort mode only: full dataset in memory
        void fetchRegistry();
        void parseRegistry();
        void fetchAndParseRegistry();

        void sortItems(SortMode mode);
        std::vector<StoreItem> getPage(int page) const;
        void enrichItemsFromGitHub();         // Fills missing star/version/lastUpdated in parallel

    public:
        std::string REGISTRY_CONTENT;
        std::vector<StoreItem> items;        // Current page or sort window
        std::vector<StoreItem> allItems;     // Full dataset (sort mode only)
        std::vector<int> pageOrder;          // Shuffled indices into allItems
        std::unordered_set<std::string> seenIds;
        SortMode currentSort = SortMode::NONE;
        int totalItemCount = 0;
        static constexpr int PAGE_SIZE = 100;

    private:
        std::filesystem::path getTempFilePath() const;

        static constexpr std::string_view REGISTRY_URL =
            "api.github.com";
        static constexpr std::string_view REGISTRY_PATH =
            "/repos/plexescor/HPR-Store/contents/registry.json";
};