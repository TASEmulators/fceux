// BoardControl.h
//
// Dialog for viewing and controlling the state of the loaded cartridge board.
// Currently hosts the DIP switches registered through boardctl.h; further
// board status/control sections can be added as additional group boxes.

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QTimer>
#include <QLabel>
#include <QList>

class BoardControlDialog_t : public QDialog
{
	Q_OBJECT

public:
	BoardControlDialog_t(QWidget *parent = nullptr);
	~BoardControlDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	void clearSections(void);
	void buildDipSwitchSection(void);
	void toggleDipSwitch(size_t idx, bool checked);

	QVBoxLayout *sectionLayout;
	QLabel *emptyLabel;
	QGroupBox *dipBox;
	QList<QCheckBox*> dipChecks;
	QTimer *updateTimer;

public slots:
	void rebuild(void);
	void refresh(void);
	void onRomUnload(void);
};

int openBoardControlWindow(QWidget *parent);
int closeBoardControlWindow(void);
