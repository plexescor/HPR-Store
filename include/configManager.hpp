#pragma once

#include <string>
#include <filesystem>

struct StoreConfig
{
    bool enableNetwork = true;
    std::string customRegistryUrl;
    std::string customLocalRegistryPath;
};

class ConfigManager
{
public:
    // Path to config.csv in HPR-Store config directory
    static std::filesystem::path getConfigFilePath();

    // Loads config.csv, creating default file if missing
    static StoreConfig loadConfig();

    // Writes default template if config.csv doesn't exist
    static void createDefaultConfigIfMissing();
};
