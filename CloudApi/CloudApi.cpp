#include "CloudApi.h"

using namespace Copy;

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
 * GetParts - This function fetches parts from the cloud
 */
void CloudApi::GetParts(std::vector<PartInfo> &parts)
{
	// @@ TODO
}

/**
 * HasParts - This function will ask the cloud if it has the requested parts, and return
 * a vector of parts that the cloud does not have
 */
std::vector<CloudApi::PartInfo> CloudApi::HasParts(std::vector<PartInfo> parts)
{
	// @@ TODO
	return parts;
}

/**
 * SendNeededParts - This function sends all the parts in the vector, that the cloud doesn't have
 */
void CloudApi::SendNeededParts(const std::vector<PartInfo> &parts)
{
	// @@ TODO
}

/**
 * CreateFile - Creates or updates a file at a given path, with the parts listed
 */
void CloudApi::CreateFile(const std::string &cloudPath, const std::vector<PartInfo> &parts)
{
	std::map<std::string, std::string> headerFields;
	SetCommonHeaderFields(headerFields);

	JSON::Array items;
	JSON::Object item, main_request;
	item.Set<std::string>("action", "create");
	item.Set<std::string>("object_type", "file");
	item.Set<std::string>("path", cloudPath);

	// Sum up the size of the parts
	uint64_t size = 0;
	for(auto &part : parts)
		size += part.size;

	item.Set<std::string>("size", std::to_string(size));

	JSON::Array part_items;
	for(auto &part : parts)
	{
		JSON::Object partObj;
		partObj.Set<std::string>("fingerprint", part.fingerprint);
		partObj.Set<std::string>("offset", std::to_string(part.offset));
		partObj.Set<std::string>("size", std::to_string(part.size));
		part_items.push_back(JSON::Value::Create(partObj));
	}
	item.Set<JSON::Array>("parts", part_items);

	items.push_back(JSON::Value::Create(item));

	main_request.Set<JSON::Array>("meta", items);
	
	ProcessRequest("update_objects", headerFields, main_request);
}

std::string CloudApi::Post(std::map<std::string, std::string> &headerFields, const std::string &data, const std::string &method)
{
	std::string response;

	auto completeUrl = m_config.address + "/" + method;

	if(m_config.debugCallback)
		m_config.debugCallback(std::string("Requesting ") + data);

	struct curl_slist *curlList = nullptr;
	for(auto iter = headerFields.begin(); iter != headerFields.end(); iter++)
		curlList = curl_slist_append(curlList, (iter->first + std::string(": ") + iter->second).c_str());
	
	auto callbackData = std::make_pair(this, &response);
	curl_easy_setopt(m_curl, CURLOPT_URL, completeUrl.c_str());
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curlList);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &callbackData);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlWriteDataCallback);
	curl_easy_setopt(m_curl, CURLOPT_POST, 1);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, data.size()); 
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, &data[0]);

	headerFields.clear();
	auto callbackInfo = std::make_pair(this, &headerFields);
	curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &callbackInfo);
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, CurlWriteHeaderCallback);

	try
	{
		Perform();
	}
	catch(const std::exception &)
	{
		curl_slist_free_all(curlList);
		throw;
	}

	if(m_config.debugCallback)
		m_config.debugCallback(std::string("Reply " ) + response);

	curl_slist_free_all(curlList);

	return response;
}

size_t CloudApi::CurlWriteDataCallback(char *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::string *> *info)
{
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
}

size_t CloudApi::CurlWriteHeaderCallback(void *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::map<std::string, std::string> *> *info)
{
	auto headerLine = reinterpret_cast<char *>(ptr);
	auto keys = SplitString(headerLine, ":");
	keys.first.erase(std::remove_if(keys.first.begin(), keys.first.end(), ::isspace), keys.first.end());
	keys.second.erase(std::remove_if(keys.second.begin(), keys.second.end(), ::isspace), keys.second.end());
	info->second->operator[](keys.first) = keys.second;
	return size * nmemb;
}

int CloudApi::CurlDebugCallback(CURL *curl, curl_infotype infoType, char *data, size_t size, CloudApi *thisClass)
{
	if(!size)
		return 0;

	std::ostringstream ss;

	switch(infoType)
	{
		case CURLINFO_HEADER_IN:
			ss << "HEADER <- " << std::string(data, size);
			break;

		case CURLINFO_HEADER_OUT:
			ss << "HEADER -> " << std::string(data, size);
			break;

		case CURLINFO_TEXT:
			ss << std::string(data, size);
			break;
	}

	thisClass->m_config.debugCallback(ss.str());

	return 0;
}

