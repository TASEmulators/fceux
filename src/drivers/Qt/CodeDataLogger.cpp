// CodeDataLogger.cpp
//
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../x6502.h"
#include "../../debug.h"
#include "../../ppu.h"

#include "Qt/ConsoleUtilities.h"
#include "Qt/CodeDataLogger.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"

static int  autoSaveCDL = true;
static int  autoLoadCDL = true;
static int  autoResumeCDL = false;
static char loadedcdfile[512] = {0};

static int getDefaultCDLFile( char *filepath );
//----------------------------------------------------
CodeDataLoggerDialog_t::CodeDataLoggerDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox *frame, *subframe;
	QPushButton *btn;

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &CodeDataLoggerDialog_t::updatePeriodic );

   setWindowTitle( tr("Code Data Logger") );

	mainLayout   = new QVBoxLayout();
	vbox1        = new QVBoxLayout();
	hbox         = new QHBoxLayout();
	grid         = new QGridLayout();
	statLabel    = new QLabel( tr(" Logger is Paused: Press Start to Run ") );
	cdlFileLabel = new QLabel( tr("CDL File:") );

	vbox1->addLayout( grid );
	vbox1->addLayout( hbox  );
	vbox1->addWidget( cdlFileLabel );

	hbox->addWidget( statLabel, 0, Qt::AlignHCenter );

	frame = new QGroupBox(tr("Code/Data Log Status"));
	frame->setLayout( vbox1 );

	prgLoggedCodeLabel = new QLabel( tr("0x000000 0.00%") );
	prgLoggedDataLabel = new QLabel( tr("0x000000 0.00%") );
	prgUnloggedLabel   = new QLabel( tr("0x000000 0.00%") );
	chrLoggedCodeLabel = new QLabel( tr("0x000000 0.00%") );
	chrLoggedDataLabel = new QLabel( tr("0x000000 0.00%") );
	chrUnloggedLabel   = new QLabel( tr("0x000000 0.00%") );
	autoSaveCdlCbox    = new QCheckBox( tr("Auto-save .CDL when closing ROMs") );
	autoLoadCdlCbox    = new QCheckBox( tr("Auto-load .CDL when opening this window") );
	autoResumeLogCbox  = new QCheckBox( tr("Auto-resume logging when loading ROMs") );

	g_config->getOption("SDL.AutoSaveCDL", &autoSaveCDL);
	g_config->getOption("SDL.AutoLoadCDL", &autoLoadCDL);
	g_config->getOption("SDL.AutoResumeCDL", &autoResumeCDL);

	autoSaveCdlCbox->setChecked( autoSaveCDL );
	autoLoadCdlCbox->setChecked( autoLoadCDL );
	autoResumeLogCbox->setChecked( autoResumeCDL );

	connect(autoSaveCdlCbox  , SIGNAL(stateChanged(int)), this, SLOT(autoSaveCdlStateChange(int)) );
	connect(autoLoadCdlCbox  , SIGNAL(stateChanged(int)), this, SLOT(autoLoadCdlStateChange(int)) );
	connect(autoResumeLogCbox, SIGNAL(stateChanged(int)), this, SLOT(autoResumeCdlStateChange(int)) );

	subframe = new QGroupBox(tr("PRG Logged as Code"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgLoggedCodeLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 0, Qt::AlignCenter );

	subframe = new QGroupBox(tr("PRG Logged as Data"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgLoggedDataLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 1, Qt::AlignCenter );

	subframe = new QGroupBox(tr("PRG not Logged"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( prgUnloggedLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 0, 2, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR Logged as Code"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrLoggedCodeLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 0, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR Logged as Data"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrLoggedDataLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 1, Qt::AlignCenter );

	subframe = new QGroupBox(tr("CHR not Logged"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( chrUnloggedLabel );
	subframe->setLayout( vbox );

	grid->addWidget( subframe, 1, 2, Qt::AlignCenter );

	grid = new QGridLayout();
	vbox1->addLayout( grid );
	btn = new QPushButton( tr("Reset Log") );
	grid->addWidget( btn, 0, 0, Qt::AlignCenter );
   connect( btn, SIGNAL(clicked(void)), this, SLOT(ResetCDLogClicked(void)));

	startPauseButton = new QPushButton( tr("Start") );
	grid->addWidget( startPauseButton, 0, 1, Qt::AlignCenter );
   connect( startPauseButton, SIGNAL(clicked(void)), this, SLOT(StartPauseCDLogClicked(void)));

	btn = new QPushButton( tr("Save") );
	grid->addWidget( btn, 0, 2, Qt::AlignCenter );
   connect( btn, SIGNAL(clicked(void)), this, SLOT(saveCdlFile(void)));

	btn = new QPushButton( tr("Load") );
	grid->addWidget( btn, 1, 0, Qt::AlignCenter );
   connect( btn, SIGNAL(clicked(void)), this, SLOT(loadCdlFile(void)));

	btn = new QPushButton( tr("Save As") );
	grid->addWidget( btn, 1, 2, Qt::AlignCenter );
   connect( btn, SIGNAL(clicked(void)), this, SLOT(saveCdlFileAs(void)));

	hbox = new QHBoxLayout();
	vbox1->addLayout( hbox );

	subframe = new QGroupBox(tr("Logging Workflow Options"));
	vbox     = new QVBoxLayout();
	vbox->addWidget( autoSaveCdlCbox );
	vbox->addWidget( autoLoadCdlCbox );
	vbox->addWidget( autoResumeLogCbox );
	subframe->setLayout( vbox );
	hbox->addWidget( subframe );

	subframe = new QGroupBox(tr("Generate ROM"));
	vbox     = new QVBoxLayout();

	btn = new QPushButton( tr("Save Stripped Data") );
	vbox->addWidget( btn );
	btn = new QPushButton( tr("Save Unused Data") );
	vbox->addWidget( btn );
	subframe->setLayout( vbox );
	hbox->addWidget( subframe );

	mainLayout->addWidget( frame );

	setLayout( mainLayout );

   updateTimer->start( 100 ); // 10hz

	if (autoLoadCDL)
	{
		char nameo[2048];
		getDefaultCDLFile( nameo );
		LoadCDLog(nameo);
	}
}
//----------------------------------------------------
CodeDataLoggerDialog_t::~CodeDataLoggerDialog_t(void)
{
   updateTimer->stop();

	printf("Code Data Logger Window Deleted\n");
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Code Data Logger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::closeWindow(void)
{
   printf("Code Data Logger Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::autoSaveCdlStateChange(int state)
{
	autoSaveCDL = state != Qt::Unchecked;

	g_config->setOption("SDL.AutoSaveCDL", autoSaveCDL);
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::autoLoadCdlStateChange(int state)
{
	autoLoadCDL = state != Qt::Unchecked;

	g_config->setOption("SDL.AutoLoadCDL", autoLoadCDL);
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::autoResumeCdlStateChange(int state)
{
	autoResumeCDL = state != Qt::Unchecked;

	g_config->setOption("SDL.AutoResumeCDL", autoResumeCDL);
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::updatePeriodic(void)
{
	char str[768];
	float fcodecount = codecount;
	float fdatacount = datacount;
	float frendercount = rendercount;
	float fvromreadcount = vromreadcount;
	float fundefinedcount = undefinedcount;
	float fundefinedvromcount = undefinedvromcount;
	float fromsize = cdloggerdataSize;
	float fvromsize = (cdloggerVideoDataSize != 0) ? cdloggerVideoDataSize : 1;

	if ( FCEUI_GetLoggingCD() )
	{
		startPauseButton->setText( tr("Pause") );
		statLabel->setText( tr(" Logger is Running: Press Pause to Stop ") );
		statLabel->setStyleSheet("background-color: green; color: white;");
	}
	else
	{
		startPauseButton->setText( tr("Start") );
		statLabel->setText( tr(" Logger is Paused: Press Start to Run ") );
		statLabel->setStyleSheet("background-color: red; color: white;");
	}

	if ( cdloggerdataSize > 0 )
	{
	   sprintf(str,"0x%06x  %.2f%%", codecount, (fcodecount / fromsize) * 100);
	   prgLoggedCodeLabel->setText( tr(str) );

	   sprintf(str,"0x%06x  %.2f%%", datacount,(fdatacount / fromsize) * 100);
	   prgLoggedDataLabel->setText( tr(str) );

	   sprintf(str,"0x%06x  %.2f%%", undefinedcount, (fundefinedcount / fromsize) * 100);
	   prgUnloggedLabel->setText( tr(str) );

	   sprintf(str,"0x%06x  %.2f%%", rendercount, (frendercount / fvromsize) * 100);
	   chrLoggedCodeLabel->setText( tr(str) );

	   sprintf(str,"0x%06x  %.2f%%", vromreadcount, (fvromreadcount / fvromsize) * 100);
	   chrLoggedDataLabel->setText( tr(str) );

	   sprintf(str,"0x%06x  %.2f%%", undefinedvromcount, (fundefinedvromcount / fvromsize) * 100);
	   chrUnloggedLabel->setText( tr(str) );
	}
	else
	{
	   prgLoggedCodeLabel->setText( tr("------") );
	   prgLoggedDataLabel->setText( tr("------") );
	   prgUnloggedLabel->setText( tr("------") );
	   chrLoggedCodeLabel->setText( tr("------") );
	   chrLoggedDataLabel->setText( tr("------") );
	   chrUnloggedLabel->setText( tr("------") );
	}

	sprintf( str, "CDL File: %s", loadedcdfile );

	cdlFileLabel->setText( tr(str) );
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::ResetCDLogClicked(void)
{
	::ResetCDLog();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::StartPauseCDLogClicked(void)
{
	if ( FCEUI_GetLoggingCD() )
	{
		//printf("CD Logging Paused\n");
		PauseCDLogging();
		startPauseButton->setText( tr("Start") );
	}
	else
	{
		//printf("CD Logging Started\n");
		StartCDLogging();
		startPauseButton->setText( tr("Pause") );
	}
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::saveCdlFile(void)
{
	SaveCDLogFile();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::saveCdlFileAs(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	const char *romFile;
	QFileDialog  dialog(this, tr("Save CDL To File") );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("CDL Files (*.cdl *.CDL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".cdl") );

	romFile = getRomFile();

	if ( romFile != NULL )
	{
		char dir[512], base[256];

		parseFilepath( romFile, dir, base );

		strcat( base, ".cdl");

		dialog.setDirectory( tr(dir) );

		dialog.selectFile( tr(base) );
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

	if ( filename.isNull() )
   {
      return;
   }
	//qDebug() << "selected file path : " << filename.toUtf8();

	fceuWrapperLock();
	strcpy( loadedcdfile, filename.toStdString().c_str() );
	SaveCDLogFile();
	fceuWrapperUnLock();
}
//----------------------------------------------------
void CodeDataLoggerDialog_t::loadCdlFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	char dir[512];
	const char *romFile;
	QFileDialog  dialog(this, tr("Load CDL File") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("CDL files (*.cdl *.CDL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	romFile = getRomFile();

	if ( romFile )
	{
		getDirFromFile( romFile, dir );

		dialog.setDirectory( tr(dir) );
	}

	// Check config option to use native file dialog or not
	g_config->getOption ("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);

	dialog.show();
	ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			filename = fileList[0];
		}
	}

   if ( filename.isNull() )
   {
      return;
   }
	//qDebug() << "selected file path : " << filename.toUtf8();

	fceuWrapperLock();
	LoadCDLog ( filename.toStdString().c_str() );
	fceuWrapperUnLock();

   return;
}
//----------------------------------------------------
static int getDefaultCDLFile( char *filepath )
{
	const char *romFile;
	char dir[512], baseFile[256];

	filepath[0] = 0;

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}

	parseFilepath( romFile, dir, baseFile );
	
	sprintf( filepath, "%s/%s.cdl", dir, baseFile );

	//printf("%s\n", filepath );

	return 0;
}
//----------------------------------------------------
void FreeCDLog(void)
{
	fceuWrapperLock();
	if (cdloggerdata)
	{
		free(cdloggerdata);
		cdloggerdata = NULL;
		cdloggerdataSize = 0;
	}
	if (cdloggervdata)
	{
		free(cdloggervdata);
		cdloggervdata = NULL;
		cdloggerVideoDataSize = 0;
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------
void InitCDLog(void)
{
	fceuWrapperLock();
	cdloggerdataSize = PRGsize[0];
	cdloggerdata = (unsigned char*)malloc(cdloggerdataSize);
	if (!CHRram[0] || (CHRptr[0] == PRGptr[0])) {	// Some kind of workaround for my OneBus VRAM hack, will remove it if I find another solution for that
		cdloggerVideoDataSize = CHRsize[0];
		cdloggervdata = (unsigned char*)malloc(cdloggerVideoDataSize);
	} else {
		if (GameInfo->type != GIT_NSF) {
			cdloggerVideoDataSize = 0;
			cdloggervdata = (unsigned char*)malloc(8192);
		}
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------
void ResetCDLog(void)
{
	if ( GameInfo == NULL )
	{
		return;
	}
	fceuWrapperLock();

	codecount = datacount = rendercount = vromreadcount = 0;
	undefinedcount = cdloggerdataSize;

	if ( cdloggerdata != NULL )
	{
		memset(cdloggerdata, 0, cdloggerdataSize);
	}
	if (cdloggerVideoDataSize != 0) 
	{
		undefinedvromcount = cdloggerVideoDataSize;

		if ( cdloggervdata != NULL )
		{
			memset(cdloggervdata, 0, cdloggerVideoDataSize);
		}
	}
  	else
  	{
		if (GameInfo->type != GIT_NSF) 
		{
			undefinedvromcount = 8192;
			memset(cdloggervdata, 0, 8192);
		}
	}
	fceuWrapperUnLock();
}
//----------------------------------------------------
bool LoadCDLog(const char* nameo)
{
	FILE *FP;
	int i,j;

	FP = fopen(nameo, "rb");
	if (FP == NULL)
	{
		return false;
	}

	for(i = 0;i < (int)cdloggerdataSize;i++)
	{
		j = fgetc(FP);
		if (j == EOF)
			break;
		if ((j & 1) && !(cdloggerdata[i] & 1))
			codecount++; //if the new byte has something logged and
		if ((j & 2) && !(cdloggerdata[i] & 2))
			datacount++; //and the old one doesn't. Then increment
		if ((j & 3) && !(cdloggerdata[i] & 3))
			undefinedcount--; //the appropriate counter.
		cdloggerdata[i] |= j;
	}

	if(cdloggerVideoDataSize != 0)
	{
		for(i = 0;i < (int)cdloggerVideoDataSize;i++)
		{
			j = fgetc(FP);
			if(j == EOF)break;
			if((j & 1) && !(cdloggervdata[i] & 1))rendercount++; //if the new byte has something logged and
			if((j & 2) && !(cdloggervdata[i] & 2))vromreadcount++; //if the new byte has something logged and
			if((j & 3) && !(cdloggervdata[i] & 3))undefinedvromcount--; //the appropriate counter.
			cdloggervdata[i] |= j;
		}
	}

	fclose(FP);
	RenameCDLog(nameo);

	return true;
}
//----------------------------------------------------
void StartCDLogging(void)
{
	fceuWrapperLock();
	FCEUI_SetLoggingCD(1);
	//EnableTracerMenuItems();
	//SetDlgItemText(hCDLogger, BTN_CDLOGGER_START_PAUSE, "Pause");
	fceuWrapperUnLock();
}
//----------------------------------------------------
bool PauseCDLogging(void)
{
	// can't pause while Trace Logger is using
	//if ((logging) && (logging_options & LOG_NEW_INSTRUCTIONS))
	//{
	//	MessageBox(hCDLogger, "The Trace Logger is currently using this for some of its features.\nPlease turn the Trace Logger off and try again.","Unable to Pause Code/Data Logger", MB_OK);
	//	return false;
	//}
	fceuWrapperLock();
	FCEUI_SetLoggingCD(0);
	//EnableTracerMenuItems();
	//SetDlgItemText(hCDLogger, BTN_CDLOGGER_START_PAUSE, "Start");
	fceuWrapperUnLock();
	return true;
}
//----------------------------------------------------
void CDLoggerROMClosed(void)
{
	PauseCDLogging();
	if (autoSaveCDL)
	{
		SaveCDLogFile();
	}
}
//----------------------------------------------------
void CDLoggerROMChanged(void)
{
	FreeCDLog();
	InitCDLog();
	ResetCDLog();
	RenameCDLog("");

	if (!autoResumeCDL)
		return;

	// try to load respective CDL file
	char nameo[1024];
	getDefaultCDLFile( nameo );

	FILE *FP;
	FP = fopen(nameo, "rb");
	if (FP != NULL)
	{
		// .cdl file with this ROM name exists
		fclose(FP);
		//if (!hCDLogger)
		//{
		//	DoCDLogger();
		//}
		if (LoadCDLog(nameo))
		{
			StartCDLogging();
		}
	}
}
//----------------------------------------------------
void RenameCDLog(const char* newName)
{
	strcpy(loadedcdfile, newName);
}
//----------------------------------------------------
void SaveCDLogFile(void)
{
	if (loadedcdfile[0] == 0)
	{
		char nameo[1024];
		getDefaultCDLFile( nameo );
		RenameCDLog(nameo);
	}

	FILE *FP;
	FP = fopen(loadedcdfile, "wb");
	if (FP == NULL)
	{
		FCEUD_PrintError("Error Saving File");
		return;
	}
	fwrite(cdloggerdata, cdloggerdataSize, 1, FP);
	if (cdloggerVideoDataSize != 0)
	{
		fwrite(cdloggervdata, cdloggerVideoDataSize, 1, FP);
	}
	fclose(FP);
}
//----------------------------------------------------
