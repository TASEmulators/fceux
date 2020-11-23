// InputConf.h
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
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"

class InputConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		InputConfDialog_t(QWidget *parent = 0);
		~InputConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);
		//void keyPressEvent(QKeyEvent *event);
   	//void keyReleaseEvent(QKeyEvent *event);

		QCheckBox   *fourScoreEna;
		QCheckBox   *port2Mic;
		QLabel      *nesPortLabel[2];
		QPushButton *nesPortConfButton[2];
		QComboBox   *nesPortComboxBox[2];
		QLabel      *expPortLabel;
		QPushButton *expPortConfButton;
		QComboBox   *expPortComboxBox;

		int          curNesInput[3];
		int          usrNesInput[3];

	private:
		void  setInputs(void);
		void  updatePortLabels(void);
		void  openPortConfig(int portNum);

   public slots:
      void closeWindow(void);
	private slots:
		void port1Configure(void);
		void port2Configure(void);
		void port1Select(int index);
		void port2Select(int index);
		void expSelect(int index);

};

void openInputConfWindow( QWidget *parent );
