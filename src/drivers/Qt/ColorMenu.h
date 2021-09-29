// ColorMenu.h
//

#pragma once

#include <QWidget>
#include <QWidgetAction>
#include <QAction>
#include <QColor>
#include <QDialog>
#include <QColorDialog>
#include <QCloseEvent>

class ColorMenuPickerDialog_t : public QDialog
{
	Q_OBJECT

	public:
		ColorMenuPickerDialog_t( QColor *c, const char *titleText, QWidget *parent = 0);
		~ColorMenuPickerDialog_t(void);

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

class ColorMenuItem : public QAction
{
	Q_OBJECT

	public:
		ColorMenuItem( QString txt, const char *confName, QWidget *parent = 0);
		~ColorMenuItem(void);

		void connectColor( QColor *c );
	protected:
		QString  title;
		QColor  *colorPtr;
		QColor   lastColor;
		ColorMenuPickerDialog_t *picker;
		const char *confName;

		void setImageColor( QColor c );
	public slots:
		void openColorPicker(void);
		void pickerClosed(int ret);

	signals:
		void colorChanged( QColor &c );
};
