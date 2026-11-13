// MovieOptions.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QStackedWidget>

#include "Qt/main.h"

class MovieOptionsDialog_t : public QDialog
{
	Q_OBJECT

public:
	MovieOptionsDialog_t(QWidget *parent = 0);
	~MovieOptionsDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event) override;

	QCheckBox *readOnlyReplay;
	QCheckBox *pauseAfterPlay;
	QCheckBox *closeAfterPlay;
	QCheckBox *bindSaveStates;
	QCheckBox *dpySubTitles;
	QCheckBox *putSubTitlesAvi;
	QCheckBox *autoBackUp;
	QCheckBox *loadFullStates;
	QCheckBox *aviEnableHUD;
	QCheckBox *aviEnableMsg;
	QCheckBox *aviEnableAudio;
	QComboBox *aviBackend;
	QStackedWidget *aviPageStack;

private:
public slots:
	void closeWindow(void);
private slots:
	void readOnlyReplayChanged(bool state);
	void pauseAfterPlayChanged(bool state);
	void closeAfterPlayChanged(bool state);
	void bindSaveStatesChanged(bool state);
	void dpySubTitlesChanged(bool state);
	void putSubTitlesAviChanged(bool state);
	void setAviHudEnable(bool state);
	void setAviMsgEnable(bool state);
	void setAviAudioEnable(bool state);
	void autoBackUpChanged(bool state);
	void loadFullStatesChanged(bool state);
	void aviBackendChanged(int idx);
};
