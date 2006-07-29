#include "types.h"
#include "fceu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char *aboutString = 0;

char *FCEUI_GetAboutString() {
	const char *aboutTemplate = 
	FCEU_NAME_AND_VERSION"\n\
~CAST~\n\
FCE - Bero\n\
FCEU - Xodnizel\n\
FCEU XD - Bbitmaster & Parasyte\n\
FCEU XD SP - Sebastian Porst\n\
FCEU MM - CaH4e3\n\
FCEU TAS - blip & nitsuja\n\
FCEU TAS+ - Luke Gustafson\n\
FCEUX\n\
 - CaH4e3, Luke Gustafson\n\
 - Matthew Gambrell, Sebastian Porst\n\
\n\
"__TIME__" "__DATE__"\n";

		char *compilerString = FCEUD_GetCompilerString();

	//allocate the string and concatenate the template with the compiler string
	if(aboutString) free(aboutString);
	aboutString = (char*)malloc(strlen(aboutTemplate) + strlen(compilerString) + 1);
	strcpy(aboutString,aboutTemplate);
	strcat(aboutString,compilerString);
	return aboutString;
}