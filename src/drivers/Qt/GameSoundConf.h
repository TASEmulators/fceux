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

class GameSndConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GameSndConfDialog_t(QWidget *parent = 0);
		~GameSndConfDialog_t(void);

	protected:
		QCheckBox   *enaChkbox;
		QCheckBox   *enaLowPass;
		QCheckBox   *swapDutyChkbox;
		QComboBox   *qualitySelect;
		QComboBox   *rateSelect;
		QSlider     *bufSizeSlider;

	private slots:

};

#endif
