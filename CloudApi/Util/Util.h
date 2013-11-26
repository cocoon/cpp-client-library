#pragma once

namespace Copy
{
	inline std::pair<std::string, std::string> SplitString(std::string s, const std::string &delim) 
	{
		auto position = s.find(delim);
		if(position == std::string::npos)
			return std::make_pair(s, "");

		std::pair<std::string, std::string> result;

		result.second = s.substr(position + delim.size());
		s.resize(s.size() - position + delim.size());
		result.first = s;

		return result;
	}

	inline std::string PrettySize(uint64_t bytes)
	{
		if(bytes > 1024 * 1024 * 1000)
		{
			bytes /= 1024 * 1024 * 1000;
			return std::to_string(bytes) + "GB";
		}
		else if(bytes > 1024 * 1024)
		{
			bytes /= 1024 * 1024;
			return std::to_string(bytes) + "MB";
		}
		else if(bytes > 1024)
		{
			bytes /= 1024;
			return std::to_string(bytes) + "kB";
		}
		else
			return std::to_string(bytes) + "B";
	}

	inline std::vector<uint8_t> ToData(const std::string &string)
	{
		// @@ TODO
		return std::vector<uint8_t>();
	}
}

