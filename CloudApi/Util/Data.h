#pragma once

namespace Copy {

class Data
{
public:
	Data(size_t size) 
	{
		m_data.resize(size);
	}

	Data()
	{
	}

	Data(const std::string &string)
	{
		for(auto &chr : string)
			m_data.push_back(chr);
	}

	size_t Size() const { return m_data.size(); }
	void Resize(size_t size) { m_data.resize(size); }

	size_t PtrToOffset(void *ptr)
	{
		// Check to see if it exists
		if(&m_data[0] > ptr)
			throw std::logic_error("Invalid cast");
		
		auto offset = (size_t) ((uint64_t)ptr - (uint64_t)&m_data[0]);

		if(!offset < Size())
			throw std::logic_error("Invalid cast");
		
		return offset;
	}

	std::string ToString()
	{
		std::string string;
		for(auto &chr : m_data)
			string.push_back(chr);
		return string;
	}

	std::string CreateFingerprint()
	{
		MD5_CTX md5Ctx;
		MD5_Init(&md5Ctx);
		MD5_Update(&md5Ctx, &m_data[0], m_data.size());
		Data md5digest(16);
		MD5_Final(md5digest.Cast<uint8_t>(), &md5Ctx);

		SHA_CTX sha1Ctx;
		SHA1_Init(&sha1Ctx);
		SHA1_Update(&sha1Ctx, &m_data[0], m_data.size());
		Data sha1digest(20);
		SHA1_Final(sha1digest.Cast<uint8_t>(), &sha1Ctx);
	}

	void Release()
	{
		m_data.resize(0);
		m_data.shrink_to_fit();
	}

	template<typename T>
	T * CastAllocAtEnd(size_t length)
	{
		// @@ TODO
		return nullptr;
	}

	void Grow(size_t length)
	{
		// @@ TODO
	}

	void Copy(size_t length, const Data &data)
	{
		// @@ TODO
	}

	void Copy(size_t length, const void *data)
	{
		// @@ TODO
	}

	template<typename T>
	T * Cast(size_t offset = 0, size_t expectedSize = 0)
	{
		// @@ TODO
		return nullptr;
	}

	bool IsEmpty() const { return m_data.empty(); }

protected:
	std::vector<unsigned char> m_data;
};

}
