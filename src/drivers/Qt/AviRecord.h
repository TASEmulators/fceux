// AviRecord.h
//

int aviRecordOpenFile( const char *filepath, int format, int width, int height );

int aviRecordAddFrame( void );

int aviRecordClose(void);

