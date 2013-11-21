#pragma once

namespace CopyExample
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
}

