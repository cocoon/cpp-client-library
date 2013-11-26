#pragma once

#ifdef ORDER_BIG_ENDIAN
#define __IS_BIG_ENDIAN__ true
#else
#define __IS_BIG_ENDIAN__ false
#endif

namespace Copy {

/**
 * StructParser - The variable structure class parses a memory
 *	buffer composed of variable length structures
 */
class StructParser 
{
public:
	StructParser();
	StructParser(uint32_t structureSizeOffset, const void *data, 
			uint32_t dataLen, bool sizeFixed = false, bool bigEndian = __IS_BIG_ENDIAN__);
	~StructParser() ;

	uint32_t GetCount() const;
	void Initialize(uint32_t, const void *data, uint32_t dataLen, bool sizeFixed = false,
			bool bigEndian = __IS_BIG_ENDIAN__);

	template <typename T> 
	T * GetEntry(uint32_t index) const;

	template <typename T>
	T * GetNextEntry();

	uint32_t GetCurrentIndex() const;

	void ResetIndex();

protected:
	uint8_t const *m_data;
	uint32_t m_dataLen;
	uint32_t m_structureSizeOffset;
	bool m_sizeFixed;

	uint32_t m_currentOffset;
	uint32_t m_currentIndex;
	uint32_t m_count;

	std::vector<const void *> m_indexToPtrMap;
};

/**
 * StructParser - Default constructor
 */
inline StructParser::StructParser() :
	m_currentOffset(0), m_structureSizeOffset(0), m_currentIndex(0),  m_data(nullptr), m_dataLen(0), m_sizeFixed(false)
{
}

/**
 * ~StructParser - Deconstructor
 */
inline StructParser::~StructParser()
{
}

/**
 * StructParser - Default constructor
 */
inline StructParser::StructParser(uint32_t structureSizeOffset, const void *data, uint32_t dataLen,
		bool sizeFixed, bool bigEndian)
{
	Initialize(structureSizeOffset, data, dataLen, sizeFixed, bigEndian);
}

/**
 * Initialize - Initializses new index at offsets
 */
inline void StructParser::Initialize(uint32_t structureSizeOffset, const void *data,
		uint32_t dataLen, bool sizeFixed, bool bigEndian)
{
	m_currentOffset = 0;
	m_currentIndex = 0;
	m_structureSizeOffset = structureSizeOffset;
	m_data = static_cast<const uint8_t *>(data);
	m_dataLen = dataLen;
	m_count = 0;
	m_sizeFixed = sizeFixed;

	m_indexToPtrMap.clear();

	// While there is data to process, skip past sizes
	uint32_t size;
	while (m_currentOffset + (m_sizeFixed ? 0 : m_structureSizeOffset) < m_dataLen)
	{
		// Stash this index for later
		m_indexToPtrMap.push_back(&m_data[m_currentOffset]);

		if(m_sizeFixed)
		{
			m_currentOffset += m_structureSizeOffset;

		}
		else
		{
			size = *((uint32_t *)&m_data[m_currentOffset + m_structureSizeOffset]);

			if(bigEndian != __IS_BIG_ENDIAN__)
				size = BSWAP_32(size);

			if(!size)
				throw std::logic_error("Failed to parse size of next entry");

			// Skip past this next structure
			m_currentOffset += size;
		}

		m_count++;
	}

	// Detect possible memory overflow condition
	if(m_currentOffset > m_dataLen)
		throw std::logic_error("Current offset is higher then actual data");
}

/**
 * GetCount - Returns the count of entries
 */
inline uint32_t StructParser::GetCount() const
{
	return m_count;
}

/**
 * GetNextEntry - Returns the next entry
 */
template <typename T>
inline T * StructParser::GetNextEntry()
{
	return GetEntry<T>(m_currentIndex++);
}

/**
 * GetCurrentIndex - Returns the current index
 */
inline uint32_t StructParser::GetCurrentIndex() const
{
	return m_currentIndex;
}

/**
 * ResetIndex - Resets next entry index
 */
inline void StructParser::ResetIndex()
{
	m_currentIndex = 0;
}

/**
 * GetEntry - Returns the entry at index
 */
template <typename T>
inline T * StructParser::GetEntry(uint32_t index) const
{
	if(index >= m_indexToPtrMap.size())
		return nullptr;
	
	return (T *)m_indexToPtrMap[index];
}

}
