#include <stdio.h>
#include "boost/program_options.hpp"
#include "Common.h"

using namespace CopyExample;
using namespace boost;

int main(int argc, const char *argv[])
{
	program_options::options_description desc("Options");

	CloudApi::Config config;

	desc.add_options()
		("oauth-consumer-key", program_options::value<std::string>()->required(), "The OAUTH consumer key (required)")
		("oauth-consumer-secret", program_options::value<std::string>()->required(), "The OAUTH consumer secret (required)")
		("oauth-access-token", program_options::value<std::string>()->required(), "The OAUTH access token (required)")
		("oauth-access-token-secret", program_options::value<std::string>()->required(), "The OAUTH access token secret (required)")
		("list,l", program_options::value<std::string>()->required(), "List a path <path>")
		("send,p", program_options::value<std::vector<std::string>>()->required(), "Send a file <localPath> <remotePatk>");

	program_options::variables_map vm;

	try
	{
		try
		{
			// Parse the commandline
			program_options::store(program_options::parse_command_line(argc, argv, desc), vm);

			config.consumerKey = vm["oauth-consumer-key"].as<std::string>();
			config.consumerSecret = vm["oauth-consumer-secret"].as<std::string>();
			config.accessTokenSecret = vm["oauth-access-token"].as<std::vector<std::string>>();
			config.accessTokenSecret = vm["oauth-access-token-secret"].as<std::string>();
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
		{
			cloudApi.List(vm["list"].as<std::string>()[0]);
		}
		else if(vm.count("send"))
		{
		}
	}
	catch(std::exception &e)
	{
		std::cout << "Main completed with error " << e.what() << std::endl;
	}
}
