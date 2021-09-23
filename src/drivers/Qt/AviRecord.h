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
#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

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

struct AVOption;
struct AVCodecContext;

class LibavEncOptItem : public QTreeWidgetItem
{
	//Q_OBJECT

	public:
		LibavEncOptItem(QTreeWidgetItem *parent = nullptr);
		~LibavEncOptItem(void);

		void setValueText(void);

		void *obj;
		const AVOption *opt;

		std::vector <const AVOption*> units;
};

class LibavEncOptInputWin : public QDialog
{
	Q_OBJECT

public:
	LibavEncOptInputWin( LibavEncOptItem *item, QWidget *parent = 0);
	~LibavEncOptInputWin(void);

protected:
	void closeEvent(QCloseEvent *event);

	LibavEncOptItem *item;
	QComboBox       *combo;
	QSpinBox        *intEntry;
	QDoubleSpinBox  *floatEntry;
	QSpinBox        *numEntry;
	QSpinBox        *denEntry;
	QLineEdit       *strEntry;
	QPushButton     *okButton;
        QPushButton     *cancelButton;
        QPushButton     *resetDefaults;
	std::vector <QCheckBox*> chkBox;

public slots:
	void closeWindow(void);
private slots:
	void applyChanges(void);
	void resetDefaultsCB(void);
};

class LibavEncOptWin : public QDialog
{
	Q_OBJECT

public:
	LibavEncOptWin(int type, QWidget *parent = 0);
	~LibavEncOptWin(void);

protected:
	void closeEvent(QCloseEvent *event);
	void updateItems(void);

	int type;
	QTreeWidget *tree;
	AVCodecContext *ctx;
	const char *codec_name;

public slots:
	void closeWindow(void);
private slots:
	void resetDefaultsCB(void);
	void editWindowFinished(int);
	void itemChangeActivated( QTreeWidgetItem *item, int col);
	//void hotKeyActivated(QTreeWidgetItem *item, int column);
	//void hotKeyDoubleClicked(QTreeWidgetItem *item, int column);
};

class  LibavOptionsPage : public QWidget
{
	Q_OBJECT

	public:
		LibavOptionsPage(QWidget *parent = nullptr);
		~LibavOptionsPage(void);

	protected:
		QComboBox  *videoEncSel;
		QComboBox  *videoPixfmt;
		QComboBox  *audioEncSel;
		QComboBox  *audioSamplefmt;
		QComboBox  *audioSampleRate;
		QComboBox  *audioChanLayout;
		QGroupBox  *videoGbox;
		QGroupBox  *audioGbox;

		void initCodecLists(void);
		void initPixelFormatSelect(const char *codec_name);
		void initSampleFormatSelect(const char *codec_name);
		void initSampleRateSelect(const char *codec_name);
		void initChannelLayoutSelect(const char *codec_name);

	private slots:
		void includeAudioChanged(bool);
		void openVideoCodecOptions(void);
		void openAudioCodecOptions(void);
		void videoCodecChanged(int idx);
		void audioCodecChanged(int idx);
		void videoPixelFormatChanged(int idx);
		void audioSampleFormatChanged(int idx);
		void audioSampleRateChanged(int idx);
		void audioChannelLayoutChanged(int idx);
};

#endif

#endif
