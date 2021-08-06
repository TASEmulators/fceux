// MovieRecord.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

#include "Qt/main.h"
#include "Qt/ConsoleUtilities.h"

class MovieRecordDialog_t : public QDialog
{
	Q_OBJECT

public:
	MovieRecordDialog_t(QWidget *parent = 0);
	~MovieRecordDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit   *dirEdit;
	QLineEdit   *fileEdit;
	QLineEdit   *authorEdit;
	QPushButton *fileBrowse;
	QPushButton *okButton;
	QPushButton *cancelButton;
	QComboBox   *stateSel;
	std::string  filepath;
	std::string  ic_file;

private:
	void setFilePath( QString s );
	void setFilePath( std::string s );
	void setFilePath( const char *s );
	void setLoadState(void);

public slots:
	void closeWindow(void);
private slots:
	void browseFiles(void);
	void recordMovie(void);
	void stateSelChanged(int idx);
};
