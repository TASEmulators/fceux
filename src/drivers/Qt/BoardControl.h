// BoardControl.h
//
// Dialog for viewing and controlling the state of the loaded cartridge board.
// Currently hosts the DIP switches registered through boardctl.h; further
// board status/control sections can be added as additional group boxes.

#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QList>
#include <QVector>
#include <QString>
#include <cstddef>

// Draws a horizontal bank of slide switches like a physical DIP switch package.
// Switches are drawn left to right in the order given, with the "ON" position
// at the top. The widget does not change its own state when clicked; it emits
// switchClicked() and the owner updates it through setStates().
class DipSwitchBank_t : public QWidget
{
	Q_OBJECT

public:
	struct Item
	{
		QString label;   // text drawn below the switch
		QString tooltip;
		bool on = false;
	};

	DipSwitchBank_t(const QVector<Item> &items, QWidget *parent = nullptr);

	void setStates(const QVector<bool> &states);

	QSize sizeHint(void) const override;
	QSize minimumSizeHint(void) const override;

signals:
	void switchClicked(int index, bool newState);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	bool event(QEvent *event) override;

private:
	int switchAt(const QPoint &pos) const;
	QRect switchRect(int index) const;
	int labelHeight(void) const;

	QVector<Item> items;
};

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
	void refreshDipStates(void);

	QVBoxLayout *sectionLayout;
	QLabel *emptyLabel;
	QGroupBox *dipBox;
	struct DipBankInfo
	{
		DipSwitchBank_t *widget;
		QVector<size_t> indices;   // registry index of each switch, in drawing order
	};
	QList<DipBankInfo> dipBanks;
	QTimer *updateTimer;

public slots:
	void rebuild(void);
	void refresh(void);
	void onRomUnload(void);
};

int openBoardControlWindow(QWidget *parent);
int closeBoardControlWindow(void);
