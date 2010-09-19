/// \file
/// \brief Contains methods related to the build configuration

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "version.h"
#include "fceu.h"
#include "driver.h"
#include "utils/memory.h"

static char *aboutString = 0;

///returns a string suitable for use in an aboutbox
char *FCEUI_GetAboutString() {
	const char *aboutTemplate = 
	FCEU_NAME_AND_VERSION "\n\n\
Authors:\n\
zeromus, adelikat,\n\n\
Contributers:\n\
Acmlm,CaH4e3\n\
DWEdit,QFox\n\
qeed,Shinydoofy,ugetab\n\
\n\
FCEUX 2.0\n\
mz, nitsujrehtona, Lukas Sabota,\n\
SP, Ugly Joe\n\
\n\n\
Previous versions:\n\n\
FCE - Bero\n\
FCEU - Xodnizel\n\
FCEU XD - Bbitmaster & Parasyte\n\
FCEU XD SP - Sebastian Porst\n\
FCEU MM - CaH4e3\n\
FCEU TAS - blip & nitsuja\n\
FCEU TAS+ - Luke Gustafson\n\
\n\
"__TIME__" "__DATE__"\n";

	if(aboutString) return aboutString;

	const char *compilerString = FCEUD_GetCompilerString();

	//allocate the string and concatenate the template with the compiler string
	if (!(aboutString = (char*)FCEU_dmalloc(strlen(aboutTemplate) + strlen(compilerString) + 1)))
        return NULL;
	
    sprintf(aboutString,"%s%s",aboutTemplate,compilerString);
	return aboutString;
}
