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
		QCheckBox   *autoRegion;
		QCheckBox   *gl_LF_chkBox;
		QCheckBox   *new_PPU_ena;
		QCheckBox   *frmskipcbx;
		QCheckBox   *sprtLimCbx;
		QCheckBox   *clipSidesCbx;
		QCheckBox   *showFPS_cbx;
		QCheckBox   *autoScaleCbx;
		QCheckBox   *aspectCbx;
		QCheckBox   *cursorVisCbx;
		QCheckBox   *drawInputAidsCbx;
		QDoubleSpinBox *xScaleBox;
		QDoubleSpinBox *yScaleBox;
		QLabel         *aspectSelectLabel;
		QLabel         *xScaleLabel;
		QLabel         *yScaleLabel;
		QLineEdit      *ntsc_start;
		QLineEdit      *ntsc_end;
		QLineEdit      *pal_start;
		QLineEdit      *pal_end;
		QLineEdit      *winSizeReadout;
		QLineEdit      *vpSizeReadout;

		void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
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
		void  use_new_PPU_changed( int value );
		void  frameskip_changed( int value );
		void  useSpriteLimitChanged( int value );
		void  clipSidesChanged( int value );
		void  showFPSChanged( int value );
		void  aspectChanged(int index);
		void  regionChanged(int index);
		void  driverChanged(int index);
		void  scalerChanged(int index);
		void  cursorShapeChanged(int index);
		void  cursorVisChanged(int value);
		void  drawInputAidsChanged(int value);
		void  applyChanges( void );
		void  ntscStartScanLineChanged(const QString &);
		void  ntscEndScanLineChanged(const QString &);
		void  palStartScanLineChanged(const QString &);
		void  palEndScanLineChanged(const QString &);

};

#endif
