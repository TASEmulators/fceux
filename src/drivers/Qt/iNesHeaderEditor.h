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

class iNES_HEADER;

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
		QCheckBox    *battNvRamBox;
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
		QPushButton  *restoreBtn;
		QPushButton  *saveAsBtn;
		QPushButton  *closeBtn;
		QGroupBox    *iNesUnOfGroupBox;
		QGroupBox    *sysGroupBox;
		QGroupBox    *vsGroupBox;
		QGroupBox    *extGroupBox;
		QLabel       *prgRamLbl;
		QLabel       *prgNvRamLbl;
		QLabel       *mapperSubLbl;
		QLabel       *chrRamLbl;
		QLabel       *chrNvRamLbl;
		QLabel       *inputDevLbl;
		QLabel       *miscRomsLbl;
		iNES_HEADER  *iNesHdr;
	private:

		bool loadHeader(iNES_HEADER *header);
		void setHeaderData(iNES_HEADER *header);
		void showErrorMsgWindow(const char *str);
		void ToggleINES20(bool ines20);
		void ToggleUnofficialPropertiesEnabled(bool ines20, bool check);
		void ToggleUnofficialExtraRegionCode(bool ines20, bool unofficial_check, bool check);
		void ToggleUnofficialPrgRamPresent(bool ines20, bool unofficial_check, bool check);

   public slots:
      void closeWindow(void);
	private slots:
		void iNes1Clicked(bool checked);
		void iNes2Clicked(bool checked);

};
