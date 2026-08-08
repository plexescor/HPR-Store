#include "registryManager.hpp"
#include "network.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <iostream>
#include <string>
#include <print>

RegistryManager::RegistryManager() = default;
RegistryManager::~RegistryManager() = default;

std::string RegistryManager::fetchRegistry()
{
    ix::HttpClient httpClient;
    std::string fullUrl = "https://" + std::string(REGISTRY_URL) + std::string(REGISTRY_PATH);

    auto args = httpClient.createRequest();
    auto response = httpClient.get(fullUrl, args);

    if (response->statusCode == 200)
    {
        REGISTRY_CONTENT = response->body;
    }
    else
    {
        std::cerr << "Failed to fetch registry from " << fullUrl
                  << ". Status code: " << response->statusCode
                  << ", Error: " << response->errorMsg << std::endl;
        REGISTRY_CONTENT.clear();
    }

    std::println("{}", REGISTRY_CONTENT);
    return REGISTRY_CONTENT;
}