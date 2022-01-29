// ConsoleVideoConf.h
//

#ifndef __ConsoleVideoH__
#define __ConsoleVideoH__

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QSlider>
#include <QFrame>
#include <QGroupBox>
#include <QDoubleSpinBox>

class ConsoleVideoConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ConsoleVideoConfDialog_t(QWidget *parent = 0);
		~ConsoleVideoConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *bar);

		QTimer      *updateTimer;
		QComboBox   *driverSelect;
		QComboBox   *scalerSelect;
		QComboBox   *regionSelect;
		QComboBox   *cursorSelect;
		QComboBox   *aspectSelect;
		QComboBox   *inputDisplaySel;
		QComboBox   *videoTest;
		QCheckBox   *autoRegion;
		QCheckBox   *vsync_ena;
		QCheckBox   *gl_LF_chkBox;
		QCheckBox   *new_PPU_ena;
		QCheckBox   *frmskipcbx;
		QCheckBox   *sprtLimCbx;
		QCheckBox   *clipSidesCbx;
		QCheckBox   *showFPS_cbx;
		QCheckBox   *showGuiMsgs_cbx;
		QCheckBox   *autoScaleCbx;
		QCheckBox   *aspectCbx;
		QCheckBox   *cursorVisCbx;
		QCheckBox   *drawInputAidsCbx;
		QCheckBox   *intFrameRateCbx;
		QCheckBox   *showFrameCount_cbx;
		QCheckBox   *showLagCount_cbx;
		QCheckBox   *showRerecordCount_cbx;
		QDoubleSpinBox *xScaleBox;
		QDoubleSpinBox *yScaleBox;
		QLabel         *aspectSelectLabel;
		QLabel         *xScaleLabel;
		QLabel         *yScaleLabel;
		QSpinBox       *ntsc_start;
		QSpinBox       *ntsc_end;
		QSpinBox       *pal_start;
		QSpinBox       *pal_end;
		QLineEdit      *winSizeReadout;
		QLineEdit      *vpSizeReadout;
		QLineEdit      *scrRateReadout;

		void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
		void  setComboBoxFromValue( QComboBox *cbx, int pval );
		//void  setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property );

		void  resetVideo(void);
		void  updateReadouts(void);
		QSize calcNewScreenSize(void);

	public slots:
		void closeWindow(void);

	private slots:
		void  periodicUpdate(void);
		void  autoRegionChanged( int value );
		void  openGL_linearFilterChanged( int value );
		void  autoScaleChanged( int value );
		void  aspectEnableChanged( int value );
		void  use_new_PPU_changed( bool value );
		void  frameskip_changed( int value );
		void  vsync_changed( int value );
		void  intFrameRate_changed( int value );
		void  useSpriteLimitChanged( int value );
		void  clipSidesChanged( int value );
		void  showGuiMsgsChanged( int value );
		void  showFPSChanged( int value );
		void  aspectChanged(int index);
		void  regionChanged(int index);
		void  driverChanged(int index);
		void  scalerChanged(int index);
		void  testPatternChanged(int index);
		void  cursorShapeChanged(int index);
		void  cursorVisChanged(int value);
		void  drawInputAidsChanged(int value);
		void  showFrameCountChanged(int value);
		void  showLagCountChanged( int value );
		void  showRerecordCountChanged( int value );
		void  inputDisplayChanged(int index);
		void  applyChanges( void );
		void  ntscStartScanLineChanged(int value);
		void  ntscEndScanLineChanged(int value);
		void  palStartScanLineChanged(int value);
		void  palEndScanLineChanged(int value);

};

#endif
