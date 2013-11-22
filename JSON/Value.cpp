/*
 * File Value.cpp part of the SimpleJSON Library - http://mjpa.in/json
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

using namespace CopyExample::JSON;
using namespace CopyExample;

/**
 * Parses a JSON encoded value to a Value object
 *
 * @access protected
 *
 * @param wchar_t** data Pointer to a wchar_t* that contains the data
 *
 * @return Value* Returns a pointer to a Value object on success, NULL on error
 */
ValuePtr Value::Parse(const char **data)
{
	// Is it a string?
	if(**data == '"')
	{
		std::string str;
		if(!ExtractString(&(++(*data)), str))
			throw std::logic_error("Json Decode Failure");
		else
			return std::make_shared<Value>(str);
	}
	// Is it a boolean?
	else if((U8::CompareNoCase(*data, "true", 4) == 0) || (U8::CompareNoCase(*data, "false", 5) == 0))
	{
		bool value = U8::CompareNoCase(*data, "true", 4) == 0;
		(*data) += value ? 4 : 5;
		return std::make_shared<Value>(value);
	}
	
	// Is it a null?
	else if(U8::CompareNoCase(*data, "null", 4) == 0)
	{
		(*data) += 4;
		return std::make_shared<Value>();
	}
	
	// Is it a number?
	else if(**data == '-' || (**data >= '0' && **data <= '9'))
	{
		// Negative?
		bool neg = **data == '-';
		if(neg)
			(*data)++;

		uint64_t number = 0;

		// Parse the whole part of the number - only if it wasn't 0
		if(**data == '0')
			(*data)++;
		else if(**data >= '1' && **data <= '9')
			number = ParseInt(data);
		else
			throw std::logic_error("Failed to decode json");
		
		// Could be a decimal now...
		if(**data == '.')
		{
			(*data)++;

			// Not get any digits?
			if(!(**data >= '0' && **data <= '9'))
				throw std::logic_error("Failed to decode json");
			
			// Find the decimal and sort the decimal place out
			// Use ParseDecimal as ParseInt won't work with decimals less than 0.1
			// thanks to Javier Abadia for the report & fix
			double decimal = ParseDecimal(data);
			
			// @@ TODO: Realistically this will do nothing, and we don't
			// really support floating point numbers in JSON
			number += static_cast<uint64_t>(decimal);
		}

		// Could be an exponent now...
		if(**data == 'E' || **data == 'e')
		{
			(*data)++;

			// Check signage of expo
			bool neg_expo = false;
			if(**data == '-' || **data == '+')
			{
				neg_expo = **data == '-';
				(*data)++;
			}
			
			// Not get any digits?
			if(!(**data >= '0' && **data <= '9'))
				throw std::logic_error("Failed to decode json");

			// @@ TODO: Realistically this will do nothing, and we don't
			// really support floating point numbers in JSON
			uint64_t expo = ParseInt(data);
			for(double i = 0.0; i < expo; i++)
				number = static_cast<uint64_t>(neg_expo ? (number / 10.0) : (number * 10.0));
		}

		// Was it neg?
		if(neg) 
			number *= -1;

		// @@ TODO support decimal points (maybe never)
		return std::make_shared<Value>(static_cast<uint64_t>(number));
	}

	// An object?
	else if(**data == '{')
	{
		Object object;
		
		(*data)++;
	
		while(**data != 0)
		{
			// Whitespace at the start?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// Special case - empty object
			if(object.Size() == 0 && **data == '}')
			{
				(*data)++;
				return std::make_shared<Value>(object);
			}
			
			// We want a string now...
			std::string name;
			if(!ExtractString(&(++(*data)), name))
				throw std::logic_error("Failed to decode json");
			
			// More whitespace?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// Need a : now
			if(*((*data)++) != ':')
				throw std::logic_error("Failed to decode json");
			
			// More whitespace?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// The value is here			
			ValuePtr value = Parse(data);
			if(!value)
				throw std::logic_error("Failed to decode json");
			
			// Add the name:value
			object.Put(name, value);
			
			// More whitespace?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// End of object?
			if(**data == '}')
			{
				(*data)++;
				return std::make_shared<Value>(object);
			}
			
			// Want a , now
			if(**data != ',')
				throw std::logic_error("Failed to decode json");
			
			(*data)++;
		}
		
		// We ran out of data
		throw std::logic_error("Failed to decode json");
	}
	
	// An array?
	else if(**data == '[')
	{
		Array array;
		
		(*data)++;
		
		while(**data != 0)
		{
			// Whitespace at the start?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// Special case - empty array
			if(array.size() == 0 && **data == ']')
			{
				(*data)++;
				return std::make_shared<Value>(array);
			}
			
			// Get the value
			auto value = Parse(data);
			if(!value)
				throw std::logic_error("Failed to decode json");
			
			// Add the value
			array.push_back(value);
			
			// More whitespace?
			if(!SkipWhitespace(data))
				throw std::logic_error("Failed to decode json");
			
			// End of array?
			if(**data == ']')
			{
				(*data)++;
				return std::make_shared<Value>(array);
			}
			
			// Want a , now
			if(**data != ',')
				throw std::logic_error("Failed to decode json");
			
			(*data)++;
		}
		
		// We ran out of data
		throw std::logic_error("Failed to decode json");
	}
	
	// Ran out of possibilites, it's bad!
	else
		throw std::logic_error("Failed to decode json");
}

