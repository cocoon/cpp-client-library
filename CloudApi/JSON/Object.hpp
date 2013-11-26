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

inline Object::Object() 
{
}

inline Object::Object(const std::string &jsonPayload) 
{
	auto value = JSON::Parse(jsonPayload.c_str());
	m_fields = std::move(value->AsObject().m_fields);
}

inline Object::Object(Object &&array) :
		m_fields(std::move(array.m_fields))
{
}

inline Object::Object(const Object &array) :
	m_fields(array.m_fields)
{
}

template <class T>
inline T Object::Get(const std::string &key) const
{
	throw std::logic_error(std::string("Required field missing ") + key);
}

inline Object & Object::operator = (const Object &object)
{
	if(&object == this)
		return *this;

	m_fields = object.m_fields;
	return *this;
}

inline Object & Object::operator = (Object &&object)
{
	m_fields = std::move(object.m_fields);
	return *this;
}

template <>
inline Array Object::Get<Array>(const std::string &key) const
{
	return Find(key)->AsArray();
}

template <>
inline bool Object::Get<bool>(const std::string &key) const
{
	auto value = Find(key);

	if(value->IsBool())
		return value->AsBool();
	else if(value->IsNumber())
		return value->AsNumber();
	else
		return std::stod(value->AsString());
}

template <>
inline ValuePtr Object::Get<ValuePtr>(const std::string &key) const
{
	auto value = Find(key);
	return value;
}

template <>
inline Object Object::Get<Object>(const std::string &key) const
{
	auto value = Find(key);
	return value->AsObject();
}

template <>
inline std::vector<std::string> Object::Get<std::vector<std::string>>(const std::string &key) const
{
	auto value = Find(key);
	std::vector<std::string> result;
	for(auto &entry : value->AsArray())
		result.push_back(entry->AsString());
	return result;
}

template <>
inline std::string Object::Get<std::string>(const std::string &key) const
{
	auto value = Find(key);
	return value->AsString();
}

template <>
inline double Object::Get<double>(const std::string &key) const
{
	auto value = Find(key);
	if(!value->IsNumber() && !value->IsString())
		throw std::logic_error(std::string("Field was not of type json=type Number of String ") + key);

	return std::stod(value->AsString());
}

template <>
inline uint64_t Object::Get<uint64_t>(const std::string &key) const
{
	auto value = Find(key);
	if(!value->IsNumber() && !value->IsString())
		throw std::logic_error(std::string("Field was not of type json=type Number of String ") + key);

	return value->IsNumber() ? static_cast<uint64_t>(value->AsNumber())
		: std::stoll(value->AsString());
}

template <>
inline uint32_t Object::Get<uint32_t>(const std::string &key) const
{
	return static_cast<uint32_t>(Get<uint64_t>(key));
}

template <>
inline uint16_t Object::Get<uint16_t>(const std::string &key) const
{
	return static_cast<uint16_t>(Get<uint64_t>(key));
}

template <class T>
inline T Object::GetOpt(const std::string &key, const T &defaultValue) const
{
	throw std::logic_error(std::string("Unhangled template ") + key);
}

template <>
inline std::string Object::GetOpt<std::string>(const std::string &key, const std::string &defaultValue) const
{
	auto value = FindOpt(key);

	if(!value)
		return defaultValue;

	return value->AsString();
}

template <>
inline double Object::GetOpt<double>(const std::string &key, const double &defaultValue) const
{
	auto value = FindOpt(key);
	if(!value || (!value->IsNumber() && !value->IsString()))
		return defaultValue;

	try
	{
		return std::stod(value->AsString());
	}
	catch(std::exception &)
	{
		return defaultValue;
	}
}

template <>
inline ValuePtr Object::GetOpt<ValuePtr>(const std::string &key, const ValuePtr &defaultObject) const
{
	auto value = FindOpt(key);
	if(!value)
		return defaultObject;

	return value;
}

template <>
inline Object Object::GetOpt<Object>(const std::string &key, const Object &defaultObject) const
{
	auto value = FindOpt(key);
	if(!value)
		return defaultObject;

	try
	{
		return value->AsObject();
	}
	catch(std::exception &)
	{
		return defaultObject;
	}
}

template <>
inline uint32_t Object::GetOpt<uint32_t>(const std::string &key, const uint32_t &defaultValue) const
{
	auto value = FindOpt(key);
	if(!value || (!value->IsNumber() && !value->IsString()))
		return defaultValue;

	if(value->IsString() && value->AsString().empty())
		return defaultValue;

	try
	{
		return value->IsNumber() ? static_cast<uint32_t>(value->AsNumber()) 
			: std::stoi(value->AsString());
	}
	catch(std::exception &)
	{
		return defaultValue;
	}
}

template <>
inline uint64_t Object::GetOpt<uint64_t>(const std::string &key, const uint64_t &defaultValue) const
{
	auto value = FindOpt(key);
	if(!value || (!value->IsNumber() && !value->IsString()))
		return defaultValue;

	if(value->IsString() && value->AsString().empty())
		return defaultValue;

	try
	{
		return value->IsNumber() ? static_cast<uint32_t>(value->AsNumber()) 
			: std::stoll(value->AsString());
	}
	catch(std::exception &)
	{
		return defaultValue;
	}
}

inline bool Object::Has(const std::string &key) const
{
	return FindOpt(key) != nullptr;
}

inline Type Object::GetType(const std::string &key) const
{
	auto value = FindOpt(key);
	if(!value)
		return Type_Null;

	return value->GetType();
}

inline std::string Object::AsString()
{
	// Convert us to a value then stringify
	return static_cast<std::string>(*this);
}

template <class T>
inline ValuePtr Object::Set(const std::string &key, const T &_value)
{
	auto value = Value::Create(_value);
	Put(key, value);
	return value;
}

template <class T>
inline ValuePtr Object::Set(const std::string &key, const T &&_value)
{
	auto value = Value::Create(_value);
	Put(key, value);
	return value;
}

	}
}
