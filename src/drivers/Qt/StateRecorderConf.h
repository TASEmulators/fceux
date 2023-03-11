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
	QCheckBox   *recorderEnable;
	QLineEdit   *numSnapsLbl;
	QLineEdit   *snapMemSizeLbl;
	QLineEdit   *totalMemUsageLbl;
	QPushButton *applyButton;
	QPushButton *closeButton;

	void recalcMemoryUsage(void);

public slots:
	void closeWindow(void);
private slots:

};
