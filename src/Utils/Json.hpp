#pragma once

namespace Utils::Json
{
	std::string TypeToString(nlohmann::json::value_t type);
	int ReadFlags(const std::string binaryFlags);
}
