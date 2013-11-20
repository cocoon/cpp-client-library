#include <stdio.h>
#include "Common.h"
#include "CmdLine.h"

using namespace CopyExample;

int main(int argc, const char *argv[])
{
	try
	{
		// Parse the commandline
		CmdLine cmdline(argc, argv);

		// Create a cloud api instance
		CloudApi::Config config;
		config.cloudApiVersion;
		config.sessionUser;

		CloudApi cloudApi(config);

		// See what the user wants to do
		switch(cmdline.GetAction())
		{
			case CmdLine::LOGIN:
			{
				std::cout << "Logging in as " << cmdline.GetOption(1) << std::endl;
				cloudApi.Login(cmdline.GetOption(1), cmdline.GetOption(2));
				break;
			}
		}
	}
	catch(std::exception &e)
	{
		std::cout << "Main completed with error " << e.what() << std::endl;
		std::cout << CmdLine::Usage() << std::endl;
	}
}
