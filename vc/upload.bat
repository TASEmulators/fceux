pscp -pw %1 fceux.zip fceux@www.pi-r-squared.com:web/zip/fceux.zip
IF NOT EXIST fceux.zip GOTO end
IF NOT EXIST defaultconfig\makedownload GOTO end
pscp -pw %1 .\userconfig\download.html fceux@www.pi-r-squared.com:web/web/download.html
:end