void CloudApi::Perform()
{
	if(m_config.debugCallback)
	{
		curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, this);
		curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);
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
	SetCommonHeaderFields(headerFields);
	ListResult result;
	bool firstTime = !config.index;

	JSON::Object main_request;
	main_request.Set<std::string>("path", config.path);

	if(config.maxCount)
		main_request.Set<std::string>("max_items", std::to_string(config.maxCount));

	main_request.Set<std::string>("list_watermark", std::to_string(config.index));
	main_request.Set<std::string>("include_total_items", std::to_string(0));
	main_request.Set<std::string>("recurse", std::to_string(config.recurse));
	main_request.Set<std::string>("include_parts", std::to_string(config.includeParts));
	main_request.Set<std::string>("include_child_counts", std::to_string(config.includeChildCounts));
	main_request.Set<std::string>("include_attributes", std::to_string(1));
	main_request.Set<std::string>("include_sync_filters", std::to_string(0));

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
		result.root = ParseCloudObj(parentInfo);
	}

	for(auto &cloudObjInfo : cloudObjArray)
	{
		auto cloudObj = ParseCloudObj(cloudObjInfo);
		if(cloudObj)
			result.children.push_back(cloudObj);
	}

	return result;
}

CloudApi::CloudObj CloudApi::ParseCloudObj(const JSON::ValuePtr &cloudObjInfo)
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
		obj.size = cloudObjInfoObj.GetOpt<uint64_t>("size", 0);

		if(cloudObjInfoObj.Has("revisions"))
		{
			auto revisionsArray = cloudObjInfoObj.Get<JSON::Array>("revisions");

			for(auto &revision : revisionsArray)
			{
				auto revisionObj = revision->AsObject();

				if(revisionObj.Has("parts"))
				{
					auto partsArray = revisionObj.Get<JSON::Array>("parts");
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

				break;
			}
		}
	}

	return obj;
}

std::string CloudApi::EncodeJsonRequest(const std::string &method, std::map<std::string, std::string> &headerFields, JSON::Object _request)
{
	JSON::JSONRPC requestRpc;
	requestRpc.id = JSON::Value::Create("0");
	requestRpc.method = JSON::Value::Create(method);

	requestRpc.params  = JSON::Value::Create(_request);
	auto requestJSON = requestRpc.ToJSON();
	return JSON::Value::Create(requestJSON)->Stringify();
}

