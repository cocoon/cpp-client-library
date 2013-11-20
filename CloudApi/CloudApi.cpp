#include "CloudApi.h"

using namespace CopyExample;

std::once_flag CloudApi::s_hasInitializedCurl;

/**
 * CloudApi - Constructs the cloud api instance, the Config structure
 * contains all the required configuration options
 */
CloudApi::CloudApi(const Config &param) :
	m_config(param)
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
 * Login - Logs in with a user and password combo, returns a UserInfo structure 
 * that contains all the required authentication informatino for the user
 */
CloudApi::UserInfo CloudApi::Login(const std::string &user, const std::string &password)
{
	// @@ TODO
	return UserInfo();
}

/**
 * Authenticate - Re-authenticates the user, except just the auth token is re-verified
 */
CloudApi::UserInfo CloudApi::Authenticate(const std::string &authtoken)
{
	// @@ TODO
	return UserInfo();
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
	
	auto callbackData = std::make_pair(this, &response);
	curl_easy_setopt(m_curl, CURLOPT_URL, completeUrl.c_str());
	curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curlList);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &callbackData);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, [](char *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::string *> *info)->size_t
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
		});

	curl_easy_setopt(m_curl, CURLOPT_POST, 1);
	curl_easy_setopt(m_curl, CURLOPT_HEADER, 0);
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, data.size()); 
	curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, &data[0]);

	headerFields.clear();
	auto callbackInfo = std::make_pair(this, &headerFields);
	curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &callbackInfo);
	/*
	curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, [](void *ptr, size_t size, size_t nmemb, std::pair<YHttpClient *, std::map<std::string, std::string> *> *info)
		{
			auto headerLine = reinterpret_cast<char *>(ptr);
			auto keyValue = std::string(pHeaderLine).Split(": ");
			keyValue.first.erase(std::remove_if(keyValue.first.begin(), keyValue.first.end(), ::isspace), keyValue.first.end());
			keyValue.second.erase(std::remove_if(keyValue.second.begin(), keyValue.second.end(), ::isspace), keyValue.second.end());
			info->second->operator[](keyValue.first) = keyValue.second;
			return size * nmemb;
		}
	*/

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
	curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip,deflate");
	uint32_t httpStatus;
	auto result = curl_easy_perform(m_curl);
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpStatus);

	if(result != CURLE_OK)
		std::rethrow_exception(std::make_exception_ptr(std::logic_error(curl_easy_strerror(result))));
	// Allow 302 - Found (redirect) 200 - OK http status codes
	else if(httpStatus && httpStatus != 200 && httpStatus != 302)
		std::rethrow_exception(std::make_exception_ptr(std::logic_error("Unexpected http status")));
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

void CloudApi::SetCommonHeaderFields(std::map<std::string, std::string> &headerFields, const std::string &authToken)
{
	headerFields["X-Client-Version"] = m_config.clientVersion;
	headerFields["X-Client-Machine-Id"] = m_config.hostUuid;
	headerFields["X-Client-Machine-Name"] = m_config.hostName;
	headerFields["X-Client-Machine-User"] = m_config.sessionUser;

	if(!authToken.empty())
		headerFields["X-Authorization"] = authToken;
	else if(!m_config.authToken.empty())
		headerFields["X-Authorization"] = m_config.authToken;

	headerFields["X-Api-Version"] = m_config.cloudApiVersion;
	headerFields["X-Client-Type"] = PlatformName;
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

