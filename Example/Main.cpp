#include <stdio.h>
#include "boost/program_options.hpp"
#include "CloudApi/Common.h"

using namespace Copy;
using namespace boost;

static void DoList(CloudApi &cloudApi, program_options::variables_map &vm)
{
	CloudApi::ListConfig listConfig;
	listConfig.path = vm["list"].as<std::string>();

	auto result = cloudApi.ListPath(listConfig);

	size_t fieldWidth = 0;
	for(auto &obj : result.children)
	{
		if(fieldWidth < obj.path.size())
			fieldWidth = obj.path.size();
	}

	for(auto &obj : result.children)
	{
		std::cout << std::setw(5) << obj.type << std::setw(5) << PrettySize(obj.size)
			 << " " << obj.path << std::endl;
	}
}

static void DoGet(CloudApi &cloudApi, program_options::variables_map &vm)
{
	auto cloudPath = vm["get"].as<std::vector<std::string>>()[0];
	auto filePath = vm["get"].as<std::vector<std::string>>()[1];

	std::cout << "Downloading cloud path" << cloudPath << std::endl;

	// List the file to get its part list
	CloudApi::ListConfig config;
	config.path = cloudPath;
	config.includeParts = true;

	auto result = cloudApi.ListPath(config);

	for(auto &part : result.root.parts)
		std::cout << "Detected part " << part.fingerprint << " for file" << std::endl;
}

static void DoSend(CloudApi &cloudApi, program_options::variables_map &vm)
{
	std::ifstream file;
	auto filePath = vm["send"].as<std::vector<std::string>>()[0];
	auto cloudPath = vm["send"].as<std::vector<std::string>>()[1];

	std::cout << "Sending " << filePath << " to " << cloudPath << std::endl;

	file.open(filePath, std::ios::binary);
	if(!file.is_open())
		throw std::logic_error(std::string("Failed to open ") + filePath);
	
	std::vector<CloudApi::PartInfo> parts;
	std::vector<CloudApi::PartInfo> partBatch;

	// This function will send up a batch of parts, move them to the pain part array,
	// and free their data
	auto sendPartBatch = [&]()
		{
			std::cout << "Sending " << partBatch.size() << " part(s)" << std::endl;

			cloudApi.SendNeededParts(partBatch);
			
			// Clear their data and transfer back to main part bucket
			for(auto &part : partBatch)
			{
				part.data.Release();
				parts.push_back(std::move(part));
			}

			partBatch.clear();
		};

	// Read in 1MB chunks, and send the parts along the way
	uint64_t offset = 0;
	while(1)
	{
		CloudApi::PartInfo part;

		// Read as much as we can
		part.data.Resize(1024 * 1024);
		file.seekg(offset, std::ios::beg);
		file.read(part.data.Cast<char>(), part.data.Size());

		part.data.Resize(file.gcount());

		if(part.data.IsEmpty())
			break;

		// Prepare the part
		part.fingerprint = CreateFingerprint(part.data);
		part.offset = offset;
		offset += part.data.Size();

		// Add it to our batch
		partBatch.push_back(std::move(part));

		// Send 5 up at a time
		if(partBatch.size() == 5)
			sendPartBatch();
	}

	// Send up any stragglers
	sendPartBatch();
}

int main(int argc, const char *argv[])
{
	program_options::options_description desc("Options");

	CloudApi::Config config;

	desc.add_options()
		("consumer-key", program_options::value<std::string>()->required(), "The OAUTH consumer key (required)")
		("consumer-secret", program_options::value<std::string>()->required(), "The OAUTH consumer secret (required)")
		("access-token", program_options::value<std::string>()->required(), "The OAUTH access token (required)")
		("access-token-secret", program_options::value<std::string>()->required(), "The OAUTH access token secret (required)")
		("debug,d", "Enable debug output")
		("list,l", program_options::value<std::string>()->required(), "List a path <path>")
		("send,s", program_options::value<std::vector<std::string>>()->required(), "Send a file <localPath> <remotePatk>") 
		("get,g", program_options::value<std::vector<std::string>>()->required(), "Get a file from the cloud <remotePatk> <localPath");

	program_options::variables_map vm;

	try
	{
		try
		{
			// Parse the commandline
			program_options::store(program_options::parse_command_line(argc, argv, desc), vm);

			config.consumerKey = vm["consumer-key"].as<std::string>();
			config.consumerSecret = vm["consumer-secret"].as<std::string>();
			config.accessToken = vm["access-token"].as<std::string>();
			config.accessTokenSecret = vm["access-token-secret"].as<std::string>();
		}
		catch(std::exception &e)
		{
			// Print usage as we failed to parse them
			std::cout << e.what() << std::endl << desc << std::endl;
			exit(-1);
		}

		if(vm.count("debug"))
			config.debugCallback = [&](const std::string &message) { std::cout << message << std::endl; };

		CloudApi cloudApi(config);

		// Determine if they want to send, or list
		if(vm.count("list"))
			DoList(cloudApi, vm);
		if(vm.count("send"))
			DoSend(cloudApi, vm);
		if(vm.count("get"))
			DoGet(cloudApi, vm);
	}
	catch(std::exception &e)
	{
		std::cout << "Main completed with error " << e.what() << std::endl;
	}
}
