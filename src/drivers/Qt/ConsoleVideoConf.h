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

#include "Qt/TextOverlayDisplay.h"

class ConsoleVideoConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ConsoleVideoConfDialog_t(QWidget *parent = 0);
		~ConsoleVideoConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *bar) override;

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
		QCheckBox   *textOverlayCbx[TextOverlayDisplay_t::ITEM_COUNT];
		QComboBox   *textOverlayRateSel;
		#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
		QCheckBox   *winFullScreenBorderCbx;
		#endif
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

		void  setupToolTips(void);
		void  resetVideo(void);
		void  updateReadouts(void);
		QSize calcNewScreenSize(void);

	public slots:
		void closeWindow(void);

	private slots:
		void  periodicUpdate(void);
		void  autoRegionChanged( bool value );
		void  openGL_linearFilterChanged( bool value );
		void  autoScaleChanged( bool value );
		void  aspectEnableChanged( bool value );
		void  use_new_PPU_changed( bool value );
		void  frameskip_changed( bool value );
		void  vsync_changed( bool value );
		void  intFrameRate_changed( bool value );
		void  useSpriteLimitChanged( bool value );
		void  clipSidesChanged( bool value );
		void  showGuiMsgsChanged( bool value );
		void  showFPSChanged( bool value );
		void  aspectChanged(int index);
		void  regionChanged(int index);
		void  driverChanged(int index);
		void  scalerChanged(int index);
		void  testPatternChanged(int index);
		void  cursorShapeChanged(int index);
		void  cursorVisChanged(bool value);
		void  drawInputAidsChanged(bool value);
		void  showFrameCountChanged(bool value);
		void  showLagCountChanged( bool value );
		void  showRerecordCountChanged( bool value );
		void  textOverlayItemChanged( int item, bool checked );
		void  textOverlayRateChanged( int index );
		void  updateTextOverlayControls(void);
		void  inputDisplayChanged(int index);
		void  applyChanges( void );
		void  ntscStartScanLineChanged(int value);
		void  ntscEndScanLineChanged(int value);
		void  palStartScanLineChanged(int value);
		void  palEndScanLineChanged(int value);
		#if defined(WIN32) && (QT_VERSION_MAJOR < 6)
		void  winFullScreenBorderChanged(bool value);
		#endif

};

#endif
