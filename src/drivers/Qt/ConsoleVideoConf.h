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

class ConsoleVideoConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ConsoleVideoConfDialog_t(QWidget *parent = 0);
		~ConsoleVideoConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *bar);

		QComboBox   *driverSelect;
		QComboBox   *regionSelect;
		QCheckBox   *gl_LF_chkBox;
		QCheckBox   *new_PPU_ena;
		QCheckBox   *frmskipcbx;
		QCheckBox   *sprtLimCbx;
		QCheckBox   *clipSidesCbx;
		QCheckBox   *showFPS_cbx;

		void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
		//void  setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property );

		void  resetVideo(void);

	public slots:
      void closeWindow(void);

	private slots:
		void  use_new_PPU_changed( int value );
		void  frameskip_changed( int value );
		void  useSpriteLimitChanged( int value );
		void  clipSidesChanged( int value );
		void  showFPSChanged( int value );
		void  regionChanged(int index);
		void  driverChanged(int index);
		void  applyChanges( void );

};

#endif
