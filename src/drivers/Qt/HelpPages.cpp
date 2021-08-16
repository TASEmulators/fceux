// HelpPages.cpp

#include <string>

#include "Qt/HelpPages.h"

#ifdef WIN32
#include <htmlhelp.h>

void OpenHelpWindow(std::string subpage)
{
	std::string helpFileName = BaseDirectory;
	helpFileName += "\\fceux.chm";
	if (subpage.length() > 0)
	{
		helpFileName = helpFileName + "::/" + subpage + ".htm";
	}
	HtmlHelp(GetDesktopWindow(), helpFileName.c_str(), HH_DISPLAY_TOPIC, (DWORD)NULL);
}

#else

void OpenHelpWindow(std::string subpage)
{
	printf("Local CHM Help File Reference is only supported on Windows OS\n");
}

#endif
