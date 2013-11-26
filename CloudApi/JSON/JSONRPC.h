#pragma once

namespace Copy {
	namespace JSON {

struct JSONRPC 
{
	JSONRPC();
	JSONRPC(const Object &object);

	Object ToJSON() const;
	void FromJSON(const JSON::Object &object);

	bool IsValidRequest() const;
	bool IsValidResponse() const;

	ValuePtr method;
	ValuePtr params;

	ValuePtr result;
	ValuePtr error;

	ValuePtr id;
};

	}
}
