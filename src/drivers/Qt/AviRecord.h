// AviRecord.h
//
#include <stdint.h>

int aviRecordOpenFile( const char *filepath, int format, int width, int height );

int aviRecordAddFrame( void );

int aviRecordAddAudioFrame( int32_t *buf, int numSamples );

int aviRecordClose(void);

bool aviRecordRunning(void);
