// HotKeyConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

#include "Qt/main.h"

class iNesHeaderEditor_t : public QDialog
{
   Q_OBJECT

	public:
		iNesHeaderEditor_t(QWidget *parent = 0);
		~iNesHeaderEditor_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QRadioButton *iNes1Btn;
		QRadioButton *iNes2Btn;
		QComboBox    *mapperComboBox;
		QLineEdit    *mapperSubEdit;
		QLineEdit    *miscRomsEdit;
		QComboBox    *prgRomBox;
		QComboBox    *prgRamBox;
		QComboBox    *prgNvRamBox;
		QComboBox    *chrRomBox;
		QComboBox    *chrRamBox;
		QComboBox    *chrNvRamBox;
		QComboBox    *vsHwBox;
		QComboBox    *vsPpuBox;
		QComboBox    *extCslBox;
		QComboBox    *inputDevBox;
		QCheckBox    *trainerCBox;
		QCheckBox    *iNesUnOfBox;
		QCheckBox    *iNesDualRegBox;
		QCheckBox    *iNesBusCfltBox;
		QCheckBox    *iNesPrgRamBox;
		QRadioButton *horzMirrorBtn;
		QRadioButton *vertMirrorBtn;
		QRadioButton *fourMirrorBtn;
		QRadioButton *ntscRegionBtn;
		QRadioButton *palRegionBtn;
		QRadioButton *dendyRegionBtn;
		QRadioButton *dualRegionBtn;
		QRadioButton *normSysbtn;
		QRadioButton *vsSysbtn;
		QRadioButton *plySysbtn;
		QRadioButton *extSysbtn;
	private:

   public slots:
      void closeWindow(void);
	private slots:

};