/** 
 * Basic constructor for creating a JSON Value of type NULL
 *
 * @access public
 */
Value::Value()
{
	m_type = Type_Null;
}

/** 
 * Constructor from std::string to solve ambiguity between bool and wchar_t *
 *
 * @access public
 *
 * @param std::string value The std::string to use as the value
 */
Value::Value(const std::string &value)
{
	m_type = Type_String;
	m_string = value;
}

/** 
 * Basic constructor for creating a JSON Value of type Number
 *
 * @access public
 *
 * @param uint64_t value The number to use as the value
 */
Value::Value(uint64_t value)
{
	m_type = Type_Number;
	m_number = value;
}

/** 
 * Basic constructor for creating a JSON Value of type Array
 *
 * @access public
 *
 * @param Array value The Array to use as the value
 */
Value::Value(const Array &value)
{
	m_type = Type_Array;
	m_array = value;
}

/** 
 * Basic constructor for creating a JSON Value of type Object
 *
 * @access public
 *
 * @param Object value The Object to use as the value
 */
Value::Value(const Object &value)
{
	m_type = Type_Object;
	m_object = value;
}

Value::Value(const Value &value)
{
	m_type = value.m_type;

	switch (m_type)
	{
		case Type_String:
			m_string = value.m_string;
			break;
		case Type_Bool:
			m_bool = value.m_bool;
			break;
		case Type_Number:
			m_number = value.m_number;
		case Type_Array:
			m_array = value.m_array;
		case Type_Object:
			// @@ TODO: if any shared pointers to other values are contained within,
			// we'll only be doing a shallow copy
			m_object = value.m_object;
			break;
	}
}

/** 
 * Checks if the value is a NULL
 *
 * @access public
 *
 * @return bool Returns true if it is a NULL value, false otherwise
 */
bool Value::IsNull() const
{
	return m_type == Type_Null;
}

/** 
 * Checks if the value is a String
 *
 * @access public
 *
 * @return bool Returns true if it is a String value, false otherwise
 */
bool Value::IsString() const
{
	return m_type == Type_String;
}

/** 
 * Checks if the value is a Bool
 *
 * @access public
 *
 * @return bool Returns true if it is a Bool value, false otherwise
 */
bool Value::IsBool() const
{
	return m_type == Type_Bool;
}

/** 
 * Checks if the value is a Number
 *
 * @access public
 *
 * @return bool Returns true if it is a Number value, false otherwise
 */
