// HelpPages.cpp

#include <string>

#include "driver.h"
#include "Qt/HelpPages.h"
#include "Qt/ConsoleWindow.h"

#ifdef WIN32
#include <Windows.h>
#include <htmlhelp.h>

void OpenHelpWindow(std::string subpage)
{
	HWND helpWin;
	std::string helpFileName = FCEUI_GetBaseDirectory();
	helpFileName += "\\..\\doc\\fceux.chm";
	if (subpage.length() > 0)
	{
		helpFileName = helpFileName + "::/" + subpage + ".htm";
	}
	//printf("Looking for HelpFile '%s'\n", helpFileName.c_str() );
	helpWin = HtmlHelp( HWND(consoleWindow->winId()), helpFileName.c_str(), HH_DISPLAY_TOPIC, (DWORD)NULL);
	if ( helpWin == NULL )
	{
		printf("Error: Failed to open help file '%s'\n", helpFileName.c_str() );
	}
}

#else

void OpenHelpWindow(std::string subpage)
{
	printf("Local CHM Help File Reference is only supported on Windows OS\n");
}

#endif
