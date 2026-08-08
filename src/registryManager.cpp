#include "registryManager.hpp"
#include "network.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
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

void RegistryManager::fetchRegistry()
{
    std::string localContent;
    std::vector<std::string> localPaths = { 
        "registry.json", 
        "../registry.json", 
        "../../registry.json", 
        "src/registry.json",
        "../src/registry.json"
    };

    for (const auto& path : localPaths)
    {
        if (std::filesystem::exists(path))
        {
            std::ifstream file(path);
            if (file.is_open())
            {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                if (content.find("longDescription") != std::string::npos)
                {
                    localContent = content;
                    std::cout << "[RegistryManager] Loaded local " << path << " with longDescription (" << content.size() << " bytes)." << std::endl;
                    break;
                }
            }
        }
    }

    if (!localContent.empty())
    {
        REGISTRY_CONTENT = localContent;
        return;
    }

    ix::HttpClient httpClient;
    std::string fullUrl = "https://" + std::string(REGISTRY_URL) + std::string(REGISTRY_PATH);

    auto args = httpClient.createRequest();
    args->extraHeaders["User-Agent"] = "HPR";
    args->extraHeaders["Accept"] = "application/vnd.github.v3.raw";

    auto response = httpClient.get(fullUrl, args);

    if (response->statusCode == 200 && !response->body.empty())
    {
        REGISTRY_CONTENT = response->body;
        std::cout << "[RegistryManager] Fetched remote registry.json (" << response->body.size() << " bytes)." << std::endl;
    }
    else
    {
        std::cerr << "Failed to fetch registry from " << fullUrl
                  << ". Status code: " << response->statusCode
                  << ", Error: " << response->errorMsg << std::endl;
        REGISTRY_CONTENT.clear();
        items.clear();
    }
}

void RegistryManager::parseRegistry()
{
    items.clear();
    if (REGISTRY_CONTENT.empty()) return;

    try
    {
        auto j = nlohmann::json::parse(REGISTRY_CONTENT);
        items = j.get<std::vector<StoreItem>>();
        std::cout << "[RegistryManager] Parsed " << items.size() << " items successfully." << std::endl;
        for (const auto& item : items)
        {
            std::cout << "  - " << item.id << " (" << item.name << "): longDescription len = " << item.longDescription.size() << std::endl;
        }
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
}