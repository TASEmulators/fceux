// PaletteEditor.h
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
#include <QFont>
#include <QColor>
#include <QGroupBox>
#include <QPainter>
#include <QColorDialog>

#include "Qt/main.h"

class nesColorPickerDialog_t : public QDialog
{
	Q_OBJECT

	public:
		nesColorPickerDialog_t( int palIndex, QColor *c, QWidget *parent = 0);
		~nesColorPickerDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		QColorDialog *colorDialog;
		QColor *colorPtr;
		QColor  origColor;
		int     palIdx;

	public slots:
		void closeWindow(void);
	private slots:
		void colorChanged( const QColor &color );
		void colorAccepted(void);
		void colorRejected(void);
		void resetColor(void);
};

class nesPaletteView : public QWidget
{
	Q_OBJECT

	public:
		nesPaletteView( QWidget *parent = 0);
		~nesPaletteView(void);

		static const int NUM_COLORS = 0x40;

		QColor  color[ NUM_COLORS ];

		void    loadActivePalette(void);
		QPoint  convPixToCell( QPoint p );

		void  setActivePalette(void);
		void  openColorPicker(void);
		int   loadFromFile( const char *filepath );
		int   saveToFile( const char *filepath );

	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void contextMenuEvent(QContextMenuEvent *event);

		QFont  font;
		QPoint selCell;

		int  viewWidth;
		int  viewHeight;
		int  boxWidth;
		int  boxHeight;
		int  pxCharWidth;
		int  pxCharHeight;

	private slots:
		void editSelColor(void);
};

class PaletteEditorDialog_t : public QDialog
{
   Q_OBJECT

	public:
		PaletteEditorDialog_t(QWidget *parent = 0);
		~PaletteEditorDialog_t(void);

		nesPaletteView  *palView;

	protected:
		void closeEvent(QCloseEvent *event);

	private:

	public slots:
		void closeWindow(void);
	private slots:
		void openPaletteFileDialog(void);
		void savePaletteFileDialog(void);
		void setActivePalette(void);

};
