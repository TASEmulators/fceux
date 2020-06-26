// GamePadConf.cpp
//
#include "GamePadConf.h"

//----------------------------------------------------
GamePadConfDialog_t::GamePadConfDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QHBoxLayout *hbox1, *hbox2;
	QCheckBox *efs_chkbox;

	hbox1 = new QHBoxLayout();

	QLabel *label = new QLabel(tr("Port:"));
	portSel = new QComboBox();
	hbox1->addWidget( label );
	hbox1->addWidget( portSel );

	hbox2 = new QHBoxLayout();
	efs_chkbox = new QCheckBox("Enable Four Score");
	hbox2->addWidget( efs_chkbox );

	QVBoxLayout *mainLayout = new QVBoxLayout();

	mainLayout->addLayout( hbox1 );
	mainLayout->addWidget( efs_chkbox );
	//mainLayout->addLayout( hbox2 );

	setLayout( mainLayout );

	show();
	exec();
}

//----------------------------------------------------
GamePadConfDialog_t::~GamePadConfDialog_t(void)
{

}
//----------------------------------------------------