bool Value::IsNumber() const
{
	return m_type == Type_Number;
}

/** 
 * Checks if the value is an Array
 *
 * @access public
 *
 * @return bool Returns true if it is an Array value, false otherwise
 */
bool Value::IsArray() const
{
	return m_type == Type_Array;
}

/** 
 * Checks if the value is an Object
 *
 * @access public
 *
 * @return bool Returns true if it is an Object value, false otherwise
 */
bool Value::IsObject() const
{
	return m_type == Type_Object;
}

/** 
 * Retrieves the String value of this Value
 * Use IsString() before using this method.
 *
 * @access public
 *
 * @return std::wstring Returns the string value
 */
std::string Value::AsString() const
{
	return m_string;
}

/** 
 * Retrieves the Bool value of this Value
 * Use IsBool() before using this method.
 *
 * @access public
 *
 * @return bool Returns the bool value
 */
bool Value::AsBool() const
{
	return m_bool;
}

/** 
 * Retrieves the Number value of this Value
 * Use IsNumber() before using this method.
 *
 * @access public
 *
 * @return uint64_t Returns the number value
 */
uint64_t Value::AsNumber() const
{
	return m_number;
}

/** 
 * Retrieves the Array value of this Value
 * Use IsArray() before using this method.
 *
 * @access public
 *
 * @return Array Returns the array value
 */
const Array &Value::AsArray() const
{
	return m_array;
}

/** 
 * Retrieves the Object value of this Value
 * Use IsObject() before using this method.
 *
 * @access public
 *
 * @return Object Returns the object value
 */
const Object &Value::AsObject() const
{
	return m_object;
}

/** 
 * Creates a JSON encoded string for the value with all necessary characters escaped
	takes a bool to toggle pretty-printing of Value (defaults to false)
 *
 * @access public
 *
 * @return std::wstring Returns the JSON string
 */
std::string Value::Stringify(bool prettify) const
{
	std::string ret_string;
	
	switch (m_type)
	{
		case Type_Null:
			ret_string = "null";
			break;
		
		case Type_String:
			ret_string = StringifyString(m_string);
			break;
		
		case Type_Bool:
			ret_string = m_bool ? "true" : "false";
			break;
		
		case Type_Number:
		{
			ret_string = std::to_string(m_number);
			break;
		}
		
		case Type_Array:
		{
			ret_string = "[";
			if(prettify)
				ret_string += "\n";

			Array::const_iterator iter = m_array.begin();
			while(iter != m_array.end())
			{
				ret_string += (*iter)->Stringify();
				
				// Not at the end - add a separator
				if(++iter != m_array.end())
					ret_string += ",";

				if(prettify)
					ret_string += "\n";
			}
			ret_string += "]";
			break;
		}
		
		case Type_Object:
		{
			if(prettify)
			{	
				//TODO: should probably use get_shared_from_this()
				PrettifyObjectHelper(*this, ret_string, 0);
				break;
			}

			ret_string = "{";
			auto iter = m_object.m_fields.begin();
			while(iter != m_object.m_fields.end())
			{
				ret_string += StringifyString((*iter).first);
				ret_string += ":";
				ret_string += (*iter).second->Stringify();
				
				// Not at the end - add a separator
				if(++iter != m_object.m_fields.end())
					ret_string += ",";
			}
			ret_string += "}";
			break;
		}
	}

	return ret_string;
}

/** 
 * Modifies passed std::string to create a pretty-printed version of a Value that is a Object.
 *
 * @access public
 *
 * @return void
 */
