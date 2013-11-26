#include "Common.h"

using namespace Copy;
using namespace Copy::JSON;

JSONRPC::JSONRPC()
{
}

JSONRPC::JSONRPC(const Object &object)
{
	FromJSON(object);
}

Object JSONRPC::ToJSON() const
{
	Object object;

	if(IsValidRequest())
	{
		object.Set<ValuePtr>("method", method);

		if(params)
			object.Set<ValuePtr>("params", params);
	}
	else if(IsValidResponse())
	{
		if(result)
			object.Set<ValuePtr>("result", result);
		else
			object.Set<ValuePtr>("error", error);
	}
	else
		throw std::logic_error("JSON Encode Failure");

	object.Set<std::string>("jsonrpc", "2.0");
	object.Set<ValuePtr>("id", id);

	return object;
}

void JSONRPC::FromJSON(const JSON::Object &object)
{
	if(object.Get<std::string>("jsonrpc") != "2.0")
		throw std::logic_error("JSON Decode Failure");

	method = object.GetOpt<ValuePtr>("method", ValuePtr());
	params = object.GetOpt<ValuePtr>("params", ValuePtr());
	result = object.GetOpt<ValuePtr>("result", ValuePtr());
	error = object.GetOpt<ValuePtr>("error", ValuePtr());
	id = object.GetOpt<ValuePtr>("id", ValuePtr());

	if(!(IsValidRequest() || IsValidResponse()))
		throw std::logic_error("JSON Decode Failure");
}

bool JSONRPC::IsValidRequest() const
{
	// Method is required and must be a string
	if(!method || !method->IsString())
		return false;

	// Params are optional but must be an array or object
	if(params && !(params->IsArray() || params->IsObject()))
		return false;

	// No result or error
	if(result || error)
		return false;

	// ID is optional, but indicates there will be no response
	if(id && !(id->IsString() || id->IsNumber() || id->IsNull()))
		return false;

	return true;
}

bool JSONRPC::IsValidResponse() const
{
	// Either result or error, but not both
	// @@ TODO: wait for cloud to correctly implement JSON RPC
	//if(result && error || !result && !error)
	//	return false;

	// No method or params
	if(method || params)
		return false;

	// ID is required and must be string, number, or nulll
	if(!id || !(id->IsString() || id->IsNumber() || id->IsNull()))
		return false;

	return true;
}
