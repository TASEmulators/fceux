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
      QLabel      *keyName[10];
      GamePadConfigButton_t *button[10];

      int  portNum;
		int  configNo;
      int  buttonConfigStatus;

      void changeButton( int port, int button );
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
		void ena4score(int state);
		void oppDirEna(int state);
		void controllerSelect(int index);

};
