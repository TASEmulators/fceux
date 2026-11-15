// GamePadMenuBrowser.cpp

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include "Qt/GamePadMenuBrowser.h"
#include "Qt/GamePadNavigator.h"

namespace
{
	constexpr int ROLE_ENTRY = Qt::UserRole;

	// Entry value of the row that goes back one menu level
	constexpr int ENTRY_BACK = -1;
	constexpr int ENTRY_NONE = -2;
}

GamePadMenuBrowser::GamePadMenuBrowser(QWidget *parent, QMenuBar *menuBarIn)
	: QDialog(parent),
	  menuBar(menuBarIn)
{
	setWindowTitle(tr("Menu"));
	resize(480, 520);

	QVBoxLayout *vbox = new QVBoxLayout(this);

	pathLabel = new QLabel(this);
	pathLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	pathLabel->setFocusPolicy(Qt::NoFocus);

	list = new QListWidget(this);
	list->setUniformItemSizes(true);
	list->setIconSize(QSize(24, 24));
	list->setFocusPolicy(Qt::StrongFocus);

	hintLabel = new QLabel(tr("A: Select    B: Back"), this);
	hintLabel->setFocusPolicy(Qt::NoFocus);

	closeButton = new QPushButton(tr("Close"), this);
	closeButton->setFocusPolicy(Qt::NoFocus);
	closeButton->setAutoDefault(false);
	closeButton->setDefault(false);

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addWidget(hintLabel, 1);
	hbox->addWidget(closeButton);

	vbox->addWidget(pathLabel);
	vbox->addWidget(list, 1);
	vbox->addLayout(hbox);

	setFocusProxy(list);

	navigator = new GamePadNavigator(this, list);

	connect(navigator, &GamePadNavigator::buttonPressed, this, &GamePadMenuBrowser::onNavigatorButton);
	connect(list, &QListWidget::itemActivated, this, &GamePadMenuBrowser::onItemActivated);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

	populate();
}

QAction *GamePadMenuBrowser::selectedAction(void) const
{
	return chosen;
}

QString GamePadMenuBrowser::plainText(const QAction *action) const
{
	// Strip the mnemonic marker, keep a literal ampersand and drop the shortcut suffix
	QString text = action->text().section('\t', 0, 0);

	text.replace("&&", QString(QChar(0x1)));
	text.remove('&');
	text.replace(QChar(0x1), "&");

	return text;
}

QString GamePadMenuBrowser::actionText(const QAction *action) const
{
	QString text = plainText(action);

	if (action->isCheckable())
	{
		const bool exclusive = (action->actionGroup() != nullptr) && action->actionGroup()->isExclusive();

		if (exclusive)
			text = (action->isChecked() ? "(o) " : "( ) ") + text;
		else
			text = (action->isChecked() ? "[x] " : "[ ] ") + text;
	}

	if (action->menu() != nullptr)
	{
		text += "  >";
	}
	else if (!action->shortcut().isEmpty())
	{
		text += "    " + action->shortcut().toString(QKeySequence::NativeText);
	}
	return text;
}

