#pragma once
#include "registryEntry.hpp"
#include <string>
#include <vector>

class RegistryManager
{
    public:
        RegistryManager();
        ~RegistryManager();

    public:
        void fetchRegistry();
        void parseRegistry();
        void fetchAndParseRegistry();
        std::string REGISTRY_CONTENT;
        std::vector<StoreItem> items;

    private:
        static constexpr std::string_view REGISTRY_URL = 
            "api.github.com";
        static constexpr std::string_view REGISTRY_PATH = 
            "/repos/plexescor/HPR-Store/contents/registry.json";
};