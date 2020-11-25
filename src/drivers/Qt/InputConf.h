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
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QTimer>

#include "Qt/main.h"

class InputConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		InputConfDialog_t(QWidget *parent = 0);
		~InputConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QTimer      *inputTimer;
		QCheckBox   *fourScoreEna;
		QCheckBox   *port2Mic;
		QCheckBox   *autoPreset;
		QLabel      *nesPortLabel[2];
		QPushButton *nesPortConfButton[2];
		QComboBox   *nesPortComboxBox[2];
		QLabel      *expPortLabel;
		QPushButton *expPortConfButton;
		QComboBox   *expPortComboxBox;
		QPushButton *loadConfigButton;
		QPushButton *saveConfigButton;

		int          curNesInput[3];
		int          usrNesInput[3];

	private:
		void  setInputs(void);
		void  updatePortLabels(void);
		void  updatePortComboBoxes(void);
		void  openPortConfig(int portNum);

   public slots:
      void closeWindow(void);
	private slots:
		void port1Configure(void);
		void port2Configure(void);
		void port1Select(int index);
		void port2Select(int index);
		void expSelect(int index);
		void fourScoreChanged(int state);
		void port2MicChanged(int state);
		void autoPresetChanged(int state);
		void openLoadPresetFile(void);
		void openSavePresetFile(void);
		void updatePeriodic(void);

};

void openInputConfWindow( QWidget *parent );
