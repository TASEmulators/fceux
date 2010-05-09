defaultconfig\SubWCRev.exe ..\.. ".\defaultconfig\svnrev_template.h" ".\userconfig\svnrev.h"
IF NOT EXIST defaultconfig\makedownload exit 0
defaultconfig\SubWCWeb.exe ..\.. "..\..\web\download.html" ".\userconfig\download.html"
exit 0