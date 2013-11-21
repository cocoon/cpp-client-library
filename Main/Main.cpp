#include <stdio.h>
#include "boost/program_options.hpp"
#include "Common.h"

using namespace CopyExample;
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

static void DoSend(CloudApi &cloudApi, program_options::variables_map &vm)
{
	// @@ TODO
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
		("list,l", program_options::value<std::string>()->required(), "List a path <path>")
		("send,p", program_options::value<std::vector<std::string>>()->required(), "Send a file <localPath> <remotePatk>");

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

		CloudApi cloudApi(config);

		// Determine if they want to send, or list
		if(vm.count("list"))
			DoList(cloudApi, vm);
		else if(vm.count("send"))
			DoSend(cloudApi, vm);
	}
	catch(std::exception &e)
	{
		std::cout << "Main completed with error " << e.what() << std::endl;
	}
}
