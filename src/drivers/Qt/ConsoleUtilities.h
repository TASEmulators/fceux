// ConsoleUtilities.h

int  getDirFromFile( const char *path, char *dir );

const char *getRomFile( void );

int getFileBaseName( const char *filepath, char *base );

int parseFilepath( const char *filepath, char *dir, char *base, char *suffix = NULL );
