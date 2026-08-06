#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <type_traits>
#include <stdexcept>
#include <cstdlib>

template<typename T>
std::map<std::string, T> parseCsvToMap(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open " + path.string());

	std::string line;

	std::map<std::string, T> keyValuePair;
    
	// One line at a time
	while (std::getline(file, line))
	{
		// if empty or starts with #, continue, write comments with #
		if (line.empty() || line[0] == '#')
			continue;

		// find comma pos
		size_t commaPos = line.find(',');

		// If it has a comma then do this
		if (commaPos != std::string::npos)
		{
			std::string key = line.substr(0, commaPos);
			std::string value = line.substr(commaPos + 1);

			// Clean trailing and leading spaces/carriage returns
			auto trim = [](std::string &s)
			{
				while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
				{
					s.pop_back();
				}
				size_t start = s.find_first_not_of(" \t\r\n");
				if (start != std::string::npos)
				{
					s = s.substr(start);
				}
			};

			trim(key);
			trim(value);

            T converted{};

            if constexpr (std::is_same_v<T, bool>)
			{
				if (value == "true")
					converted = true;
				else if (value == "false")
					converted = false;
                else
                {
                    throw std::runtime_error("Invalid boolean: " + value);
                }
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				converted = value;
                //explicit thats why
			}
			else if constexpr (std::is_arithmetic_v<T>)
			{
				char *endptr = nullptr;
				double parsed = std::strtod(value.c_str(), &endptr);

                if (endptr == value.c_str() || *endptr != '\0')
                {
                    throw std::runtime_error("Invalid number: " + value);
                }

				converted = static_cast<T>(parsed);
				
			}
			else
            {
                std::stringstream ss(value);

                if (!(ss >> converted))
                {
                    throw std::runtime_error("Failed to parse value: " + value);
                }
            }

			keyValuePair[key] = converted;
		}
	}

    return keyValuePair;
}