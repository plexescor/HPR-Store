#pragma once
#include "registryEntry.hpp"

class RegistryManager
{
    public:
        RegistryManager();
        ~RegistryManager();

    public:
        std::vector<StoreItem> fetchRegistry();

    private:
        static constexpr std::string_view REGISTRY_URL = 
            "raw.githubusercontent.com";
        static constexpr std::string_view REGISTRY_PATH = 
            "/plexescor/HPR-Store/main/registry.json";
};