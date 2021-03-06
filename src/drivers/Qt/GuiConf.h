// GuiConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>
#include <QPalette>
#include <QApplication>
#include <QProxyStyle>
#include <QStyle>

#include "Qt/main.h"

class fceuStyle : public QProxyStyle
{
	Q_OBJECT

public:
	fceuStyle(void);
	fceuStyle(QStyle *style);

	~fceuStyle(void);

	QStyle *baseStyle() const;

	void polish(QPalette &palette) override;
	void polish(QApplication *app) override;

private:
	QStyle *styleBase(QStyle *style = Q_NULLPTR) const;

	std::string  rccFilePath;
};

class guiColorPickerDialog_t : public QDialog
{
	Q_OBJECT

	public:
		guiColorPickerDialog_t( QColor *c, QWidget *parent = 0);
		~guiColorPickerDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		QColorDialog *colorDialog;
		QColor *colorPtr;
		QColor  origColor;

	public slots:
		void closeWindow(void);
	private slots:
		void colorChanged( const QColor &color );
		void colorAccepted(void);
		void colorRejected(void);
		void resetColor(void);
};

class GuiPaletteColorSelect : public QWidget
{
	Q_OBJECT

public:
	GuiPaletteColorSelect( QPalette::ColorGroup group, QPalette::ColorRole role, QWidget *parent = 0);
	~GuiPaletteColorSelect(void);

	void updateColor(void);
	void updatePalette(void);

private:
	QColor     color;
	QCheckBox *cb;
	QLabel    *lbl;
	QPalette::ColorGroup group;
	QPalette::ColorRole  role;

	void setText(void);

	friend class guiColorPickerDialog_t;

private slots:
	void colorEditClicked(void);

};

class GuiPaletteEditDialog_t : public QDialog
{
	Q_OBJECT

public:
	GuiPaletteEditDialog_t(QWidget *parent = 0);
	~GuiPaletteEditDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);
public slots:
	void closeWindow(void);
};


class GuiConfDialog_t : public QDialog
{
	Q_OBJECT

public:
	GuiConfDialog_t(QWidget *parent = 0);
	~GuiConfDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);

	//void loadQss( const char *filepath );

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
	void openQPalette(void);
};
