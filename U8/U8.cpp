#include "Common.h"
#include "U8.h"

using namespace CopyExample;

// Table defining lengths of character sequences in UTF-8
static uint8_t g_u8CharSize[256] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x00
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x10
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x20
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x30
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x50
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x60
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x70
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x80	Illegal
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x90	Illegal
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0xA0	Illegal
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0xB0	Illegal
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // 0xC0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // 0xD0
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // 0xE0
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1      // 0xF0
};

/**
 * CharSize - This function will return the number of bytes required for the
 *	given UTF-8 character.
 */
uint32_t U8::CharSize(const char *text)
{
	return g_u8CharSize[static_cast<uint8_t>(text[0])];
}

/**
 * NextChar - Proceeds to the next logical utf8 character
 */
char *U8::NextChar(const char *str)
{
	return const_cast<char *>(&str[CharSize(str)]);
}

/**
 * StringLength - Returns the logical character count in a utf8 string
 */
uint32_t U8::StringLength(const char *str)
{
	uint32_t count = 0;

	while(*str)
	{
		count++;
		str = NextChar(str);
	}

	return count;
}

/**
 * CompareNoCase - This function will compare two strings and return 0 if the two
 *	strings are identical, without comparing case (for ascii characters only),
 * -1 if str1 is < str2, or 1 if str1 > str2.
 */
int U8::CompareNoCase(const char *str1, const char *str2, uint32_t count)
{
	char p1, p2;

	// Loop until a difference found or end of string
	while(*str1 && count)
	{
		p1 = *str1;
		p2 = *str2;

		if(p1 >= 'A' && p1 <= 'Z')
			p1 += 'a' - 'A';
		if(p2 >= 'A' && p2 <= 'Z')
			p2 += 'a' - 'A';

		if(p1 < p2)
			return -1;
		else if(p1 > p2)
			return 1;

		str1++;
		str2++;
		count--;
	}

	// If end of the line in terms of allowed comparison characters,
	// string was the same up to the point we compared
	if(!count)
		return 0;

	if(*str1 < *str2)
		return -1;
	else if(*str1 > *str2)
		return 1;
	else
		return 0;
}

/**
 * Compare - This function will compare two strings up to a given position and
 *	return 0 if the two strings are identical, -1 if str1 is < str2, or 1
 *	if str1 > str2.
 */
int U8::Compare(const char *str1, const char *str2, uint32_t count)
{
	uint32_t size;

	// Loop until a difference found, end of string, or count exhausted
	while(*str1 && count)
	{
		// Get the size of this chr
		size = CharSize(str1);

		// While we have bytes in the character to compare
		while(size)
		{
			// If this character is different
			if(*str1 != *str2)
				break;

			// One less byte in this character
			size--;

			// Point to the next byte
			str1++;
			str2++;
		}

		// If this chr is different, stop
		if(size)
			break;

		// One less character needed
		count--;
	}

	// If we ran out of chrs, then they are equal up to this point
	if(!count)
		return 0;

	// And determine what to return
	if(*str1 < *str2)
		return -1;
	else if(*str1 > *str2)
		return 1;
	else
		return 0;
}
