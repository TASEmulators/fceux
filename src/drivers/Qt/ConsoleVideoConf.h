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
		QCheckBox   *gl_LF_chkBox;
		QComboBox   *drvSel;
		QComboBox   *regionSelect;

		//void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		//void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
		//void  setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property );

	private slots:

};

#endif
