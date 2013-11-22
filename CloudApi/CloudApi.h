#pragma once

#include "Common.h"
#include "JSON.h"
#include <liboauthcpp/liboauthcpp.h>

namespace CopyExample {

/**
 * CloudApi - The example class for copy api
 */
class CloudApi
{
public:
	// This structure configurs the CloudApi class
	struct Config
	{
		// Api version the client is adhering too (for now, always 1.0)
		std::string cloudApiVersion = "1.0";

		// OAuth tokens
		std::string consumerKey, consumerSecret;
		std::string accessToken, accessTokenSecret;

		// Address to connect to
		std::string address = "http://api.qa.copy.com";

		// Enabled to log detailed output to screen
		bool curlDebug = false;
	};

	// This structure decribes a chunk of data
	struct PartInfo
	{
		// A has is comprised of an md5 + sha1
		std::string fingerprint;

		// Data of part (api dependent)
		std::vector<unsigned char> data;

		// Size of part (data may not be populated)
		uint64_t size = 0;

		// Offset of this part relative to a file
		uint64_t offset = 0;
	};

	// Populated from a call to Login, returns all the information about the
	// logged in user
	struct UserInfo
	{
		uint64_t userId = 0;
		uint64_t clientId = 0;
		std::string firstName;
		std::string lastName;
		std::string pushToken;
		std::string pushUrl;
		std::string authToken;
		std::vector<std::string> emails;
	};

	enum CloudError
	{
		INCORRECT_LOGIN_CREDENTIALS = 1008,
		USER_EMAIL_NOT_VERIFIED = 1009,
		USER_CREATE_EMAIL_ALREADY_EXISTS = 1010,
		SHARE_CREATE_ALREADY_EXISTS = 8001,
		NO_SUCH_EMAIL = 1017,
		SHARE_JOIN_DENIED = 1031,
		CLOUD_SHARE_MISSING = 1034,
		CLOUD_OBJECT_MISSING = 1021,
		INVALID_PEER_SYNC_TOKEN = 1029,
		INVALID_LIST_WATERMARK = 1030,
		PART_NOT_FOUND = 1600,
		CLOUD_RESPONSE_FAILURE = 9999, 
	};

	class CloudException : public std::exception
	{
	public:
		CloudException(CloudError code, const std::string &message) : m_code(code), m_message(message) 
		{
			m_prepared = "Code: " + std::to_string(m_code) + " Message: " + m_message;
		}

		const char * what() const noexcept override { return m_prepared.c_str(); }

		CloudError m_code;
		std::string m_message;
		std::string m_prepared;
	};

	CloudApi(Config param);
	~CloudApi();

	void SendNeededParts(const std::vector<PartInfo> &parts);
	std::vector<PartInfo> HasParts(std::vector<PartInfo> parts);
	void GetParts(std::vector<PartInfo> &parts);
	void CreateFile(const std::string &path, const std::vector<PartInfo> &parts);

	struct CloudObj
	{
		std::string path;
		std::string type;
		uint64_t childCount = 0;
		uint64_t id = 0;
		uint64_t removedTime = 0, createdTime = 0, modifiedTime = 0;
		uint64_t size = 0;
		std::vector<PartInfo> parts;
		JSON::ValuePtr attributes;

		explicit operator bool () const { return !path.empty(); }
	};

	struct ListResult
	{
		CloudObj root;
		std::vector<CloudObj> children;
		bool more = false;
	};

	struct ListConfig 
	{
		std::string path;
		uint64_t index = 0;
		bool includeParts = false;
		bool recurse = false;
		bool includeChildCounts = false;
		uint32_t maxCount = 50;
		bool groupByDir = false;
		std::string filter;
		std::string sortField;
		std::string sortDirection;
	};

	ListResult ListPath(ListConfig &config);

protected:
	void Perform();
	std::string Post(std::map<std::string, std::string> &headerFields, const std::string &data, const std::string &endpoint = "jsonrpc");
	void SetCommonHeaderFields(std::map<std::string, std::string> &headerFields, const std::string &endpoint = "jsonrpc");
	std::string EncodeJsonRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request);
	JSON::ValuePtr ProcessRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request = JSON::Object());
	void ParseCloudError(JSON::JSONRPC &responseRpc, std::map<std::string, std::string> &headerFields);
	CloudError MapCloudError(uint32_t errorCode);
	CloudObj ParseCloudObj(bool includeParts, const JSON::ValuePtr &cloudObjInfo);

	static int CurlDebugCallback(CURL *curl, curl_infotype infoType, char *data, size_t size, void *extra);
	static size_t CurlWriteHeaderCallback(void *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::map<std::string, std::string> *> *info);
	static size_t CurlWriteDataCallback(char *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::string *> *info);

	Config m_config;
	static std::once_flag s_hasInitializedCurl;
	void *m_curl = nullptr;

	OAuth::Consumer m_oauthConsumer;
	OAuth::Token m_oauthToken;
};

}
