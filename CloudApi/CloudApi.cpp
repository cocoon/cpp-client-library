#include "CloudApi.h"

using namespace CopyExample;

std::once_flag CloudApi::s_hasInitializedCurl;

/**
 * CloudApi - Constructs the cloud api instance, the Config structure
 * contains all the required configuration options
 */
CloudApi::CloudApi(Config param) :
	m_config(param), m_oauthConsumer(param.consumerKey, param.consumerSecret),
	m_oauthToken(param.accessToken, param.accessTokenSecret)
{
	std::call_once(s_hasInitializedCurl, []() { curl_global_init(CURL_GLOBAL_ALL); });

	m_curl = curl_easy_init();

	// Don't let signals mess us up! This prevents SIGALARM signal handlers to crash
	// on longjumps in the dns timeout code in hostip.c
	curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);

	// Some SSL handshakes (e.g. w/ antivirus scanning) bomb out if we don't explicitly set this.
	// openSSL 1.0.1c bug? https://code.google.com/p/plowshare/issues/detail?id=731
	curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
	curl_easy_setopt(m_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
}

CloudApi::~CloudApi()
{
	curl_easy_cleanup(m_curl);
}

/**
 * SendNeededParts - This function sends all the parts in the vector, that the cloud doesn't have
 */
void CloudApi::SendNeededParts(const std::vector<PartInfo> &parts)
{
}

/**
 * CreateFile - Creates or updates a file at a given path, with the parts listed
 */
void CloudApi::CreateFile(const std::string &cloudPath, const std::vector<PartInfo> &parts)
{
}

std::string CloudApi::Post(std::map<std::string, std::string> &headerFields, const std::string &data)
{
	std::string response;

	auto completeUrl = m_config.address + "/jsonrpc";

	struct curl_slist *curlList = nullptr;
	for(auto iter = headerFields.begin(); iter != headerFields.end(); iter++)
		curlList = curl_slist_append(curlList, (iter->first + std::string(": ") + iter->second).c_str());

	std::cout << "Posting to url " << completeUrl << std::endl;
	
	auto callbackData = std::make_pair(this, &response);
	curl_easy_setopt(m_curl, CURLOPT_URL, completeUrl.c_str());
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curlList);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &callbackData);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, [](char *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::string *> *info)->size_t
		{
			std::cout << "Writing data " << std::endl;

			std::vector<char> buf;
			buf.reserve(size * nmemb + 1);

			auto pbuf = &buf[0];

			memset(&buf[0], '\0', size * nmemb + 1);
			size_t i = 0;
			for(;  i < nmemb; i++)
			{
				strncpy(pbuf, static_cast<char *>(ptr), size);
				pbuf += size;
				ptr += size;
			}

			info->second->append(&buf[0]);
			return size * nmemb;
		});

	curl_easy_setopt(m_curl, CURLOPT_POST, 1);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, data.size()); 
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, &data[0]);

	headerFields.clear();
	auto callbackInfo = std::make_pair(this, &headerFields);
	curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &callbackInfo);
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, 
		[](void *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::map<std::string, std::string> *> *info)
		{
			std::cout << "Writing headers " << std::endl;
			auto headerLine = reinterpret_cast<char *>(ptr);
			auto keys = SplitString(headerLine, ":");
			keys[0].erase(std::remove_if(keys[0].begin(), keys[0].end(), ::isspace), keys[0].end());
			keys[1].erase(std::remove_if(keys[1].begin(), keys[1].end(), ::isspace), keys[1].end());
			info->second->operator[](keys[0]) = keys[1];
			return size * nmemb;
		});

	try
	{
		Perform();
	}
	catch(const std::exception &)
	{
		curl_slist_free_all(curlList);
		throw;
	}

	curl_slist_free_all(curlList);

	return response;
}

