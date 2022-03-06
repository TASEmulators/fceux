#include <QRect>
#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QMouseEvent>
#include <QTreeWidget>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QLabel>
#include <QTimer>
#include <QFont>
#include <QPropertyAnimation>

#include "Qt/main.h"

class FKB_Key_t
{

	public:
		FKB_Key_t(void)
		{
			vState = hwState = 0;
			toggleOnPress = 0;
		}

		char isDown(void)
		{
			return vState || hwState;
		}

		char pressed(void)
		{
			if ( toggleOnPress )
			{
				vState = !vState;
			}
			else
			{
				vState =  1;
			}
			return vState;
		}

		char released(void)
		{
			if ( !toggleOnPress )
			{
				vState = 0;
			}
			return vState;
		}

		QRect  rect;
		char   vState;
		char   hwState;
		char   toggleOnPress;
};

class FamilyKeyboardWidget : public QWidget
{
	Q_OBJECT

public:
	FamilyKeyboardWidget( QWidget *parent = 0);
	~FamilyKeyboardWidget(void);

	static const unsigned int NUM_KEYS = 0x48;

	FKB_Key_t  key[NUM_KEYS];

	void setFont( const QFont &newFont );

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseDoubleClickEvent(QMouseEvent * event) override;
	void leaveEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

	int  getKeyAtPoint( QPoint p );
	void calcFontData(void);
	void updateHardwareStatus(void);
	void drawButton( QPainter &painter, int idx, int x, int y, int w, int h );

	int  ctxMenuKey;
	int  keyUnderMouse;
	int  keyPressed;
	int  pxCharWidth;
	int  pxCharHeight;
	int  pxBtnGridX;
	int  pxBtnGridY;

	QTimer *updateTimer;

private slots:
	void updatePeriodic(void);
	void ctxMapPhysicalKey(void);
	void ctxChangeToggleOnPress(void);
};

class FKBKeyMapDialog : public QDialog
{
	Q_OBJECT

public:
	FKBKeyMapDialog(int idx, QWidget *parent = 0);
	~FKBKeyMapDialog(void);

	int buttonConfigStatus;

	void enterButtonLoop(void);

protected:
	void closeEvent(QCloseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void keyReleaseEvent(QKeyEvent *event) override;

	int  keyIdx;

	QLabel *msgLbl;
	QLabel *curMapLbl;

public slots:
	void closeWindow(void);
};

class FKBConfigDialog : public QDialog
{
	Q_OBJECT

public:
	FKBConfigDialog(QWidget *parent = 0);
	~FKBConfigDialog(void);

	FamilyKeyboardWidget *keyboard;

	void updateBindingList(void);

protected:
	void closeEvent(QCloseEvent *event);

	QMenuBar *buildMenuBar(void);

	QTreeWidget *keyTree;

	QLabel *statLbl;

	QTimer *updateTimer;

	QPropertyAnimation *keyTreeHgtAnimation;

public slots:
	void closeWindow(void);

private slots:
	void updatePeriodic(void);
	void updateStatusLabel(void);
	void openFontDialog(void);
	void toggleTreeView(bool);
	void keyTreeResizeDone(void);
	void keyTreeHeightChange(const QVariant &);
	void keyTreeItemActivated(QTreeWidgetItem *item, int column);
};

int  openFamilyKeyboardDialog( QWidget *parent );

char getFamilyKeyboardVirtualKey( int idx );
