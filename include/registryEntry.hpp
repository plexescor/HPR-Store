#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

enum class StoreItemType {
    EXTENSION,
    THEME
};

struct StoreItem {
    std::string id;
    std::string name;
    std::string author;
    std::string authorGithub;
    std::string description;
    std::string version;
    std::string downloadUrl;
    std::string sourceUrl;
    StoreItemType type;
    std::vector<std::string> tags;
    std::vector<std::string> previewImages;
    std::vector<std::string> supportedHPRVersions;
    int downloadCount = 0;
    int starCount = 0;
    std::string lastUpdated;
    
    bool operator==(const StoreItem& other) const 
    {
        return id == other.id;
    }
};

struct InstalledItem {
    std::string id;
    std::string version;
    std::string installedDate;
    std::string sourceUrl;
    std::filesystem::path installPath;
};