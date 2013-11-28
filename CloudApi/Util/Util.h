#pragma once

namespace Copy {

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

	inline std::string HexDump(const Data &data)
	{
		std::string result;

		auto _hexDigit = [](uint8_t c)->char
			{
				if(c < 10)
					return c + '0';
				return c + 'a' - 10;
			};

		for(auto &chr : data)
		{
			result.push_back(_hexDigit(chr >> 4));
			result.push_back(_hexDigit(chr & 0xf));
		}

		return result;
	}

	/**
	 * CreateFingerprint - Fingerprints a chunk of data,
	 * a fingerprint is an md5+sha1
	 */
	inline std::string CreateFingerprint(const Data &data)
	{
		std::string result;

		MD5_CTX md5Ctx;
		MD5_Init(&md5Ctx);
		MD5_Update(&md5Ctx, data.Cast<uint8_t>(), data.Size());
		Data md5digest(16);
		MD5_Final(md5digest.Cast<uint8_t>(0, 16), &md5Ctx);
		result += HexDump(md5digest);

		SHA_CTX sha1Ctx;
		SHA1_Init(&sha1Ctx);
		SHA1_Update(&sha1Ctx, data.Cast<uint8_t>(), data.Size());
		Data sha1digest(20);
		SHA1_Final(sha1digest.Cast<uint8_t>(0, 20), &sha1Ctx);

		result += HexDump(sha1digest);

		return result;
	}

	/**
	 * GetFileFromPath - Given a path of / seperated components, returns
	 * the left most component
	 */
	inline std::string GetFileFromPath(const std::string &path)
	{
		// Locate the first instance of a path seperator
		auto location = path.rfind('/');
		if(location == std::string::npos)
			return path;

		return path.substr(location + 1);
	}

	/**
	 * GetParentFromPath - Given a path of / seperated components, removes
	 * the left most component, and the leaf /
	 */
	inline std::string GetParentFromPath(const std::string &path)
	{
		// Locate the first instance of a path seperator
		auto location = path.rfind('/');
		if(location == std::string::npos)
			return path;

		return path.substr(0, location);
	}
}

