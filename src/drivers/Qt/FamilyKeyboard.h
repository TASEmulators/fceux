#include <QRect>
#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QFont>


class FKB_Key_t
{

	public:
		FKB_Key_t(void)
		{
			vState = kState = 0;
			toggleOnPress = 0;
		}

		char isDown(void)
		{
			return vState || kState;
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
		char   kState;
		char   toggleOnPress;
};

class FamilyKeyboardWidget : public QWidget
{
	Q_OBJECT

public:
	FamilyKeyboardWidget( QWidget *parent = 0);
	~FamilyKeyboardWidget(void);

	static const unsigned int NUM_KEYS = 0x48;

protected:
	//void keyPressEvent(QKeyEvent *event);
	//void kepaintEvent(QPaintEvent *event);
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseDoubleClickEvent(QMouseEvent * event) override;

	int  getKeyAtPoint( QPoint p );
	void calcFontData(void);
	void drawButton( QPainter &painter, int idx, int x, int y, int w, int h );

	int  keyUnderMouse;
	int  keyPressed;
	int  pxCharWidth;
	int  pxCharHeight;
	int  pxBtnGridX;
	int  pxBtnGridY;

	FKB_Key_t  key[NUM_KEYS];

	QTimer *updateTimer;

private slots:
	void updatePeriodic(void);
};
