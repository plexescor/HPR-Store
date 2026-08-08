#pragma once
#include "registryEntry.hpp"
#include <string>

class RegistryManager
{
    public:
        RegistryManager();
        ~RegistryManager();

    public:
        std::string fetchRegistry();
        std::string REGISTRY_CONTENT;

    private:
        static constexpr std::string_view REGISTRY_URL = 
            "raw.githubusercontent.com";
        static constexpr std::string_view REGISTRY_PATH = 
            "/plexescor/HPR-Store/main/registry.json";
};