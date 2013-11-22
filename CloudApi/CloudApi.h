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
		std::string cloudApiVersion = "1.0";
		std::string consumerKey, consumerSecret;
		std::string accessToken, accessTokenSecret;
		std::string address = "http://api.qa.copy.com";
		std::function<void(const std::string &)> debugCallback;
	};

	// This structure decribes a chunk of data
	struct PartInfo
	{
		std::string fingerprint;
		std::vector<uint8_t> data;
		uint64_t size = 0;
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
		CLOUD_MALFORMED_PART_RESPONSE = 9998,
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
	std::string Post(std::map<std::string, std::string> &headerFields, const std::string &data, const std::string &method = "jsonrpc");

	void SetCommonHeaderFields(std::map<std::string, std::string> &headerFields, const std::string &method = "jsonrpc");
	std::string EncodeJsonRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request);
	JSON::ValuePtr ProcessRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request = JSON::Object());
	void ParseCloudError(JSON::JSONRPC &responseRpc, std::map<std::string, std::string> &headerFields);
	CloudError MapCloudError(uint32_t errorCode);
	CloudObj ParseCloudObj(const JSON::ValuePtr &cloudObjInfo);

	static int CurlDebugCallback(CURL *curl, curl_infotype infoType, char *data, size_t size, CloudApi *extra);
	static size_t CurlWriteHeaderCallback(void *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::map<std::string, std::string> *> *info);
	static size_t CurlWriteDataCallback(char *ptr, size_t size, size_t nmemb, std::pair<CloudApi *, std::string *> *info);

	// Define binary cloud api types
	const uint32_t BINARY_PARTS_HEADER_VERSION = 1;
	const uint32_t BINARY_PART_ITEM_VERSION = 1;

	const uint32_t BINARY_PARTS_HEADER_SIG = 0xBA5EBA11;
	const uint32_t BINARY_PARTS_ITEM_SIG = 0xCAB005E5;

	#pragma pack(push, 1)
		struct PARTS_HEADER
		{
			uint32_t signature;			// "0xba5eba11"
			uint32_t headerSize;		// Size of this structure
			uint32_t version;			// Struct version
			uint32_t bodySize;			// Total size of all data after the header
			uint32_t partCount;			// Part count
			uint32_t errorCode;			// Error code for errors regarding the entire 
		};

		struct PART_ITEM
		{
			uint32_t signature;			// "0xcab005e5"
			uint32_t dataSize;			// Size of this struct plus payload size
			uint32_t version;			// Struct version
			uint32_t shareId;			// Share id for part (for verification)
			char fingerprint[73];		// Part fingerprint
			uint32_t partSize;			// Size of the part
			uint32_t payloadSize;		// Size of our payload (partSize or 0, error msg size on error)
			uint32_t errorCode;			// Error code for individual parts
			uint32_t reserved;			// Reserved for future use
		};
	#pragma pack(pop)

	bool BinaryPackPart(PartInfo part, std::vector<uint8_t> &data, bool addPartData, uint64_t shareId);
	void BinaryPackPartsHeader(std::vector<uint8_t> &data, uint32_t partCount);
	uint32_t BinaryParsePartsReply(std::vector<uint8_t> &replyData,
		 std::list<PartInfo> *parts, std::list<PART_ITEM*> *partInfos);
	std::vector<uint8_t> ProcessBinaryPartsRequest(const std::string &command, const std::list<PartInfo> &parts, uint64_t shareId, bool sendMode);
	std::vector<uint8_t> ProcessBinaryPartsRequest(const std::string &command, std::map<std::string, std::string> &headerFields,
		const std::list<PartInfo> &parts, uint64_t shareId, bool sendMode);

	Config m_config;
	static std::once_flag s_hasInitializedCurl;
	void *m_curl = nullptr;

	OAuth::Consumer m_oauthConsumer;
	OAuth::Token m_oauthToken;
};

}
