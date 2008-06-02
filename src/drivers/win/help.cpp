#include "main.h"
#include "help.h"
#include <htmlhelp.h>

void OpenHelpWindow(const char *subpage)
{
	char helpFileName[MAX_PATH];
	strcpy(helpFileName, BaseDirectory);
	strcat(helpFileName, "\\fceux.chm");
	if (subpage)
	{
		strcat(helpFileName, "::/");
		strcat(helpFileName, subpage);
		strcat(helpFileName, ".htm");
	}
	HtmlHelp(GetDesktopWindow(), helpFileName, HH_DISPLAY_TOPIC, (DWORD)NULL);
}
