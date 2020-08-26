// LuaControl.cpp
//
#include <list>

#include <QTextEdit>
#include <QFileDialog>

#include "../../fceu.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include "Qt/LuaControl.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"

static bool luaScriptRunning = false;

static std::string luaOutputText;
static std::list <LuaControlDialog_t*> winList;
//----------------------------------------------------
LuaControlDialog_t::LuaControlDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QLabel *lbl;
	std::string filename;

	resize( 512, 512 );

   setWindowTitle( tr("Lua Script Control") );

	mainLayout = new QVBoxLayout();

	lbl = new QLabel( tr("Script File:") );

	scriptPath = new QLineEdit();
	scriptArgs = new QLineEdit();

	g_config->getOption ("SDL.LastLoadLua", &filename );

	scriptPath->setText( filename.c_str() );

	luaOutput = new QTextEdit();
	luaOutput->setReadOnly(true);

	hbox = new QHBoxLayout();

	browseButton = new QPushButton( tr("Browse") );
	stopButton   = new QPushButton( tr("Stop") );

	if ( luaScriptRunning )
	{
		startButton  = new QPushButton( tr("Restart") );
	}
	else
	{
		startButton  = new QPushButton( tr("Start") );
	}

	stopButton->setEnabled( luaScriptRunning );

	connect(browseButton , SIGNAL(clicked()), this, SLOT(openLuaScriptFile(void)) );
	connect(stopButton   , SIGNAL(clicked()), this, SLOT(stopLuaScript(void)) );
	connect(startButton  , SIGNAL(clicked()), this, SLOT(startLuaScript(void)) );
	
	hbox->addWidget( browseButton );
	hbox->addWidget( stopButton   );
	hbox->addWidget( startButton  );

	mainLayout->addWidget( lbl );
	mainLayout->addWidget( scriptPath );
	mainLayout->addLayout( hbox );

	hbox = new QHBoxLayout();
	lbl = new QLabel( tr("Arguments:") );

	hbox->addWidget( lbl );
	hbox->addWidget( scriptArgs );

	mainLayout->addLayout( hbox );

	lbl = new QLabel( tr("Output Console:") );
	mainLayout->addWidget( lbl );
	mainLayout->addWidget( luaOutput );

	//connect(useNativeFileDialog , SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)) );

	setLayout( mainLayout );

	winList.push_back( this );
}

//----------------------------------------------------
LuaControlDialog_t::~LuaControlDialog_t(void)
{
	std::list <LuaControlDialog_t*>::iterator it;
	  
	printf("Destroy Lua Control Window\n");

	for (it = winList.begin(); it != winList.end(); it++)
	{
		if ( (*it) == this )
		{
			winList.erase(it);
			//printf("Removing Lua Window\n");
			break;
		}
	}
}
//----------------------------------------------------
void LuaControlDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Lua Control Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------
void LuaControlDialog_t::closeWindow(void)
{
   //printf("Lua Control Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------
void LuaControlDialog_t::openLuaScriptFile(void)
{
#ifdef _S9XLUA_H
   int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[512];
	QFileDialog  dialog(this, tr("Open LUA Script") );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("LUA Scripts (*.lua *.LUA) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Load") );

	g_config->getOption ("SDL.LastLoadLua", &last );

   if ( last.size() == 0 )
   {
      last.assign( "/usr/share/fceux/luaScripts" );
   }

	getDirFromFile( last.c_str(), dir );

	dialog.setDirectory( tr(dir) );

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

	g_config->setOption ("SDL.LastLoadLua", filename.toStdString().c_str() );

	scriptPath->setText( filename.toStdString().c_str() );

#endif
}
//----------------------------------------------------
void LuaControlDialog_t::startLuaScript(void)
{
#ifdef _S9XLUA_H
	luaOutputText.clear();
	fceuWrapperLock();
	if ( 0 == FCEU_LoadLuaCode( scriptPath->text().toStdString().c_str(), scriptArgs->text().toStdString().c_str() ) )
   {
      printf("Error: Could not open the selected lua script: '%s'\n", scriptPath->text().toStdString().c_str() );
   }
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------
void LuaControlDialog_t::stopLuaScript(void)
{
#ifdef _S9XLUA_H
	fceuWrapperLock();
	FCEU_LuaStop();
	fceuWrapperUnLock();
#endif
}
//----------------------------------------------------
void LuaControlDialog_t::refreshState(void)
{
	if ( luaScriptRunning )
	{
		stopButton->setEnabled( true );
		startButton->setText( tr("Restart") );
	}
	else
	{
		stopButton->setEnabled( false );
		startButton->setText( tr("Start") );
	}
	luaOutput->setText( luaOutputText.c_str() );
}
//----------------------------------------------------
void updateLuaWindows( void )
{
	std::list <LuaControlDialog_t*>::iterator it;
	  
	for (it = winList.begin(); it != winList.end(); it++)
	{
		(*it)->refreshState();
	}
}
//----------------------------------------------------
void WinLuaOnStart(intptr_t hDlgAsInt)
{
	luaScriptRunning = true;

	//printf("Lua Script Running: %i \n", luaScriptRunning );

	updateLuaWindows();
}
//----------------------------------------------------
void WinLuaOnStop(intptr_t hDlgAsInt)
{
	luaScriptRunning = false;

	//printf("Lua Script Running: %i \n", luaScriptRunning );

	updateLuaWindows();
}
//----------------------------------------------------
void PrintToWindowConsole(intptr_t hDlgAsInt, const char* str)
{
	//printf("%s\n", str );

	luaOutputText.append( str );
	
	updateLuaWindows();
}
//----------------------------------------------------