JSON::ValuePtr CloudApi::ProcessRequest(const std::string &method, std::map<std::string, std::string> &headerFields, JSON::Object _request)
{
	auto data = EncodeJsonRequest(method, headerFields, _request);

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
	// Do oauth
	OAuth::Client oauth(&m_oauthConsumer, &m_oauthToken);
	headerFields["Authorization"] = SplitString(oauth.getFormattedHttpHeader(OAuth::Http::Post,
		 m_config.address + "/" + method), ": ").second;

	// Required to bypass oath binary payloads
	if(method == "has_object_parts" || method == "send_object_parts" || method == "get_object_parts")
		headerFields["Content-Type"] = "application/octet-stream";

	headerFields["X-Api-Version"] = m_config.cloudApiVersion;
	headerFields["X-Client-Type"] = "api";
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

/**
 * BinaryPackPart - Packs a part into a request structure
 * Returns if it was successful
 */
bool CloudApi::BinaryPackPart(PartInfo part, Data &data, bool addPartData, uint64_t shareId)
{
	if(data.Size() < sizeof(PARTS_HEADER))
		data.Grow(sizeof(PARTS_HEADER));

	auto totalDataSize = sizeof(PART_ITEM) + (addPartData ? part.size : 0);

	auto partItem = data.CastAllocAtEnd<PART_ITEM>(totalDataSize);

	partItem->signature = CPU32_NET(0xCAB005E5);
	partItem->version = CPU32_NET(BINARY_PART_ITEM_VERSION);
	partItem->dataSize = CPU32_NET(totalDataSize);

	partItem->shareId = CPU32_NET(static_cast<uint32_t>(shareId));

	strcpy(partItem->fingerprint, part.fingerprint.c_str());
	partItem->partSize = CPU32_NET(part.size);
	partItem->payloadSize = (addPartData ? CPU32_NET(part.size) : CPU32_NET(0));
	partItem->errorCode = CPU32_NET(0);

	if(addPartData)
		data.Copy(data.PtrToOffset(partItem) + sizeof(PART_ITEM), part.data);

	return true;
}

/**
 * BinaryPackPartsHeader - Packs a header into a request structure
 */
void CloudApi::BinaryPackPartsHeader(Data &data, uint32_t partCount)
{
	if(data.Size() < sizeof(PARTS_HEADER))
		data.Grow(sizeof(PARTS_HEADER));

	auto header = data.Cast<PARTS_HEADER>();

	header->signature = CPU32_NET(0xBA5EBA11);
	header->headerSize = CPU32_NET(sizeof(PARTS_HEADER));
	header->version = CPU32_NET(BINARY_PARTS_HEADER_VERSION);
	header->bodySize = CPU32_NET(data.Size() - sizeof(PARTS_HEADER));
	header->partCount = CPU32_NET(partCount);
	header->errorCode = CPU32_NET(0);
}

/**
 * BinaryParsePartsReply - Parses a cloud response from the binary parts api
 * Returns count of parts with valid part data
 * Include "parts" if expecting data
 * Include "partInfos" if wanting what parts the cloud returned
 */
uint32_t CloudApi::BinaryParsePartsReply(Data &replyData,
	 std::list<PartInfo> *parts, std::list<PART_ITEM*> *partInfos)
{
	auto *header = replyData.Cast<PARTS_HEADER>();

	header->signature = NET32_CPU(header->signature);
	header->headerSize = NET32_CPU(header->headerSize);
	header->version = NET32_CPU(header->version);
	header->bodySize = NET32_CPU(header->bodySize);
	header->partCount = NET32_CPU(header->partCount);
	header->errorCode = NET32_CPU(header->errorCode);

	if(header->signature != BINARY_PARTS_HEADER_SIG)
		throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: Invalid header signature");
	else if(header->errorCode)
		throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, replyData.Cast<char>() + sizeof(PARTS_HEADER));
	else if(header->headerSize != sizeof(PARTS_HEADER))
		throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: Invalid header size");
	else if(parts && parts->size() != header->partCount)
		throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: didn't get expected part count from cloud");
	
	StructParser parser(offsetof(PART_ITEM, dataSize),
		 replyData.Cast<uint8_t>() + sizeof(PARTS_HEADER), header->bodySize, false, true);

	std::list<PartInfo>::iterator partInfoIter;
	if(parts)
		partInfoIter = parts->begin();
	PART_ITEM *partItem;
	uint32_t  partCount = 0;
	while((partItem = parser.GetNextEntry<PART_ITEM>()))
	{
		partItem->signature = NET32_CPU(partItem->signature);
		partItem->dataSize = NET32_CPU(partItem->dataSize);
		partItem->version = NET32_CPU(partItem->version);
		partItem->shareId = NET32_CPU(partItem->shareId);
		partItem->partSize = NET32_CPU(partItem->partSize);
		partItem->payloadSize = NET32_CPU(partItem->payloadSize);
		partItem->errorCode = NET32_CPU(partItem->errorCode);

		if(partInfos)
			partInfos->push_back(partItem);

		if(partItem->signature != BINARY_PARTS_ITEM_SIG)
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part signature");
		else if(parts && partItem->errorCode)
		{
			partInfoIter->data.Release();
			std::cerr << "Part error " << replyData.Cast<char>(replyData.PtrToOffset(partItem) + sizeof(PART_ITEM), partItem->payloadSize);
			
			continue;
		}
		else if(parts && std::string(partItem->fingerprint) != (*partInfoIter).fingerprint)
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part fingerprint");
		else if(parts && partItem->partSize != (*partInfoIter).size)
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part size");
		else if(parts && partItem->payloadSize != partItem->partSize)
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part payload size");
		else if(parts && partItem->dataSize != partItem->partSize + sizeof(PART_ITEM))
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part data size");
		else if(partItem->version != BINARY_PART_ITEM_VERSION)
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: invalid part structure version");

		if(!parts)
		{
			partCount++;
			continue;
		}

		// Make sure we dont read past the buffer
		auto currentOffset = replyData.PtrToOffset(partItem);
		if(currentOffset + sizeof(PART_ITEM) + partInfoIter->size > replyData.Size())
			throw CloudException(CLOUD_MALFORMED_PART_RESPONSE, "BinaryParsePartsReply: not enough data from cloud");

		partInfoIter->data.Resize(partInfoIter->size);
		partInfoIter->data.Copy(partInfoIter->size, replyData.Cast<char>(currentOffset + sizeof(PART_ITEM), partInfoIter->size));
		
		auto actualHash = CreateFingerprint(partInfoIter->data);
		assert(actualHash == partItem->fingerprint);

		partInfoIter++;
		partCount++;
	}

	return partCount;
}

Data CloudApi::ProcessBinaryPartsRequest(const std::string &method,
	 const std::list<PartInfo> &parts, uint64_t shareId, bool sendMode)
{
	std::map<std::string, std::string> headerFields;
	SetCommonHeaderFields(headerFields, method);

	return ProcessBinaryPartsRequest(method, headerFields, parts, shareId, sendMode);
}

Data CloudApi::ProcessBinaryPartsRequest(const std::string &method, std::map<std::string, std::string> &headerFields,
	const std::list<PartInfo> &parts, uint64_t shareId, bool sendMode)
{
	Data requestData;
	uint32_t partCount = 0;
	uint32_t totalPartSize = 0;

	for(auto &part : parts)
	{
		if(BinaryPackPart(part, requestData, sendMode, shareId))
		{
			partCount++;
			totalPartSize += part.size;
		}
	}

	auto contentLength = requestData.Size();

	BinaryPackPartsHeader(requestData, partCount);

	return Data(Post(headerFields, requestData.ToString(), method));
}

