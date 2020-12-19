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
#include <QFont>

#include "Qt/main.h"

class iNES_HEADER;

class iNesHeaderEditor_t : public QDialog
{
   Q_OBJECT

	public:
		iNesHeaderEditor_t(QWidget *parent = 0);
		~iNesHeaderEditor_t(void);

		bool  isInitialized(void){ return initOK; };
	protected:
		void closeEvent(QCloseEvent *event);

		QFont         font;
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

		bool          initOK;
	private:

		bool openFile(void);
		void printHeader(iNES_HEADER* _header);
		bool loadHeader(iNES_HEADER *header);
		bool SaveINESFile(const char* path, iNES_HEADER* header);
		bool WriteHeaderData(iNES_HEADER* header);
		void setHeaderData(iNES_HEADER *header);
		void showErrorMsgWindow(const char *str);
		void ToggleINES20(bool ines20);
		void ToggleUnofficialPropertiesEnabled(bool ines20, bool check);
		void ToggleUnofficialExtraRegionCode(bool ines20, bool unofficial_check, bool check);
		void ToggleUnofficialPrgRamPresent(bool ines20, bool unofficial_check, bool check);
		void ToggleVSSystemGroup(bool enable);
		void ToggleExtendSystemList(bool enable);

   public slots:
      void closeWindow(void);
	private slots:
		void saveHeader(void);
		void saveFileAs(void);
		void restoreHeader(void);
		void iNes1Clicked(bool checked);
		void iNes2Clicked(bool checked);
		void normSysClicked(bool checked);
		void vsSysClicked(bool checked);
		void plySysClicked(bool checked);
		void extSysClicked(bool checked);
		void unofficialStateChange(int state);
		void unofficialPrgRamStateChange(int state);
		void unofficialDualRegionStateChange(int state);

};
