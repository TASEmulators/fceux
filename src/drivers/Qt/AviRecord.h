// AviRecord.h
//

#ifndef __AVI_RECORD_H__
#define __AVI_RECORD_H__

#include <stdint.h>
#include <string>
#include <vector>
#include <list>

#include <QThread>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>

enum aviEncoderList
{
	AVI_RGB24 = 0,
	AVI_I420,
	#ifdef _USE_X264
	AVI_X264,
	#endif
	#ifdef _USE_X265
	AVI_X265,
	#endif
	#ifdef _USE_LIBAV
	AVI_LIBAV,
	#endif
	#ifdef WIN32
	AVI_VFW,
	#endif
	AVI_NUM_ENC
};

int aviRecordOpenFile( const char *filepath );

int aviRecordAddFrame( void );

int aviRecordAddAudioFrame( int32_t *buf, int numSamples );

int aviRecordClose(void);

bool aviRecordRunning(void);

bool aviGetAudioEnable(void);

void aviSetAudioEnable(bool val);

int aviGetSelVideoFormat(void);

void aviSetSelVideoFormat(int idx);

int FCEUD_AviGetFormatOpts( std::vector <std::string> &formatList );

int aviDebugOpenFile( const char *filepath );

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

#ifdef _USE_LIBAV
class  LibavOptionsPage : public QWidget
{
	Q_OBJECT

	public:
		LibavOptionsPage(QWidget *parent = nullptr);
		~LibavOptionsPage(void);

	protected:
		QComboBox  *videoEncSel;
		QComboBox  *audioEncSel;
		QGroupBox  *videoGbox;
		QGroupBox  *audioGbox;

		void initCodecLists(void);
		void initPixelFormatSelect(int codec_id);

	private slots:
		void videoCodecChanged(int idx);
		void audioCodecChanged(int idx);
};

#endif

#endif
