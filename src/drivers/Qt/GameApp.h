
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

#include "GameViewer.h"

class  gameWin_t : public QMainWindow
{
	Q_OBJECT

	public:
		gameWin_t(QWidget *parent = 0);
	   ~gameWin_t(void);

		gameView_t *viewport;
		//QPushButton hello( "Hello world!", 0 );
		//

	 QMenu *fileMenu;
    QMenu *helpMenu;

    QAction *openAct;
    QAction *quitAct;
    QAction *aboutAct;

	 QTimer  *gameTimer;

	protected:
	 void keyPressEvent(QKeyEvent *event);
	 void keyReleaseEvent(QKeyEvent *event);

	private:
		void createMainMenu(void);

	private slots:
		void openFile(void);
      void aboutQPlot(void);
      void runGameFrame(void);

};

#endif
