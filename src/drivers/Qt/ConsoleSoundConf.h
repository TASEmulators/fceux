// GameSoundConf.h
//

#ifndef __GameSndH__
#define __GameSndH__

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

class ConsoleSndConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ConsoleSndConfDialog_t(QWidget *parent = 0);
		~ConsoleSndConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QCheckBox   *enaChkbox;
		QCheckBox   *enaLowPass;
		QCheckBox   *swapDutyChkbox;
		QComboBox   *qualitySelect;
		QComboBox   *rateSelect;
		QSlider     *bufSizeSlider;
		QLabel      *bufSizeLabel;
		QLabel      *volLbl;
		QLabel      *triLbl;
		QLabel      *sqr1Lbl;
		QLabel      *sqr2Lbl;
		QLabel      *nseLbl;
		QLabel      *pcmLbl;

		void  setCheckBoxFromProperty( QCheckBox *cbx, const char *property );
		void  setComboBoxFromProperty( QComboBox *cbx, const char *property );
		void  setSliderFromProperty( QSlider *slider, QLabel *lbl, const char *property );

	private slots:
		void  closeWindow(void);
		void  bufSizeChanged(int value);
		void  volumeChanged(int value);
		void  triangleChanged(int value);
		void  square1Changed(int value);
		void  square2Changed(int value);
		void  noiseChanged(int value);
		void  pcmChanged(int value);
		void  enaSoundStateChange(int value);
		void  enaSoundLowPassChange(int value);
		void  swapDutyCallback(int value);
		void  soundQualityChanged(int index);
		void  soundRateChanged(int index);

};

#endif
