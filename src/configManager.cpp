#include "configManager.hpp"
#include "installer.hpp"
#include "csvParser.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

std::filesystem::path ConfigManager::getConfigFilePath()
{
    auto base = Installer::storeBasePath();
    if (base.empty()) return {};
    return base / "config.csv";
}

void ConfigManager::createDefaultConfigIfMissing()
{
    auto path = getConfigFilePath();
    if (path.empty()) return;

    try
    {
        auto dir = path.parent_path();
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }

        if (!std::filesystem::exists(path))
        {
            std::ofstream out(path, std::ios::trunc);
            if (out.is_open())
            {
                out << "# HPR-Store Configuration\n";
                out << "# enable-network: true to fetch registry from internet, false to use local file\n";
                out << "enable-network,true\n";
                out << "# custom-registry-url: Direct download URL for registry.json (used when enable-network is true). Leave empty for default.\n";
                out << "custom-registry-url,\n";
                out << "# custom-local-registry-path: Path to a local registry.json (used when enable-network is false). Leave empty for default.\n";
                out << "custom-local-registry-path,\n";
                out.close();
                std::cout << "[ConfigManager] Created default config.csv at " << path << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ConfigManager] Failed to create default config.csv: " << e.what() << std::endl;
    }
}

StoreConfig ConfigManager::loadConfig()
{
    StoreConfig config;
    auto path = getConfigFilePath();
    if (path.empty()) return config;

    createDefaultConfigIfMissing();

    if (!std::filesystem::exists(path)) return config;

    try
    {
        auto map = parseCsvToMap<std::string>(path);

        // Helper to query with kebab-case or snake_case fallback
        auto getValue = [&map](const std::string& kebab, const std::string& snake) -> std::string
        {
            auto it = map.find(kebab);
            if (it != map.end()) return it->second;
            it = map.find(snake);
            if (it != map.end()) return it->second;
            return "";
        };

        // Parse enable-network
        std::string netVal = getValue("enable-network", "enable_network");
        if (!netVal.empty())
        {
            std::string lower = netVal;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
            {
                config.enableNetwork = false;
            }
            else
            {
                config.enableNetwork = true;
            }
        }

        // Parse custom-registry-url
        config.customRegistryUrl = getValue("custom-registry-url", "custom_registry_url");

        // Parse custom-local-registry-path
        config.customLocalRegistryPath = getValue("custom-local-registry-path", "custom_local_registry_path");

        std::cout << "[ConfigManager] Loaded config from " << path
                  << " (enable-network: " << (config.enableNetwork ? "true" : "false")
                  << ", custom-registry-url: " << (config.customRegistryUrl.empty() ? "(default)" : config.customRegistryUrl)
                  << ", custom-local-registry-path: " << (config.customLocalRegistryPath.empty() ? "(default)" : config.customLocalRegistryPath)
                  << ")" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ConfigManager] Failed to parse config.csv: " << e.what() << ", using defaults." << std::endl;
    }

    return config;
}