void CloudApi::Perform()
{
	if(m_config.curlDebug)
	{
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, 
			[](CURL *curl, curl_infotype infoType, char *data, size_t size, void *extra)->int
			{
				if(!size)
					return 0;

				switch(infoType)
				{
					case CURLINFO_TEXT:
						std::cout << std::string(data, size) << std::endl;
						break;
					case CURLINFO_HEADER_IN:
						std::cout << "HEADER <- " << std::string(data, size) << std::endl;
						break;
					case CURLINFO_HEADER_OUT:
						std::cout << "HEADER -> " << std::string(data, size) << std::endl;
						break;
				}

				return 0;
			});
	}

	curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip,deflate");
	uint32_t httpStatus;
	auto result = curl_easy_perform(m_curl);
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpStatus);

	if(result != CURLE_OK)
		throw std::logic_error(curl_easy_strerror(result));
	// Allow 302 - Found (redirect) 200 - OK http status codes
	else if(httpStatus && httpStatus != 200 && httpStatus != 302)
		throw std::logic_error("Unexpected http status");
}

/**
 * ListPath - List cloudObj at a specific path
 */
CloudApi::ListResult CloudApi::ListPath(ListConfig &config)
{
	std::map<std::string, std::string> headerFields;
	SetCommonHeaderFields(headerFields, "jsonrpc");
	ListResult result;
	bool firstTime = !config.index;

	JSON::Object main_request;
	main_request.Set<std::string>("path", config.path);

	if(config.maxSize)
		main_request.Set<std::string>("max_size", std::to_string(config.maxSize));
	if(config.maxCount)
		main_request.Set<std::string>("max_items", std::to_string(config.maxCount));

	main_request.Set<std::string>("list_watermark", std::to_string(config.index));
	main_request.Set<std::string>("include_total_items", std::to_string(0));
	main_request.Set<std::string>("recurse", std::to_string(config.recurse));
	main_request.Set<std::string>("include_parts", std::to_string(config.includeParts));
	main_request.Set<std::string>("include_child_counts", std::to_string(config.includeChildCounts));
	main_request.Set<std::string>("include_attributes", std::to_string(1));

	if(!config.filter.empty())
		main_request.Set<std::string>("filter_name", config.filter);

	if(!config.groupByDir)
		main_request.Set<std::string>("group_by_dir", std::to_string(config.groupByDir));

	if(!config.sortField.empty())
		main_request.Set<std::string>("sort_field", config.sortField);

	if(!config.sortDirection.empty())
		main_request.Set<std::string>("sort_direction", config.sortDirection);

	auto list_result = ProcessRequest("list_objects", headerFields, main_request)->AsObject();

	config.index = list_result.Get<uint64_t>("list_watermark");
	result.more = list_result.Get<uint32_t>("more_items");

	// Force sync index to increment so we don't loop forever
	if(!config.index)
		config.index = config.index + 1;

	if(list_result.GetType("children") == JSON::Type_Null)
		return result;
	auto cloudObjArray = list_result.Get<JSON::Array>("children");

	// First pass include root at start
	if(firstTime)
	{
		auto parentInfo = list_result.Get<JSON::ValuePtr>("object");
		result.root = ParseCloudObj(config.includeParts, parentInfo);
	}

	for(auto &cloudObjInfo : cloudObjArray)
	{
		auto cloudObj = ParseCloudObj(config.includeParts, cloudObjInfo);
		if(cloudObj)
			result.children.push_back(cloudObj);
	}

	return result;
}

CloudApi::CloudObj CloudApi::ParseCloudObj(bool includeParts, const JSON::ValuePtr &cloudObjInfo)
{
	auto cloudObjInfoObj = cloudObjInfo->AsObject();

	CloudObj obj;

	if(!cloudObjInfoObj.Has("path"))
		return CloudObj();

	auto type = cloudObjInfoObj.GetOpt<std::string>("type", cloudObjInfoObj.GetOpt<std::string>("object_type", ""));
	auto path = cloudObjInfoObj.Get<std::string>("path");

	obj.id = cloudObjInfoObj.GetOpt<uint64_t>("object_id", 0);
	obj.removedTime = cloudObjInfoObj.GetOpt<uint64_t>("removed_time", 0);
	obj.createdTime = cloudObjInfoObj.GetOpt<uint64_t>("created_time", 0);
	obj.modifiedTime = cloudObjInfoObj.GetOpt<uint64_t>("modified_time", 0);
	obj.childCount = cloudObjInfoObj.GetOpt<uint32_t>("children_count", 0);

	// File attributes (optional)
	if(cloudObjInfoObj.GetType("attributes") == JSON::Type_Object)
		obj.attributes = cloudObjInfoObj.Get<JSON::ValuePtr>("attributes");

	obj.path = path;

	auto action = cloudObjInfoObj.GetOpt<std::string>("action", "create");

	if(!(type == "file" || type == "dir" || type == "share" || type == "company"))
		return CloudObj();
	obj.type = type;

	if(type == "file")
	{
		if(includeParts)
		{
			obj.size = cloudObjInfoObj.GetOpt<uint64_t>("size", 0);
			
			if(cloudObjInfoObj.Has("parts"))
			{
				auto partsArray = cloudObjInfoObj.Get<JSON::Array>("parts");
				for(auto &partInfo : partsArray)
				{
					PartInfo part;

					auto partInfoObj = partInfo->AsObject();

					part.offset = partInfoObj.Get<uint64_t>("offset");
					part.size = partInfoObj.Get<uint32_t>("size");
					part.fingerprint = partInfoObj.Get<std::string>("fingerprint");

					obj.parts.push_back(part);
				}
			}
		}
	}

	return obj;
}

