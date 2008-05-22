/// \file
/// \brief Contains methods related to the build configuration

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "fceu.h"

static char *aboutString = 0;

///returns a string suitable for use in an aboutbox
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
 - rheiny, zeromus\n\
 - CaH4e3, Luke Gustafson, Sebastian Porst\n\
 - adelikat, _mz\n\
\n\
"__TIME__" "__DATE__"\n";

	if(aboutString) return aboutString;

	const char *compilerString = FCEUD_GetCompilerString();

	//allocate the string and concatenate the template with the compiler string
	aboutString = (char*)malloc(strlen(aboutTemplate) + strlen(compilerString) + 1);
	sprintf(aboutString,"%s%s",aboutTemplate,compilerString);
	return aboutString;
}
