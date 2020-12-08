// MsgLogViewer.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/MsgLogViewer.h"
#include "Qt/ConsoleWindow.h"

#define  MSG_LOG_MAX_LINES  256

class msgLogBuf_t
{
	public:
	msgLogBuf_t(void)
	{
		char filename[256];

		strcpy( filename, "/tmp/fceux.log" );

		fp = ::fopen( filename, "w+");

		if ( fp == NULL )
		{
			printf("Error: Failed to open message log file: '%s'\n", filename);
		}
		maxLines   = MSG_LOG_MAX_LINES;
		totalLines = 0;
		head = tail = 0;

		for (int i=0; i<MSG_LOG_MAX_LINES; i++)
		{
			fpOfsList[i] = 0;
		}
	}

	~msgLogBuf_t(void)
	{
		if ( fp != NULL )
		{
			::fclose(fp); fp = NULL;
		}
	}

	void clear(void)
	{
		head = tail = 0;
	}

	void addLine( const char *txt, bool NewLine = false )
	{
		long ofs;

		if ( fp == NULL ) return;

		::fseek( fp, 0L, SEEK_END);

		ofs = ::ftell(fp);

		if ( NewLine )
		{
			::fprintf( fp, "%s\n", txt );
		}
		else
		{
			::fprintf( fp, "%s", txt );
		}

		fpOfsList[head] = ofs;

		head = (head + 1) % MSG_LOG_MAX_LINES;

		if ( head == tail )
		{
			tail = (tail + 1) % MSG_LOG_MAX_LINES;
		}

		totalLines++;
	}

	size_t getTotalLineCount(void)
	{
		return totalLines;
	}

	size_t size(void)
	{
		size_t s;

		s = head - tail;

		if ( s < 0 )
		{
			s += MSG_LOG_MAX_LINES;
		}
		return s;
	}

	void loadTextViewer( QPlainTextEdit *viewer )
	{
		long ofs, nbytes;

		if ( fp == NULL )
		{
			return;
		}

		if ( head == tail )
		{
			return;
		}
		char buf[65536];

		ofs = fpOfsList[tail];

		::fseek( fp, ofs, SEEK_SET);

		//printf("Seek: %li \n", ofs );

		if ( (nbytes = ::fread( buf, 1, sizeof(buf), fp )) > 0 )
		{
			//printf("READ: %li \n", nbytes );
			buf[ nbytes ] = 0;
			viewer->setPlainText( buf );
		}
	}

	private:
		FILE   *fp;
		size_t  maxLines;
		size_t  totalLines;
		size_t  head;
		size_t  tail;

		long    fpOfsList[MSG_LOG_MAX_LINES];
};

static msgLogBuf_t msgLog;
//----------------------------------------------------------------------------
MsgLogViewDialog_t::MsgLogViewDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *clearBtn, *closeBtn;

	setWindowTitle("Message Log Viewer");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();

	txtView = new QPlainTextEdit();
	txtView->setReadOnly(true);

	mainLayout->addWidget( txtView );

	hbox     = new QHBoxLayout();
	clearBtn = new QPushButton( tr("Clear") );
	closeBtn = new QPushButton( tr("Close") );
	hbox->addWidget( clearBtn );
	hbox->addWidget( closeBtn );

   connect( clearBtn, SIGNAL(clicked(void)), this, SLOT(clearLog(void))    );
   connect( closeBtn, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)) );

	mainLayout->addLayout( hbox );

	setLayout( mainLayout );

	totalLines = 0;

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &MsgLogViewDialog_t::updatePeriodic );

	updateTimer->start( 500 ); // 2hz

	msgLog.loadTextViewer( txtView );

	totalLines = msgLog.getTotalLineCount();

	txtView->moveCursor(QTextCursor::End);
}
//----------------------------------------------------------------------------
MsgLogViewDialog_t::~MsgLogViewDialog_t(void)
{
	printf("Destroy Msg Log Key Config Window\n");
	updateTimer->stop();
}
//----------------------------------------------------------------------------
void MsgLogViewDialog_t::closeEvent(QCloseEvent *event)
{
   printf("Msg Log Key Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void MsgLogViewDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void MsgLogViewDialog_t::clearLog(void)
{
	fceuWrapperLock();

	msgLog.clear();

	txtView->clear();

	fceuWrapperUnLock();
}
//----------------------------------------------------------------------------
void MsgLogViewDialog_t::updatePeriodic(void)
{
	if ( msgLog.getTotalLineCount() != totalLines )
	{
		fceuWrapperLock();

		msgLog.loadTextViewer( txtView );

		totalLines = msgLog.getTotalLineCount();

		fceuWrapperUnLock();

		txtView->moveCursor(QTextCursor::End);
	}
}
//----------------------------------------------------------------------------
/**
* Prints a textual message without adding a newline at the end.
*
* @param text The text of the message.
*
* TODO: This function should have a better name.
**/
void FCEUD_Message(const char *text)
{
	fputs(text, stdout);
	//fprintf(stdout, "\n");
	//
	msgLog.addLine( text, false );
}
//----------------------------------------------------------------------------
/**
* Shows an error message in a message box.
* (For now: prints to stderr.)
* 
* If running in Qt mode, display a dialog message box of the error.
*
* @param errormsg Text of the error message.
**/
void FCEUD_PrintError(const char *errormsg)
{
	fprintf(stderr, "%s\n", errormsg);

	msgLog.addLine( errormsg, true );

	if ( consoleWindow )
	{
		consoleWindow->QueueErrorMsgWindow( errormsg );
	}
}
//----------------------------------------------------------------------------
