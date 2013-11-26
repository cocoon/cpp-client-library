#include "Common.h"

using namespace Copy;
using namespace Copy::JSON;

Object::operator std::string()
{
	// Convert us to a value then stringify
	return Value::Create(*this)->Stringify();
}

void Object::IterateObjects(ValuePtr value, IterateObjectCallback callback)
{
	if(value->IsArray())
	{
		for(auto &item : value->m_array)
		{
			switch(item->m_type)
			{
				case Type_Object:
				case Type_Array:
					IterateObjects(item, callback);
					break;
			}
		}
	}
	else if(value->IsObject())
	{
		callback(value->m_object);
		auto iter = value->m_object.m_fields.begin();
		for(auto iter = value->m_object.m_fields.begin(); iter != value->m_object.m_fields.end(); iter++)
		{
			switch(iter->second->m_type)
			{
				case Type_Object:
				case Type_Array:
					IterateObjects(iter->second, callback);
					break;
			}
		}
	}
}

ValuePtr Object::FindOpt(const std::string &key) const
{
	auto iter = m_fields.find(key);
	if(iter == m_fields.end())
		return ValuePtr();
	return iter->second;
}

ValuePtr Object::Find(const std::string &key) const
{
	auto iter = m_fields.find(key);
	if(iter == m_fields.end())
		throw std::logic_error(std::string("Failed to find field ") + key);
	return iter->second;
}

void Object::Put(const std::string &key, const ValuePtr &value)
{
	m_fields[key] = value;
}
