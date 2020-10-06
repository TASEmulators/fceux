// NameTableViewer.h

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QSlider>
#include <QLineEdit>
#include <QGroupBox>
#include <QCloseEvent>

struct  ppuNameTablePixel_t
{
	QColor color;
};

struct ppuNameTableTile_t
{
	struct ppuNameTablePixel_t pixel[8][8];

	int  x;
	int  y;
};

struct ppuNameTable_t
{
	struct ppuNameTableTile_t tile[30][32];

	int  w;
	int  h;
};

class ppuNameTableView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuNameTableView_t( QWidget *parent = 0);
		~ppuNameTableView_t(void);

	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);

		int viewWidth;
		int viewHeight;
};

class ppuNameTableViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ppuNameTableViewerDialog_t(QWidget *parent = 0);
		~ppuNameTableViewerDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *bar);

		ppuNameTableView_t *ntView;
		QCheckBox *showScrollLineCbox;
		QCheckBox *showAttrbCbox;
		QCheckBox *ignorePaletteCbox;
		QSlider   *refreshSlider;
		QLineEdit *scanLineEdit;
		QTimer    *updateTimer;
		QRadioButton *horzMirrorBtn;
		QRadioButton *vertMirrorBtn;
		QRadioButton *fourScreenBtn;
		QRadioButton *singleScreenBtn[4];
		QLabel *tileID;
		QLabel *tileXY;
		QLabel *ppuAddrLbl;
		QLabel *attrbLbl;

	public slots:
      void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		void updateMirrorButtons(void);
		void horzMirrorClicked(void);
		void vertMirrorClicked(void);
		void fourScreenClicked(void);
		void singleScreen0Clicked(void);
		void singleScreen1Clicked(void);
		void singleScreen2Clicked(void);
		void singleScreen3Clicked(void);
		void refreshSliderChanged(int value);
		void scanLineChanged( const QString &txt );
};

int openNameTableViewWindow( QWidget *parent );