void GamePadMenuBrowser::populate(int selectRow)
{
	QList<QAction*> actions;

	list->clear();
	entries.clear();

	if (stack.isEmpty())
	{
		pathLabel->setText(tr("Menu"));
		actions = menuBar->actions();
	}
	else
	{
		QString path = tr("Menu");

		for (const Level &l : stack)
		{
			path += "  >  " + l.title;
		}
		pathLabel->setText(path);

		if (stack.last().menu)
		{
			actions = stack.last().menu->actions();
		}

		QListWidgetItem *back = new QListWidgetItem(tr("<  Back"), list);
		back->setData(ROLE_ENTRY, ENTRY_BACK);
		entries.append(nullptr);
	}

	for (QAction *a : actions)
	{
		if (!a->isVisible())
		{
			continue;
		}

		if (a->isSeparator())
		{
			// Avoid leading and repeated separators
			if ((list->count() == 0) || (list->item(list->count() - 1)->data(ROLE_ENTRY).toInt() == ENTRY_NONE) ||
				(list->item(list->count() - 1)->data(ROLE_ENTRY).toInt() == ENTRY_BACK))
			{
				continue;
			}
			QListWidgetItem *sep = new QListWidgetItem(QString(24, QChar(0x2500)), list);
			sep->setFlags(Qt::NoItemFlags);
			sep->setData(ROLE_ENTRY, ENTRY_NONE);
			entries.append(nullptr);
			continue;
		}

		if (a->text().isEmpty())
		{
			continue;
		}
		QListWidgetItem *item = new QListWidgetItem(actionText(a), list);

		item->setIcon(a->icon());
		item->setData(ROLE_ENTRY, static_cast<int>(entries.size()));
		item->setFlags(a->isEnabled() ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable) : Qt::NoItemFlags);

		entries.append(a);
	}

	// Keep the text aligned when only some of the entries have an icon
	bool anyIcon = false;

	for (int i = 0; i < list->count(); i++)
	{
		anyIcon = anyIcon || !list->item(i)->icon().isNull();
	}

	if (anyIcon)
	{
		QPixmap blank(list->iconSize());

		blank.fill(Qt::transparent);

		for (int i = 0; i < list->count(); i++)
		{
			if (list->item(i)->icon().isNull())
			{
				list->item(i)->setIcon(QIcon(blank));
			}
		}
	}

	// Remove a trailing separator
	if ((list->count() > 0) && (list->item(list->count() - 1)->data(ROLE_ENTRY).toInt() == ENTRY_NONE))
	{
		delete list->takeItem(list->count() - 1);
		entries.removeLast();
	}

	int row = -1;

	if ((selectRow >= 0) && (selectRow < list->count()) &&
		(list->item(selectRow)->flags() & Qt::ItemIsEnabled))
	{
		row = selectRow;
	}
	else
	{
		for (int i = 0; i < list->count(); i++)
		{
			if (list->item(i)->flags() & Qt::ItemIsEnabled)
			{
				row = i;
				break;
			}
		}
	}

	if (row >= 0)
	{
		list->setCurrentRow(row);
		list->scrollToItem(list->currentItem(), QAbstractItemView::PositionAtCenter);
	}
}

void GamePadMenuBrowser::enterSubmenu(QAction *action)
{
	QMenu *menu = action->menu();

	if (menu == nullptr)
	{
		return;
	}
	Level level;

	level.menu  = menu;
	level.title = plainText(action);
	level.row   = list->currentRow();

	// Menus that fill themselves on demand do so when they are about to be shown
	emit menu->aboutToShow();

	stack.append(level);

	populate();
}

void GamePadMenuBrowser::goBack(void)
{
	if (stack.isEmpty())
	{
		return;
	}
	const int row = stack.takeLast().row;

	populate(row);
}

void GamePadMenuBrowser::onItemActivated(QListWidgetItem *item)
{
	if (item == nullptr)
	{
		return;
	}
	const int entry = item->data(ROLE_ENTRY).toInt();

	if (entry == ENTRY_BACK)
	{
		goBack();
		return;
	}

	if ((entry < 0) || (entry >= entries.size()) || entries.at(entry).isNull())
	{
		return;
	}
	QAction *action = entries.at(entry);

	if (action->menu() != nullptr)
	{
		enterSubmenu(action);
	}
	else
	{
		chosen = action;
		accept();
	}
}

void GamePadMenuBrowser::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Escape))
	{
		if (!stack.isEmpty())
		{
			goBack();
			event->accept();
			return;
		}
	}
	QDialog::keyPressEvent(event);
}

void GamePadMenuBrowser::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	navigator->start();

	list->setFocus();
}

void GamePadMenuBrowser::hideEvent(QHideEvent *event)
{
	navigator->stop();

	QDialog::hideEvent(event);
}

void GamePadMenuBrowser::onNavigatorButton(GamePadEventHub::Button button)
{
	QListWidgetItem *item = list->currentItem();

	switch (button)
	{
		case GamePadEventHub::Left:
		case GamePadEventHub::X:
			goBack();
			break;
		case GamePadEventHub::Right:
			if ((item != nullptr) && (item->data(ROLE_ENTRY).toInt() >= 0))
			{
				const int entry = item->data(ROLE_ENTRY).toInt();

				if ((entry < entries.size()) && !entries.at(entry).isNull() && entries.at(entry)->menu())
				{
					enterSubmenu(entries.at(entry));
				}
			}
			break;
		case GamePadEventHub::Back:
			reject();
			break;
		default:
			break;
	}
}
