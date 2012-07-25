SubWCRev.exe ..\.. "svnrev_template.h" "..\userconfig\svnrev.h"
IF NOT EXIST makedownload exit 0
SubWCWeb.exe ..\.. "..\..\..\web\download.html" "..\userconfig\download.html"
exit 0