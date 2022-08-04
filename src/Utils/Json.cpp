#include <STDInclude.hpp>

namespace Utils::Json
{
	std::string TypeToString(nlohmann::json::value_t type)
	{
		switch (type)
		{
		case nlohmann::json::value_t::null:
			return "null";
		case nlohmann::json::value_t::number_integer:
			return "number_integer";
		case nlohmann::json::value_t::number_unsigned:
			return "number_unsigned";
		case nlohmann::json::value_t::number_float:
			return "number_float";
		case nlohmann::json::value_t::boolean:
			return "boolean";
		case nlohmann::json::value_t::string:
			return "string";
		case nlohmann::json::value_t::array:
			return "array";
		case nlohmann::json::value_t::object:
			return "object";
		default:
			return "null";
		}
	}


	int ReadFlags(const std::string binaryFlags)
	{
		int result = 0x00;
		size_t size = sizeof(int) * 8;

		if (binaryFlags.size() > size) {
			Components::Logger::Print("Flag {} might not be properly translated, it seems to contain an error (invalid length)\n", binaryFlags);
			return result;
		}

		size_t i = size - 1;
		for (char bit : binaryFlags)
		{
			if (i < 0)
			{
				// Uhmm
				Components::Logger::Print("Flag {} might not be properly translated, it seems to contain an error (invalid length)\n", binaryFlags);
				break;
			}

			bool isOne = bit == '1';
			result |= isOne << i;
			i--;
		}

		return result;
	}

}
