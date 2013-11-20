#include "Common.h"

namespace CopyExample {

	class CmdLine
	{
	public:
		CmdLine(int argc, const char *argv[]) :
			m_argc(argc), m_argv(argv)
		{
			if(argc < 2)
				throw std::logic_error("Must specify an action");
		}

		// List of actions supported by this commandline parser
		enum ACTION
		{
			LOGIN,
		};

		ACTION GetAction() const 
		{
			if(std::string(m_argv[1]) == "--login")
				return LOGIN;

			throw std::logic_error(std::string("Invalid action ") + m_argv[1]);
		}

		std::string GetOption(uint32_t index) const
		{
			return m_argv[index + 1];
		}

		static std::string Usage()
		{
			return "--login <username> <password>";
		}

		const char **m_argv;
		int m_argc;
	};

}
