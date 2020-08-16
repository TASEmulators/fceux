// GamePadConf.h
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
#include <QTextEdit>

class HexEditorDialog_t : public QDialog
{
   Q_OBJECT

	public:
		HexEditorDialog_t(QWidget *parent = 0);
		~HexEditorDialog_t(void);

	protected:
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);

		QTextEdit *txt;

	private:

   public slots:
      void closeWindow(void);
	private slots:

};
