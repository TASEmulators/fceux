// AviRecord.h
//

#ifndef __AVI_RECORD_H__
#define __AVI_RECORD_H__

#include <stdint.h>
#include <QThread>

int aviRecordOpenFile( const char *filepath );

int aviRecordAddFrame( void );

int aviRecordAddAudioFrame( int32_t *buf, int numSamples );

int aviRecordClose(void);

bool aviRecordRunning(void);

class  AviRecordDiskThread_t : public QThread
{
	Q_OBJECT

	protected:
		void run( void ) override;

	public:
		AviRecordDiskThread_t( QObject *parent = 0 );
		~AviRecordDiskThread_t(void);

	private:

	signals:
		void finished(void);
};
#endif