void Value::PrettifyObjectHelper(const Value &node, std::string &output, int level)
{
	// let stringify handle formatting
	// of non-object nodes
	if(!node.IsObject())
	{
		output += node.Stringify();
		return;
	}

	std::string indent;

	for(int i=0; i<level; i++)
		indent += "\t";

	std::string nextIndent = indent + "\t";

	output +="{\n";

	// start iterating from the current node
	auto iter = node.m_object.m_fields.begin();
	while(iter != node.m_object.m_fields.end())
	{
		output += nextIndent + StringifyString(iter->first);
		output += ":";

		std::string nextObj;
		PrettifyObjectHelper(*iter->second, nextObj, level+1);
		output += nextObj;
	
		// Not at the end - add a separator
		if(++iter != node.m_object.m_fields.end())
			output += ",\n";
	}
	output += "\n" + indent + "}";
}

Type Value::GetType() const
{
	return m_type;
}

/** 
 * Creates a JSON encoded string with all required fields escaped
 * Works from http://www.ecma-internationl.org/publications/files/ECMA-ST/ECMA-262.pdf
 * Section 15.12.3.
 *
 * @access private
 *
 * @param std::wstring str The string that needs to have the characters escaped
 *
 * @return std::wstring Returns the JSON string
 */
std::string Value::StringifyString(const std::string &str)
{
	std::string str_out = "\"";
	
	auto iter = static_cast<const char *>(str.c_str());
	while(*iter)
	{
		uint8_t chr = *iter;

		if(chr == '"' || chr == '\\' || chr == '/')
		{
			str_out += '\\';
			str_out += (char)chr;
		}
		else if(chr == '\b')
		{
			str_out += "\\b";
		}
		else if(chr == '\f')
		{
			str_out += "\\f";
		}
		else if(chr == '\n')
		{
			str_out += "\\n";
		}
		else if(chr == '\r')
		{
			str_out += "\\r";
		}
		else if(chr == '\t')
		{
			str_out += "\\t";
		}
		else if(chr < ' ' || chr & 0x80) // Control character or extended character
		{
			uint32_t codepoint;
			uint32_t charlen = U8::CharSize(iter);
			
			// Total character length determines mask of first byte
			if(charlen == 1)
				codepoint = (uint32_t)(chr & 0x7F); // 0xxxxxxx
			else if(charlen == 2)
				codepoint = (uint32_t)(chr & 0x1F); // 110xxxxx
			else if(charlen == 3)
				codepoint = (uint32_t)(chr & 0x0F); // 1110xxxx
			else if(charlen == 4)
				codepoint = (uint32_t)(chr & 0x07); // 11110xxx
			else // Invalid character length
				return str_out + "\"";

			for(uint32_t i = 1; i < charlen; i++)
			{
				if((iter[i] & 0xC0) != 0x80) // All remaining bytes must be of form 10xxxxxx
					return str_out + "\"";
				codepoint = ((codepoint << 6) | (uint32_t)(iter[i] & 0x3F)); // Grab next 6 bits
			}

			str_out += "\\u";
			for(uint32_t i = 0; i < 4; i++)
			{
				uint32_t value = (codepoint >> (12 - 4*i)) & 0xf;
				if(value <= 9)
					str_out += (char)('0' + value);
				else if(value >= 10 && value <= 15)
					str_out += (char)('A' + (value - 10));
			}
		}
		else
		{
			str_out += (char)chr;
		}
		
		iter = U8::NextChar(iter);
	}
	
	str_out += "\"";
	return str_out;
}

ValuePtr Value::Create(const std::string &value)
{
	return std::make_shared<Value>(value);
}

ValuePtr Value::Create(const std::vector<std::string> &vectorOfStrings)
{
	Array arrayValue;
	for(auto &entry : vectorOfStrings)
		arrayValue.push_back(Create(entry));
	return Create(arrayValue);
}

ValuePtr Value::Create(uint64_t value)
{
	return std::make_shared<Value>(value);
}

ValuePtr Value::Create(const Array &value)
{
	return std::make_shared<Value>(value);
}

ValuePtr Value::Create(const Object &value)
{
	return std::make_shared<Value>(value);
}

ValuePtr Value::Create(const ValuePtr &value)
{
	return value;
}

ValuePtr Value::CreateNull()
{
	return std::make_shared<Value>();
}
