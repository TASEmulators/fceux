// GuiConf.h
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
#include <QLineEdit>

#include "Qt/main.h"

class GuiConfDialog_t : public QDialog
{
	Q_OBJECT

public:
	GuiConfDialog_t(QWidget *parent = 0);
	~GuiConfDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);

	void loadQss( const char *filepath );

	QCheckBox *useNativeFileDialog;
	QCheckBox *useNativeMenuBar;
	QCheckBox *useCustomStyle;
	QComboBox *styleComboBox;
	QLineEdit *custom_qss_path;

private:
public slots:
	void closeWindow(void);
private slots:
	void useNativeFileDialogChanged(int v);
	void useNativeMenuBarChanged(int v);
	void useCustomStyleChanged(int v);
	void styleChanged(int index);
	void openQss(void);
	void clearQss(void);
};
