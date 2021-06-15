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
		int   exportToFileACT( const char *filepath );

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

		QAction *undoAct;
		QAction *redoAct;
		QTimer  *updateTimer;
	private:

	public slots:
		void closeWindow(void);
	private slots:
		void updatePeriodic(void);
		void openPaletteFileDialog(void);
		void savePaletteFileDialog(void);
		void exportPaletteFileDialog(void);
		void setActivePalette(void);
		void undoLastOperation(void);
		void redoLastOperation(void);

};

class nesPalettePickerView : public QWidget
{
	Q_OBJECT

	public:
		nesPalettePickerView( QWidget *parent = 0);
		~nesPalettePickerView(void);

		static const int NUM_COLORS = 0x40;

		QColor  color[ NUM_COLORS ];

		void    loadActivePalette(void);
		QPoint  convPixToCell( QPoint p );
		void    setSelBox( int val );
		void    setSelBox( QPoint p );
		int     getSelBox(void){ return selBox; };
		void    setPalAddr( int a );

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
		int  selBox;
		int  palAddr;

	private slots:
};

class nesPalettePickerDialog : public QDialog
{
   Q_OBJECT

	public:
		nesPalettePickerDialog(int idx, QWidget *parent = 0);
		~nesPalettePickerDialog(void);

		nesPalettePickerView  *palView;

	protected:
		void closeEvent(QCloseEvent *event);

		int  palIdx;
		int  palAddr;
		int  palOrigVal;
	private:

	public slots:
		void closeWindow(void);
	private slots:
		void cancelButtonClicked(void);
		void resetButtonClicked(void);
		void okButtonClicked(void);

};
