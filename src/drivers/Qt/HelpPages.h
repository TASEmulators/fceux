// HelpPages.h
#pragma once

#include <QDialog>
#include <QAction>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QCloseEvent>
#include <QHelpEngine>

class HelpBrowser : public QTextBrowser
{
	public:
    		HelpBrowser(QHelpEngine* helpEngine, QWidget* parent = 0);
    		QVariant loadResource (int type, const QUrl& name);
	private:
		QHelpEngine* helpEngine;
};

class HelpDialog : public QDialog
{
	Q_OBJECT

	public:
		HelpDialog( const char *helpFileName, QWidget *parent = 0);
		~HelpDialog(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QHelpEngine  *helpEngine;
		QSplitter    *hsplitter;
		QTabWidget   *tabWgt;
		HelpBrowser  *textViewer;
		QAction      *backButton;
		QAction      *forwardButton;

	public slots:
		void closeWindow(void);
		void navBackwardAvailable(bool);
		void navForwardAvailable(bool);
};
