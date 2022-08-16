#pragma once

namespace Utils::Json
{
	std::string TypeToString(nlohmann::json::value_t type);

	unsigned long ReadFlags(const std::string binaryFlags, const size_t size);
}
