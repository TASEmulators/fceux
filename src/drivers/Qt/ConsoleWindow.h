
//

#ifndef __GameAppH__
#define __GameAppH__

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>
#include <QThread>
#include <QMutex>

#include "Qt/ConsoleViewerGL.h"
#include "Qt/ConsoleViewerSDL.h"
#include "Qt/GamePadConf.h"

class  emulatorThread_t : public QThread
{
	Q_OBJECT

	//public slots:
		void run( void ) override;
	signals:
    void finished();
};

class  consoleWin_t : public QMainWindow
{
	Q_OBJECT

	public:
		consoleWin_t(QWidget *parent = 0);
	   ~consoleWin_t(void);

		ConsoleViewGL_t *viewport;
		//ConsoleViewSDL_t *viewport;

		void setCyclePeriodms( int ms );

		QMutex *mutex;

	protected:
	 QMenu *fileMenu;
    QMenu *optMenu;
    QMenu *helpMenu;

    QAction *openROM;
    QAction *closeROM;
    QAction *quitAct;
    QAction *gamePadConfig;
    QAction *gameSoundConfig;
    QAction *gameVideoConfig;
    QAction *hotkeyConfig;
    QAction *autoResume;
    QAction *aboutAct;

	 QTimer  *gameTimer;
	 emulatorThread_t *emulatorThread;

    GamePadConfDialog_t *gamePadConfWin;

	protected:
    void closeEvent(QCloseEvent *event);
	 void keyPressEvent(QKeyEvent *event);
	 void keyReleaseEvent(QKeyEvent *event);

	private:
		void createMainMenu(void);

	private slots:
		void closeApp(void);
		void openROMFile(void);
		void closeROMCB(void);
      void aboutQPlot(void);
      void openGamePadConfWin(void);
      void openGameSndConfWin(void);
      void openGameVideoConfWin(void);
      void openHotkeyConfWin(void);
      void toggleAutoResume(void);
      void updatePeriodic(void);

};

extern consoleWin_t *consoleWindow;

#endif
