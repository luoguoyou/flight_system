#pragma once

#include <string>
#include <vector>

std::string escapeField(const std::string& value);
std::string unescapeField(const std::string& value);
std::vector<std::string> splitPacket(const std::string& packet);
std::string makePacket(const std::vector<std::string>& fields);
