#include <QRect>
#include <QWidget>
#include <QFont>


class FKB_Key_t
{

	public:
		QRect  rect;
		char   name[16];
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
	void paintEvent(QPaintEvent *event);

	void calcFontData(void);

	int  pxCharWidth;
	int  pxCharHeight;
	int  pxBtnGridX;
	int  pxBtnGridY;

	FKB_Key_t  key[NUM_KEYS];
};
