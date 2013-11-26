#pragma once

namespace Copy {

class Data
{
public:
	Data() {}

	size_t Size() const { return m_data.size(); }
	void Resize(size_t size) { m_data.resize(size); }

	size_t PtrToOffset(void *ptr)
	{
		// @@ TODO
		return 0; 
	}

	std::string ToString()
	{
		// @@ TODO
		return std::string();
	}

	static Data FromString(const std::string &string)
	{
		// @@ TODO
		return Data();
	}

	std::string CreateFingerprint()
	{
		// @@ TODO
		return std::string();
	}

	void Release()
	{
		// @@ TODO
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
