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

namespace CopyExample {
	namespace JSON {

class Value;

class Object 
{
public:
	friend class Value;

	Object();
	Object(const std::string &jsonPayload);
	Object(Object &&array);
	Object(const Object &array);

	typedef std::function<void (Object &object)> IterateObjectCallback;

	template <class T>
	inline T Get(const std::string &key) const;

	template <class T>
	inline T GetOpt(const std::string &key, const T &defaultValue) const;

	inline Type GetType(const std::string &key) const;

	inline bool Has(const std::string &key) const;

	template<class T>
	ValuePtr Set(const std::string &key, const T &_value);

	template<class T>
	ValuePtr Set(const std::string &key, const T &&_value);

	operator std::string();
	std::string AsString();

	uint32_t Size() const { return static_cast<uint32_t>(m_fields.size()); }
	bool IsEmpty() const { return m_fields.empty(); }
	void Clear() { m_fields.clear(); }
	uint32_t Erase(const std::string &key) { return static_cast<uint32_t>(m_fields.erase(key)); }

	Object & operator = (const Object &object);
	Object & operator = (Object &&object);

protected:

	ValuePtr Find(const std::string &key) const;
	ValuePtr FindOpt(const std::string &key) const;
	void Put(const std::string &key, const ValuePtr &value);

	static void IterateObjects(ValuePtr value, IterateObjectCallback callback);

private:

	std::map<std::string, ValuePtr> m_fields;
};
	}
}
