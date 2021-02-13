// GamePadConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QGroupBox>
#include <QPainter>

#include "Qt/main.h"

class GamePadConfigButton_t : public QPushButton
{   
	public:
 		GamePadConfigButton_t(int i);

	protected:
   	void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);

		int idx;
};

class GamePadView_t : public QWidget
{
	Q_OBJECT

	public:
		GamePadView_t( QWidget *parent = 0);
		~GamePadView_t(void);

		void setPort( int port );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		//void contextMenuEvent(QContextMenuEvent *event);

		void drawLetterOnButton( QPainter &painter, QRect &rect, QColor &color, int ch );

		int  portNum;
		int  viewWidth;
		int  viewHeight;
		int  pxCharWidth;
		int  pxCharHeight;

		QFont font;
};

class GamePadConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GamePadConfDialog_t(QWidget *parent = 0);
		~GamePadConfDialog_t(void);

	protected:
		QTimer    *inputTimer;
		QComboBox *portSel;
		QComboBox *devSel;
		QComboBox *mapSel;
		QComboBox *profSel;
		QCheckBox *efs_chkbox;
		QLabel      *guidLbl;
		QLabel      *mapMsg;
		QLabel      *keyName[GAMEPAD_NUM_BUTTONS];
		QLabel      *keyState[GAMEPAD_NUM_BUTTONS];
		GamePadConfigButton_t *button[GAMEPAD_NUM_BUTTONS];
      		GamePadView_t *gpView;

		int  portNum;
		int  buttonConfigStatus;
		int  changeSeqStatus; // status of sequentally changing buttons mechanism
		  //    0 - we can start new change process
		  // 1-10 - changing in progress
		  //   -1 - changing is aborted
		
		void changeButton( int port, int button );
		void clearButton( int port, int button );
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		void closeEvent(QCloseEvent *bar);
	private:
		void updateCntrlrDpy(void);
		void createNewProfile( const char *name );
		void loadMapList(void);
		void saveConfig(void);
		void promptToSave(void);

	public slots:
		void closeWindow(void);
	private slots:
		void changeButton0(void);
		void changeButton1(void);
		void changeButton2(void);
		void changeButton3(void);
		void changeButton4(void);
		void changeButton5(void);
		void changeButton6(void);
		void changeButton7(void);
		void changeButton8(void);
		void changeButton9(void);
		void clearButton0(void);
		void clearButton1(void);
		void clearButton2(void);
		void clearButton3(void);
		void clearButton4(void);
		void clearButton5(void);
		void clearButton6(void);
		void clearButton7(void);
		void clearButton8(void);
		void clearButton9(void);
		void clearAllCallback(void);
		void ena4score(int state);
		void oppDirEna(int state);
		void portSelect(int index);
		void deviceSelect(int index);
		void newProfileCallback(void);
		void loadProfileCallback(void);
		void saveProfileCallback(void);
		void deleteProfileCallback(void);
		void updatePeriodic(void);
		void changeSequentallyCallback(void);

};

int openGamePadConfWindow( QWidget *parent );

int closeGamePadConfWindow(void);
