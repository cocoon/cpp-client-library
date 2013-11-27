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

	void Release()
	{
		m_data.resize(0);
		m_data.shrink_to_fit();
	}

	template<typename T>
	T * CastAllocAtEnd(size_t length)
	{
		Grow(length);
		return Cast<T>(Size() - length);
	}

	void Grow(size_t length)
	{
		Resize(Size() + length);
	}

	void Copy(size_t length, const Data &data)
	{
		if(Size() < data.Size())
			throw std::logic_error("Not enough room to copy bytes");

		Copy(length, data.Cast<uint8_t>(0, length));
	}

	void Copy(size_t length, const void *data)
	{
		memcpy(Cast<uint8_t>(), data, length);
	}

	template<typename T>
	const T * Cast(size_t offset = 0, size_t expectedSize = 0) const
	{
		if(Size() - offset < expectedSize)
			throw std::logic_error("Bad cast");

		return reinterpret_cast<const T *>(&m_data[offset]);
	}

	template<typename T>
	T * Cast(size_t offset = 0, size_t expectedSize = 0)
	{
		if(Size() - offset < expectedSize)
			throw std::logic_error("Bad cast");

		return reinterpret_cast<T *>(&m_data[offset]);
	}

	bool IsEmpty() const { return m_data.empty(); }

	std::vector<uint8_t>::iterator begin() { return m_data.begin(); }
	std::vector<uint8_t>::iterator end() { return m_data.end(); }

	std::vector<uint8_t>::const_iterator begin() const { return m_data.begin(); }
	std::vector<uint8_t>::const_iterator end() const { return m_data.end(); }

protected:
	std::vector<uint8_t> m_data;
};

}
