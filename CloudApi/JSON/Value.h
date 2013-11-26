/*
 * File Value.h part of the SimpleJSON Library - http://mjpa.in/json
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

#pragma once

namespace Copy {
	namespace JSON {

class Value
{
public:
	friend class Object;
	Value();
	Value(const std::string &value);
	Value(uint64_t value);
	Value(const Array &value);
	Value(const Object &value);
	Value(const Value &value);

	static ValuePtr Create(const std::string &value);
	static ValuePtr Create(const std::vector<std::string> &vectorOfStrings);
	static ValuePtr Create(uint64_t value);
	static ValuePtr Create(const Array &value);

	static ValuePtr Create(const Object &value);
	static ValuePtr Create(const ValuePtr &value);

	static ValuePtr CreateNull();

	bool IsNull() const;
	bool IsString() const;
	bool IsBool() const;
	bool IsNumber() const;
	bool IsArray() const;
	bool IsObject() const;
	
	std::string AsString() const;
	bool AsBool() const;
	uint64_t AsNumber() const;
	const Array &AsArray() const;
	const Object &AsObject() const;
	
	std::string Stringify(bool prettify=false) const;
	
	Type GetType() const;

	static ValuePtr Parse(const char **data);

private:
	static std::string StringifyString(const std::string &str);
	static void PrettifyObjectHelper(const Value &node, std::string &output, int level);
	
	Type m_type;
	std::string m_string;
	bool m_bool;
	uint64_t m_number;
	Array m_array;
	Object m_object;
};

	}
}
