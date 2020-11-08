// ConsoleUtilities.h

#pragma once

#include <QValidator>

int  getDirFromFile( const char *path, char *dir );

const char *getRomFile( void );

int getFileBaseName( const char *filepath, char *base, char *suffix = NULL );

int parseFilepath( const char *filepath, char *dir, char *base, char *suffix = NULL );


class fceuDecIntValidtor : public QValidator
{ 
   public:
   	fceuDecIntValidtor( int min, int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( int min, int max );
	private:
		int  min;
		int  max;
};

class fceuHexIntValidtor : public QValidator
{ 
   public:
   	fceuHexIntValidtor( int min, int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( int min, int max );
	private:
		int  min;
		int  max;
};

