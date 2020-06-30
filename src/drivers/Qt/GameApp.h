
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

#include "Qt/GameViewerGL.h"
#include "Qt/GameViewerSDL.h"
#include "Qt/GamePadConf.h"

class  gameWin_t : public QMainWindow
{
	Q_OBJECT

	public:
		gameWin_t(QWidget *parent = 0);
	   ~gameWin_t(void);

		gameViewGL_t *viewport;
		//gameViewSDL_t *viewport;

	protected:
	 QMenu *fileMenu;
    QMenu *optMenu;
    QMenu *helpMenu;

    QAction *openROM;
    QAction *closeROM;
    QAction *quitAct;
    QAction *gamePadConfig;
    QAction *aboutAct;

	 QTimer  *gameTimer;

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
      void runGameFrame(void);

};

#endif
