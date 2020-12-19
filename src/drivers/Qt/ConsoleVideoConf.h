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

		QComboBox   *driverSelect;
		QComboBox   *scalerSelect;
		QComboBox   *regionSelect;
		QCheckBox   *gl_LF_chkBox;
		QCheckBox   *new_PPU_ena;
		QCheckBox   *frmskipcbx;
		QCheckBox   *sprtLimCbx;
		QCheckBox   *clipSidesCbx;
		QCheckBox   *showFPS_cbx;
		QCheckBox   *autoScaleCbx;
		QCheckBox   *sqrPixCbx;
		QDoubleSpinBox *xScaleBox;
		QDoubleSpinBox *yScaleBox;
		QLabel         *xScaleLabel;
		QLabel         *yScaleLabel;

		void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
		//void  setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property );

		void  resetVideo(void);
		QSize calcNewScreenSize(void);

	public slots:
      void closeWindow(void);

	private slots:
		void  openGL_linearFilterChanged( int value );
		void  sqrPixChanged( int value );
		void  use_new_PPU_changed( int value );
		void  frameskip_changed( int value );
		void  useSpriteLimitChanged( int value );
		void  clipSidesChanged( int value );
		void  showFPSChanged( int value );
		void  regionChanged(int index);
		void  driverChanged(int index);
		void  scalerChanged(int index);
		void  applyChanges( void );

};

#endif
