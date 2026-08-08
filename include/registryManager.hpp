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
        // Local database path next to the library
        std::filesystem::path getLocalRegistryPath() const;

        // Normal browsing: read local file directly, shuffle, page through
        void readLocalRegistry();
        void updateDatabase(); // Download remote registry and replace the local file

        // Sort mode only: full dataset in memory
        void sortItems(SortMode mode);
        std::vector<StoreItem> getPage(int page) const;
        void rebuildPageOrder();

    public:
        std::vector<StoreItem> items;        // Current page or sort window
        std::vector<StoreItem> allItems;     // Full dataset
        std::vector<int> pageOrder;          // Shuffled indices into allItems
        std::unordered_set<std::string> seenIds;
        SortMode currentSort = SortMode::NONE;
        int activeTypeFilter = 0; // 0=BOTH, 1=EXTENSION, 2=THEME
        int totalItemCount = 0;
        static constexpr int PAGE_SIZE = 100;

    private:
        static constexpr std::string_view REGISTRY_URL =
            "api.github.com";
        static constexpr std::string_view REGISTRY_PATH =
            "/repos/plexescor/HPR-Store/contents/registry.json";
};