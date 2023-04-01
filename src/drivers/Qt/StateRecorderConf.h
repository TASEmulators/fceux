// StateRecorderConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

class StateRecorderDialog_t : public QDialog
{
	Q_OBJECT

public:
	StateRecorderDialog_t(QWidget *parent = 0);
	~StateRecorderDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);

	QSpinBox    *snapMinutes;
	QSpinBox    *snapSeconds;
	QSpinBox    *historyDuration;
	QSpinBox    *pauseDuration;
	QCheckBox   *recorderEnable;
	QLineEdit   *numSnapsLbl;
	QLineEdit   *snapMemSizeLbl;
	QLineEdit   *totalMemUsageLbl;
	QLineEdit   *saveTimeLbl;
	QPushButton *applyButton;
	QPushButton *closeButton;
	QComboBox   *cmprLvlCbox;
	QComboBox   *pauseOnLoadCbox;

	double       saveTimeMs;

	void recalcMemoryUsage(void);

public slots:
	void closeWindow(void);
private slots:
	void applyChanges(void);
	void spinBoxValueChanged(int newValue);
	void enableChanged(int);
	void compressionLevelChanged(int newValue);
	void pauseOnLoadChanged(int index);
};
