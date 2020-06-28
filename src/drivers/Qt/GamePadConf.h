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
#include <QGroupBox>

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

class GamePadConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GamePadConfDialog_t(QWidget *parent = 0);
		~GamePadConfDialog_t(void);

	protected:
		QComboBox *portSel;
      QLabel      *keyName[GAMEPAD_NUM_BUTTONS];
      GamePadConfigButton_t *button[GAMEPAD_NUM_BUTTONS];

      int  portNum;
		int  configNo;
      int  buttonConfigStatus;

      void changeButton( int port, int button );
      void clearButton( int port, int button );
      void keyPressEvent(QKeyEvent *event);
	   void keyReleaseEvent(QKeyEvent *event);
      void closeEvent(QCloseEvent *bar);
	private:
		void updateCntrlrDpy(void);

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
      void loadDefaults(void);
		void ena4score(int state);
		void oppDirEna(int state);
		void controllerSelect(int index);

};
