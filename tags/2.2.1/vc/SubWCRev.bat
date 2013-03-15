SubWCRev.exe .. "defaultconfig\svnrev_template.h" "userconfig\svnrev.h"
IF NOT EXIST defaultconfig\makedownload exit 0
SubWCRev.exe .. "..\..\web\download.html" "userconfig\download.html"
exit 0