// InputConf.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGroupBox>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/InputConf.h"

static InputConfDialog_t *win = NULL;
//----------------------------------------------------------------------------
void openInputConfWindow( QWidget *parent )
{
	if ( win != NULL )
	{
		return;
	}
	win = new InputConfDialog_t(parent);

	win->show();
}
//----------------------------------------------------------------------------
InputConfDialog_t::InputConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout, *vbox1, *vbox;
	QHBoxLayout *hbox;
	QGroupBox   *nesInputFrame, *port1Frame, *port2Frame;
	QGroupBox   *presetFrame, *expansionPortFrame;
	QPalette     pal;
	QColor       color;
	char stmp[256];
	int  fourscore, autoInputPreset;

	pal = this->palette();

	inputTimer  = new QTimer( this );

   connect( inputTimer, &QTimer::timeout, this, &InputConfDialog_t::updatePeriodic );

	setWindowTitle("Input Configuration");

	mainLayout = new QVBoxLayout();

	nesInputFrame = new QGroupBox( tr("NES-Style Input Ports") );
	vbox1         = new QVBoxLayout();
	hbox          = new QHBoxLayout();
	fourScoreEna  = new QCheckBox( tr("Attach 4-Score (Implies four gamepads)") );
	port2Mic      = new QCheckBox( tr("Replace Port 2 Start with Microphone") );
	autoPreset    = new QCheckBox( tr("Auto Load/Save Presets at ROM Open/Close") );

	g_config->getOption("SDL.FourScore", &fourscore);
	fourScoreEna->setChecked( fourscore );
	port2Mic->setChecked( replaceP2StartWithMicrophone );

	g_config->getOption( "SDL.AutoInputPreset", &autoInputPreset );
	autoPreset->setChecked( autoInputPreset );

	hbox->addWidget( fourScoreEna );
	hbox->addWidget( port2Mic );
	vbox1->addLayout( hbox );

	hbox       = new QHBoxLayout();
	port1Frame = new QGroupBox( tr("Port 1:") );
	port2Frame = new QGroupBox( tr("Port 2:") );

	hbox->addWidget( port1Frame );
	hbox->addWidget( port2Frame );
	vbox1->addLayout( hbox );

	nesPortComboxBox[0] = new QComboBox();
	nesPortComboxBox[1] = new QComboBox();
	expPortComboxBox    = new QComboBox();

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();

	vbox->addLayout( hbox );
	hbox->addWidget( nesPortLabel[0]      = new QLabel( tr("<None>") ) );
	hbox->addWidget( nesPortConfButton[0] = new QPushButton( tr("Configure") ) );
	vbox->addWidget( nesPortComboxBox[0] );

	port1Frame->setLayout( vbox );

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();

	vbox->addLayout( hbox );
	hbox->addWidget( nesPortLabel[1]      = new QLabel( tr("<None>") ) );
	hbox->addWidget( nesPortConfButton[1] = new QPushButton( tr("Configure") ) );
	vbox->addWidget( nesPortComboxBox[1] );

	port2Frame->setLayout( vbox );

	nesInputFrame->setLayout( vbox1 );
	nesPortConfButton[0]->setEnabled(false);
	nesPortConfButton[1]->setEnabled(false);

	mainLayout->addWidget( nesInputFrame );

	hbox               = new QHBoxLayout();
	presetFrame        = new QGroupBox( tr("Input Presets:") );
	expansionPortFrame = new QGroupBox( tr("Famicom Expansion Port:") );

	hbox->addWidget( presetFrame );
	hbox->addWidget( expansionPortFrame );

	mainLayout->addLayout( hbox );

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( autoPreset );

	hbox  = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( loadConfigButton = new QPushButton( tr("Load") ) );
	hbox->addWidget( saveConfigButton = new QPushButton( tr("Save") ) );

	presetFrame->setLayout( vbox );

	vbox  = new QVBoxLayout();
	hbox  = new QHBoxLayout();

	vbox->addLayout( hbox );
	hbox->addWidget( expPortLabel      = new QLabel( tr("<None>") ) );
	hbox->addWidget( expPortConfButton = new QPushButton( tr("Configure") ) );
	vbox->addWidget( expPortComboxBox );

	expPortConfButton->setEnabled(false);
	expansionPortFrame->setLayout( vbox );

	color = pal.color(QPalette::WindowText);

	sprintf( stmp, "border: 2px solid #%02X%02X%02X", color.red(), color.green(), color.blue() );
			
	//printf("%s\n", stmp);
	nesPortLabel[0]->setAlignment(Qt::AlignCenter);
	nesPortLabel[1]->setAlignment(Qt::AlignCenter);
	expPortLabel->setAlignment(Qt::AlignCenter);
	nesPortLabel[0]->setStyleSheet( stmp );
	nesPortLabel[1]->setStyleSheet( stmp );
	expPortLabel->setStyleSheet( stmp );

	setLayout( mainLayout );

	for (int i=0; i<2; i++)
	{
		getInputSelection( i, &curNesInput[i], &usrNesInput[i] );
		nesPortComboxBox[i]->addItem( tr("<None>")          , SI_NONE      );
		nesPortComboxBox[i]->addItem( tr("Gamepad")         , SI_GAMEPAD   );
		nesPortComboxBox[i]->addItem( tr("Zapper")          , SI_ZAPPER    );
		nesPortComboxBox[i]->addItem( tr("Power Pad A")     , SI_POWERPADA );
		nesPortComboxBox[i]->addItem( tr("Power Pad B")     , SI_POWERPADB );
		nesPortComboxBox[i]->addItem( tr("Arkanoid Paddle") , SI_ARKANOID  );

		for (int j=0; j<nesPortComboxBox[i]->count(); j++)
		{
			if ( nesPortComboxBox[i]->itemData(j).toInt() == curNesInput[i] )
			{
				nesPortComboxBox[i]->setCurrentIndex( j );
			}
			if ( nesPortComboxBox[i]->itemData(j).toInt() == curNesInput[i] )
			{
				nesPortLabel[i]->setText( nesPortComboxBox[i]->itemText(j) );
			}
		}
	}

	getInputSelection( 2, &curNesInput[2], &usrNesInput[2] );
	expPortComboxBox->addItem( tr("<None>")           , SIFC_NONE         );
	expPortComboxBox->addItem( tr("Arkanoid Paddle")  , SIFC_ARKANOID     );
	expPortComboxBox->addItem( tr("Shadow")           , SIFC_SHADOW       );
	expPortComboxBox->addItem( tr("Hyper Shot Gun")   , SIFC_HYPERSHOT    );
	expPortComboxBox->addItem( tr("Family Keyboard")  , SIFC_FKB          );
	expPortComboxBox->addItem( tr("Mahjong")          , SIFC_MAHJONG      );
	expPortComboxBox->addItem( tr("Quiz King Buzzers"), SIFC_QUIZKING     );
	expPortComboxBox->addItem( tr("Family Trainer A") , SIFC_FTRAINERA    );
	expPortComboxBox->addItem( tr("Family Trainer B") , SIFC_FTRAINERB    );
	expPortComboxBox->addItem( tr("Oeka Kids Tablet") , SIFC_OEKAKIDS     );
	expPortComboxBox->addItem( tr("Top Rider")        , SIFC_TOPRIDER     );

	for (int j=0; j<expPortComboxBox->count(); j++)
	{
		if ( expPortComboxBox->itemData(j).toInt() == curNesInput[2] )
		{
			expPortComboxBox->setCurrentIndex( j );
		}
		if ( expPortComboxBox->itemData(j).toInt() == curNesInput[2] )
		{
			expPortLabel->setText( expPortComboxBox->itemText(j) );
		}
	}

	connect( fourScoreEna, SIGNAL(stateChanged(int)), this, SLOT(fourScoreChanged(int)) );
	connect( port2Mic    , SIGNAL(stateChanged(int)), this, SLOT(port2MicChanged(int))  );
	connect( autoPreset  , SIGNAL(stateChanged(int)), this, SLOT(autoPresetChanged(int)));

	connect( nesPortComboxBox[0], SIGNAL(activated(int)), this, SLOT(port1Select(int)) );
	connect( nesPortComboxBox[1], SIGNAL(activated(int)), this, SLOT(port2Select(int)) );
	connect( expPortComboxBox   , SIGNAL(activated(int)), this, SLOT(expSelect(int))   );

	connect( nesPortConfButton[0], SIGNAL(clicked(void)), this, SLOT(port1Configure(void)) );
	connect( nesPortConfButton[1], SIGNAL(clicked(void)), this, SLOT(port2Configure(void)) );

	connect( loadConfigButton, SIGNAL(clicked(void)), this, SLOT(openLoadPresetFile(void)) );
	connect( saveConfigButton, SIGNAL(clicked(void)), this, SLOT(openSavePresetFile(void)) );

	updatePortLabels();

	inputTimer->start( 500 ); // 2hz
}
//----------------------------------------------------------------------------
InputConfDialog_t::~InputConfDialog_t(void)
{
	printf("Destroy Input Config Window\n");
	inputTimer->stop();

	if ( win == this )
	{
		win = NULL;
	}
}
//----------------------------------------------------------------------------
void InputConfDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Input Config Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::setInputs(void)
{
	int idx[3];
	ESI   port[2];
	ESIFC fcexp;
	int fourscore = false, microphone = false;

	g_config->getOption("SDL.FourScore", &fourscore);

	microphone = port2Mic->isChecked();

	idx[0] = nesPortComboxBox[0]->currentIndex();
	idx[1] = nesPortComboxBox[1]->currentIndex();
	idx[2] = expPortComboxBox->currentIndex();

	port[0] = (ESI)nesPortComboxBox[0]->itemData( idx[0] ).toInt();
	port[1] = (ESI)nesPortComboxBox[1]->itemData( idx[1] ).toInt();
	fcexp   = (ESIFC)expPortComboxBox->itemData( idx[2] ).toInt();

	FCEUD_SetInput( fourscore, microphone, port[0], port[1], fcexp );
}
//----------------------------------------------------------------------------
void InputConfDialog_t::updatePortLabels(void)
{

	for (int i=0; i<2; i++)
	{
		getInputSelection( i, &curNesInput[i], &usrNesInput[i] );

		for (int j=0; j<nesPortComboxBox[i]->count(); j++)
		{
			if ( nesPortComboxBox[i]->itemData(j).toInt() == curNesInput[i] )
			{
				nesPortLabel[i]->setText( nesPortComboxBox[i]->itemText(j) );
			}
		}

		nesPortConfButton[i]->setEnabled( curNesInput[i] == SI_GAMEPAD );
	}

	getInputSelection( 2, &curNesInput[2], &usrNesInput[2] );

	for (int j=0; j<expPortComboxBox->count(); j++)
	{
		if ( expPortComboxBox->itemData(j).toInt() == curNesInput[2] )
		{
			expPortLabel->setText( expPortComboxBox->itemText(j) );
		}
	}
}
//----------------------------------------------------------------------------
void InputConfDialog_t::updatePortComboBoxes(void)
{
	for (int i=0; i<2; i++)
	{
		getInputSelection( i, &curNesInput[i], &usrNesInput[i] );

		for (int j=0; j<nesPortComboxBox[i]->count(); j++)
		{
			if ( nesPortComboxBox[i]->itemData(j).toInt() == curNesInput[i] )
			{
				nesPortComboxBox[i]->setCurrentIndex( j );
			}
		}
	}

	getInputSelection( 2, &curNesInput[2], &usrNesInput[2] );

	for (int j=0; j<expPortComboxBox->count(); j++)
	{
		if ( expPortComboxBox->itemData(j).toInt() == curNesInput[2] )
		{
			expPortComboxBox->setCurrentIndex( j );
		}
	}
}
//----------------------------------------------------------------------------
void InputConfDialog_t::port1Select(int index)
{
	//printf("Port 1 Number:%i \n", index);
	setInputs();
	updatePortLabels();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::port2Select(int index)
{
	//printf("Port 2 Number:%i \n", index);
	setInputs();
	updatePortLabels();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::expSelect(int index)
{
	//printf("Expansion Port Number:%i \n", index);
	setInputs();
	updatePortLabels();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::fourScoreChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	printf("Set 'SDL.FourScore' = %i\n", value);
	g_config->setOption("SDL.FourScore", value);

	setInputs();
	updatePortLabels();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::port2MicChanged(int state)
{
	setInputs();
	updatePortLabels();
}
//----------------------------------------------------------------------------
void InputConfDialog_t::autoPresetChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;
	//printf("Set 'SDL.AutoInputPreset' = %i\n", value);
	g_config->setOption("SDL.AutoInputPreset", value);
}
//----------------------------------------------------------------------------
void InputConfDialog_t::openPortConfig(int portNum)
{
	updatePortLabels();

	switch ( curNesInput[portNum] )
	{
		default:
		case SI_NONE:
		case SI_ZAPPER:
			// Do Nothing
		break;
		case SI_GAMEPAD:
			consoleWindow->openGamePadConfWin();
		break;
	}
}
//----------------------------------------------------------------------------
void InputConfDialog_t::port1Configure(void)
{
	openPortConfig(0);
}
//----------------------------------------------------------------------------
void InputConfDialog_t::port2Configure(void)
{
	openPortConfig(1);
}
//----------------------------------------------------------------------------
void InputConfDialog_t::openLoadPresetFile(void)
{
   int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	std::string path;
	const char *baseDir;
	QFileDialog  dialog(this, tr("Load Preset From File") );
	QDir dir;

	baseDir = FCEUI_GetBaseDirectory();

	path = std::string(baseDir) + "/input/presets/";

	dir.mkpath( QString::fromStdString(path) );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("Preset File (*.pre *.PRE) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	dialog.setDirectory( tr(path.c_str()) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	fceuWrapperLock();
	loadInputSettingsFromFile( filename.toStdString().c_str() );
	fceuWrapperUnLock();

	updatePortLabels();
	updatePortComboBoxes();
}

//----------------------------------------------------------------------------
void InputConfDialog_t::openSavePresetFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string path;
	const char *baseDir, *romFile;
	QFileDialog  dialog(this, tr("Save Preset to File") );
	QDir dir;

	baseDir = FCEUI_GetBaseDirectory();

	path = std::string(baseDir) + "/input/presets/";

	dir.mkpath( QString::fromStdString(path) );

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("Preset Files (*.pre *.PRE) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Save") );
	dialog.setDefaultSuffix( tr(".pre") );

	romFile = getRomFile();

	if ( romFile != NULL )
	{
		char dirStr[256], base[256];

		parseFilepath( romFile, dirStr, base );

		strcat( base, ".pre");

		dialog.selectFile( tr(base) );
	}

	dialog.setDirectory( tr(path.c_str()) );

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
	qDebug() << "selected file path : " << filename.toUtf8();

	saveInputSettingsToFile( filename.toStdString().c_str() );
}
//----------------------------------------------------------------------------
void InputConfDialog_t::updatePeriodic(void)
{
	bool updateNeeded = false;
	int tmpCurInputType[3], tmpUsrInputType[3];
	int fourScoreValue;

	for (int i=0; i<3; i++)
	{
		getInputSelection( i, &tmpCurInputType[i], &tmpUsrInputType[i] );

		if ( curNesInput[i] != tmpCurInputType[i] )
		{
			updateNeeded = true;
		}
		if ( usrNesInput[i] != tmpUsrInputType[i] )
		{
			updateNeeded = true;
		}
	}

	if ( updateNeeded )
	{
		updatePortLabels();
		updatePortComboBoxes();
	}

	g_config->getOption("SDL.FourScore", &fourScoreValue);

	if ( fourScoreValue != fourScoreEna->isChecked() )
	{
		fourScoreEna->setChecked( fourScoreValue );
	}
}
//----------------------------------------------------------------------------
