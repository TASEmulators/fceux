// ConsoleUtilities.h

#pragma once

#include <QColor>
#include <QTimer>
#include <QValidator>
#include <QDialog>
#include <QHelpEvent>

int  getDirFromFile( const char *path, char *dir );

const char *getRomFile( void );

int getFileBaseName( const char *filepath, char *base, char *suffix = NULL );

int parseFilepath( const char *filepath, char *dir, char *base, char *suffix = NULL );

int fceuExecutablePath( char *outputPath, int outputSize );

int fceuLoadConfigColor( const char *confName, QColor *color );

class fceuDecIntValidtor : public QValidator
{ 
   public:
   	fceuDecIntValidtor( long long int min, long long int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( long long int min, long long int max );
	private:
		long long int  min;
		long long int  max;
};

class fceuHexIntValidtor : public QValidator
{ 
	public:
		fceuHexIntValidtor( long long int min, long long int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( long long int min, long long int max );
	private:
		long long int  min;
		long long int  max;
};

class fceuCustomToolTip : public QDialog
{
	public:
		fceuCustomToolTip( QWidget *parent = nullptr );
		~fceuCustomToolTip( void );

		void setHideOnMouseMove(bool);
	protected:
		bool eventFilter(QObject *obj, QEvent *event) override;
		void mouseMoveEvent( QMouseEvent *event ) override;

		void hideTip(void);
		void hideTipImmediately(void);
	private:
		QWidget *w;
		QTimer  *hideTimer;
		bool     hideOnMouseMode;

		static fceuCustomToolTip *instance;

	private slots:
		void  hideTimerExpired(void);
};

QString fceuGetOpcodeToolTip( uint8_t *opcode, int size );

QDialog *fceuCustomToolTipShow( QHelpEvent *helpEvent, QDialog *popup );
