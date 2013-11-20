#pragma once

#include "Common.h"
#include "JSON.h"

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
		std::string cloudApiVersion;

		// The client version (arbitrary)
		std::string clientVersion;

		// Name of user logged in (arbritrary)
		std::string sessionUser;

		// Some unique value tied to the physical system 
		// must match what was used at login, if an auth token is used
		// to authenticate
		std::string hostUuid;

		// Name of the machine we are running on
		std::string hostName;

		// Auth token (If you know ahead of time, bypass login)
		std::string authToken;
		std::string clientType = PlatformName;

		// Address to connect to
		std::string address = "https://api.copy.com";
	};

	// This structure decribes a chunk of data
	struct PartInfo
	{
		// A has is comprised of an md5 + sha1
		std::string hash;

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

	CloudApi(const Config &param);
	~CloudApi();

	UserInfo Login(const std::string &user, const std::string &password);
	UserInfo Authenticate(const std::string &authtoken);
	void SendNeededParts(const std::vector<PartInfo> &parts);
	void CreateFile(const std::string &path, const std::vector<PartInfo> &parts);

protected:
	void Perform();
	std::string Post(std::map<std::string, std::string> &headerFields, const std::string &data);
	void SetCommonHeaderFields(std::map<std::string, std::string> &headerFields, const std::string &authToken);
	std::string EncodeJsonRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request);
	JSON::ValuePtr ProcessRequest(const std::string &command, std::map<std::string, std::string> &headerFields, JSON::Object _request);
	void ParseCloudError(JSON::JSONRPC &responseRpc, std::map<std::string, std::string> &headerFields);
	CloudError MapCloudError(uint32_t errorCode);

	Config m_config;
	static std::once_flag s_hasInitializedCurl;
	void *m_curl = nullptr;
};

}
