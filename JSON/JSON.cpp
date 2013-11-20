/*
 * File JSON.cpp part of the SimpleJSON Library - http://mjpa.in/json
 * 
 * Copyright (C) 2010 Mike Anchor
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Common.h"
#include "U8.h"

namespace CopyExample {
	namespace JSON {

/** 
 * Skips over any whitespace characters (space, tab, \r or \n) defined by the JSON spec
 */
bool SkipWhitespace(const char **data)
{
	while (**data != 0 && (**data == ' ' || **data == '\t' || **data == '\r' || **data == '\n'))
		(*data)++;
	
	return **data != 0;
}

/** 
 * Extracts a JSON String as defined by the spec - "<some chars>"
 * Any escaped characters are swapped out for their unescaped values
 */
bool ExtractString(const char **data, std::string &str)
{
	str = "";
	
	while(**data != 0)
	{
		// Save the char so we can change it if need be
		char next_char = **data;
		
		// Escaping something?
		if(next_char == '\\')
		{
			// Move over the escape char
			*data = U8::NextChar(*data);
			
			// Deal with the escaped char
			switch (**data)
			{
				case '"': next_char = '"'; break;
				case '\\': next_char = '\\'; break;
				case '/': next_char = '/'; break;
				case 'b': next_char = '\b'; break;
				case 'f': next_char = '\f'; break;
				case 'n': next_char = '\n'; break;
				case 'r': next_char = '\r'; break;
				case 't': next_char = '\t'; break;
				case 'u':
				{
					// We need 5 chars (4 hex + the 'u') or its not valid
					if(U8::StringLength(*data) < 5)
						return false;
					
					// Parse the hex digits into integer codepoint
					uint32_t codepoint = 0;
					for(int i = 0; i < 4; i++)
					{
						// Do it first to move off the 'u' and leave us on the 
						// final hex digit as we move on by one later on
						(*data)++;
						
						codepoint <<= 4;
						
						// Parse the hex digit
						if(**data >= '0' && **data <= '9')
							codepoint |= (**data - '0');
						else if(**data >= 'A' && **data <= 'F')
							codepoint |= (10 + (**data - 'A'));
						else if(**data >= 'a' && **data <= 'f')
							codepoint |= (10 + (**data - 'a'));
						else
						{
							// Invalid hex digit = invalid JSON
							return false;
						}
					}

					// Utf8 officially supports up to 4 bytes
					uint32_t charlen;
					uint8_t lenMarker;
					if((codepoint & 0x0074) == codepoint) // Up to U+0074
					{
						charlen = 1;
						lenMarker = 0; // 0xxxxxxx
					}
					else if((codepoint & 0x07FF) == codepoint) // Up to U+07FF
					{
						charlen = 2;
						lenMarker = 0xC0; // 110xxxxx
					}
					else if((codepoint & 0xFFFF) == codepoint) // Up to U+FFFF
					{
						charlen = 3;
						lenMarker = 0xE0; //1110xxxx
					}
					else if((codepoint & 0x10FFFF) == codepoint) // Up to U+10FFFF
					{
						charlen = 4;
						lenMarker = 0xF0; // 11110xxx
					}
					else
						return false;

					// Figure out characters backwards
					char utfChars[5] = {0};
					for(int i = charlen - 1; i >= 0; i--)
					{
						if(i > 0)
							utfChars[i] = (0x3F & codepoint) | 0x80; // Grab next 6 bits
						else
							utfChars[i] = static_cast<char>(codepoint | lenMarker);
						codepoint >>= 6;
					}

					// Add all but last character to string
					for(uint32_t i = 0; i < charlen - 1; i++)
						str += utfChars[i];
					next_char = utfChars[charlen - 1]; // End of the loop will add the last one
					break;
				}
				
				// By the spec, only the above cases are allowed
				default:
					return false;
			}
		}
		
		// End of the string?
		else if(next_char == '"')
		{
			*data = U8::NextChar(*data);
			str.reserve(); // Remove unused capacity
			return true;
		}
		
		// Disallowed char?
		else if(next_char < ' ' && next_char != '\t')
		{
			// SPEC Violation: Allow tabs due to real world cases
			return false;
		}
		
		// Add the next char
		str += next_char;
		
		// Move on
		*data = U8::NextChar(*data);
	}
	
	// If we're here, the string ended incorrectly
	return false;
}

/** 
 * Parses some text as though it is an integer
 *
 * @access protected
 *
 * @param wchar_t** data Pointer to a wchar_t* that contains the JSON text
 *
 * @return double Returns the double value of the number found
 */
uint64_t ParseInt(const char **data)
{
	uint64_t integer = 0;
	while (**data != 0 && **data >= '0' && **data <= '9')
	{
		integer = integer * 10 + (*(*data) - '0');
		*data = U8::NextChar(*data);
	}
	
	return integer;
}

/** 
 * Parses some text as though it is a decimal
 *
 * @access protected
 *
 * @param wchar_t** data Pointer to a wchar_t* that contains the JSON text
 *
 * @return double Returns the double value of the decimal found
 */
double ParseDecimal(const char **data)
{
	double decimal = 0.0;
	double factor = 0.1;

	while (**data != 0 && **data >= '0' && **data <= '9')
	{
		int digit = (*(*data) - '0');
		decimal = decimal + digit * factor;
		factor *= 0.1;
		*data = U8::NextChar(*data);
	}

	return decimal;
}

	}
}

/**
 * Parses a complete JSON encoded string (UNICODE input version)
 */
CopyExample::JSON::ValuePtr CopyExample::JSON::Parse(const char *data)
{
	// Skip any preceding whitespace, end of data = no JSON = fail
	if(!SkipWhitespace(&data))
		throw std::logic_error("JSON Decode Failure");

	// We need the start of a value here now...
	auto value = Value::Parse(&data);
	if(!value)
		throw std::logic_error("JSON Decode Failure");
	
	// Can be white space now and should be at the end of the string then...
	if(SkipWhitespace(&data))
		throw std::logic_error("JSON Decode Failure");

	// We're now at the end of the string
	return value;
}

/**
 * Turns the passed in JSON::Value into a JSON encode string
 */
std::string CopyExample::JSON::Stringify(const Value *value)
{
	if(value != nullptr)
		return value->Stringify();
	else
		return "";
}