std::string CloudApi::EncodeJsonRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request)
{
	JSON::JSONRPC requestRpc;
	requestRpc.id = JSON::Value::Create("0");
	requestRpc.method = JSON::Value::Create(command);

	requestRpc.params  = JSON::Value::Create(_request);
	auto requestJSON = requestRpc.ToJSON();
	return JSON::Value::Create(requestJSON)->Stringify();
}

JSON::ValuePtr CloudApi::ProcessRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request)
{
	auto data = EncodeJsonRequest(command, headerFields, _request);

	auto response = Post(headerFields, data);

	auto value = JSON::Parse(response.c_str());

	JSON::JSONRPC responseRpc(value->AsObject());
	if(!responseRpc.IsValidResponse())
		throw CloudException(CLOUD_RESPONSE_FAILURE, "JSON response not valid JSONRPC");
	ParseCloudError(responseRpc, headerFields);

	return responseRpc.result;
}

void CloudApi::SetCommonHeaderFields(std::map<std::string, std::string> &headerFields, const std::string &method)
{
	headerFields["X-Client-Version"] = m_config.clientVersion;
	headerFields["X-Client-Machine-Id"] = m_config.hostUuid;
	headerFields["X-Client-Machine-Name"] = m_config.hostName;
	headerFields["X-Client-Machine-User"] = m_config.sessionUser;

	// Do oauth
	OAuth::Client oauth(&m_oauthConsumer, &m_oauthToken);
	headerFields["Authorization"] = SplitString(oauth.getFormattedHttpHeader(OAuth::Http::Post,
		 m_config.address + "/" + method), ": ")[1];

	headerFields["X-Api-Version"] = m_config.cloudApiVersion;
	headerFields["X-Client-Type"] = m_config.clientType;
	headerFields["X-Client-Time"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count() *
		 std::chrono::system_clock::period::num / std::chrono::system_clock::period::den);
}

CloudApi::CloudError CloudApi::MapCloudError(uint32_t errorCode)
{
	switch(errorCode)
	{
		case 1008:
			return INCORRECT_LOGIN_CREDENTIALS;

		case 1009:
			return USER_EMAIL_NOT_VERIFIED;

		case 1010:
			return USER_CREATE_EMAIL_ALREADY_EXISTS;

		case 8001:
			return SHARE_CREATE_ALREADY_EXISTS;

		case 1017:
			return NO_SUCH_EMAIL;

		case 1031:
			return SHARE_JOIN_DENIED;
			
		case 1034:
			return CLOUD_SHARE_MISSING;

		case 1021:
			return CLOUD_OBJECT_MISSING;

		case 1029:
			return INVALID_PEER_SYNC_TOKEN;

		case 1030:
			return INVALID_LIST_WATERMARK;

		case 1600:
			return PART_NOT_FOUND;

		default:
			return CLOUD_RESPONSE_FAILURE;
	}
}

void CloudApi::ParseCloudError(JSON::JSONRPC &responseRpc, std::map<std::string, std::string> &headerFields)
{
	if(headerFields["X-Request-Result"] == "success")
		return;

	if(!responseRpc.error || responseRpc.error->IsNull())
		return;

	auto error = responseRpc.error->AsObject();
	auto errorCode = error.Get<uint32_t>("code");
	auto errorString = error.Get<std::string>("message");

	throw CloudException(MapCloudError(errorCode), errorString);
}

