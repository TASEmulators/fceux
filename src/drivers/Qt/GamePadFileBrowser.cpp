// GamePadFileBrowser.cpp

#include <QApplication>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "Qt/GamePadFileBrowser.h"

namespace
{
	constexpr int REPEAT_DELAY_MS    = 400;
	constexpr int REPEAT_INTERVAL_MS = 80;
	constexpr int PUMP_INTERVAL_MS   = 10;

	constexpr int ROLE_TYPE = Qt::UserRole;
	constexpr int ROLE_PATH = Qt::UserRole + 1;

	// Returns the key a button is translated into, or 0 if the button
	// is unmapped or handled by the dialog directly
	int buttonToKey(GamePadEventHub::Button button)
	{
		switch (button)
		{
			case GamePadEventHub::Up:    return Qt::Key_Up;
			case GamePadEventHub::Down:  return Qt::Key_Down;
			case GamePadEventHub::A:
			case GamePadEventHub::Start: return Qt::Key_Return;
			case GamePadEventHub::B:     return Qt::Key_Escape;
			case GamePadEventHub::L2:    return Qt::Key_PageUp;
			case GamePadEventHub::R2:    return Qt::Key_PageDown;
			default:                     return 0;
		}
	}

	bool isRepeatable(GamePadEventHub::Button button)
	{
		return (button == GamePadEventHub::Up)   || (button == GamePadEventHub::Down) ||
		       (button == GamePadEventHub::L2)   || (button == GamePadEventHub::R2);
	}
}

GamePadFileBrowser::GamePadFileBrowser(QWidget *parent, const QString &caption)
	: QDialog(parent),
	  iconProvider(new QFileIconProvider()),
	  currentDir(QDir::home()),
	  placesMode(false),
	  filterIdx(0),
	  repeatButton(GamePadEventHub::NumButtons),
	  repeatKey(0)
{
	setWindowTitle(caption);
	resize(640, 480);

	QVBoxLayout *vbox = new QVBoxLayout(this);

	pathLabel = new QLabel(this);
	pathLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	pathLabel->setFocusPolicy(Qt::NoFocus);

	list = new QListWidget(this);
	list->setUniformItemSizes(true);
	list->setFocusPolicy(Qt::StrongFocus);

	filterLabel = new QLabel(this);
	filterLabel->setFocusPolicy(Qt::NoFocus);

	openButton   = new QPushButton(tr("Open"), this);
	cancelButton = new QPushButton(tr("Cancel"), this);

	for (QPushButton *b : { openButton, cancelButton })
	{
		b->setFocusPolicy(Qt::NoFocus);
		b->setAutoDefault(false);
		b->setDefault(false);
	}

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addWidget(filterLabel, 1);
	hbox->addWidget(openButton);
	hbox->addWidget(cancelButton);

	vbox->addWidget(pathLabel);
	vbox->addWidget(list, 1);
	vbox->addLayout(hbox);

	setFocusProxy(list);

	pumpTimer.setInterval(PUMP_INTERVAL_MS);
	pumpTimer.setTimerType(Qt::PreciseTimer);

	connect(&pumpTimer, &QTimer::timeout, GamePadEventHub::instance(), &GamePadEventHub::pump);

	repeatTimer.setSingleShot(false);

	connect(&repeatTimer, &QTimer::timeout, this, &GamePadFileBrowser::onRepeatTimeout);
	connect(list, &QListWidget::itemActivated, this, &GamePadFileBrowser::onItemActivated);
	connect(openButton, &QPushButton::clicked, this, &GamePadFileBrowser::onOpenClicked);
	connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	connect(GamePadEventHub::instance(), &GamePadEventHub::buttonChanged,
		this, &GamePadFileBrowser::onGamePadButton);

	populate();
}

GamePadFileBrowser::~GamePadFileBrowser(void)
{
	delete iconProvider;
}

void GamePadFileBrowser::setNameFilters(const QStringList &filters)
{
	static const QRegularExpression parens("\\(([^)]*)\\)\\s*$");
	static const QRegularExpression splitter("[\\s,;]+");

	filterTitles.clear();
	filterPatterns.clear();

	for (const QString &f : filters)
	{
		QRegularExpressionMatch m = parens.match(f);

		QString title = m.hasMatch() ? f.left(m.capturedStart()).trimmed() : f.trimmed();
		QString pats  = m.hasMatch() ? m.captured(1) : f;

		QStringList patList = pats.split(splitter, Qt::SkipEmptyParts);

		filterTitles.append(title.isEmpty() ? pats : title);
		filterPatterns.append(patList);
	}
	filterIdx = 0;

	updateFilterLabel();
	populate();
}

void GamePadFileBrowser::setPlaces(const QStringList &paths)
{
	places = paths;
	populate();
}

void GamePadFileBrowser::setDirectory(const QString &path)
{
	QDir d(path);

	if (d.exists())
	{
		currentDir = d;
	}
	populate();
}

void GamePadFileBrowser::selectFile(const QString &path)
{
	QFileInfo fi(path);

	if (fi.absoluteDir().exists())
	{
		currentDir = fi.absoluteDir();
	}
	populate(fi.absoluteFilePath());
}

QString GamePadFileBrowser::selectedFile(void) const
{
	return chosenFile;
}

void GamePadFileBrowser::updateFilterLabel(void)
{
	if (filterTitles.isEmpty())
	{
		filterLabel->clear();
	}
	else
	{
		filterLabel->setText(tr("Filter: %1").arg(filterTitles.at(filterIdx)));
	}
}

void GamePadFileBrowser::addEntry(const QString &text, EntryType type, const QString &path)
{
	QListWidgetItem *item = new QListWidgetItem(text, list);

	item->setData(ROLE_TYPE, static_cast<int>(type));
	item->setData(ROLE_PATH, path);

	switch (type)
	{
		case EntryFile:
			item->setIcon(iconProvider->icon(QFileIconProvider::File));
			break;
		default:
			item->setIcon(iconProvider->icon(QFileIconProvider::Folder));
			break;
	}
}

void GamePadFileBrowser::populate(const QString &selectPath)
{
	placesMode = false;

	list->clear();

	pathLabel->setText(currentDir.absolutePath());

	if (!currentDir.isRoot())
	{
		addEntry("..", EntryParent, QString());
	}

	if (!places.isEmpty())
	{
		addEntry(tr("[ Places ]"), EntryPlaces, QString());
	}
	const int firstReal = list->count();

	const QDir::SortFlags sort = QDir::Name | QDir::IgnoreCase;

	for (const QFileInfo &fi : currentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, sort))
	{
		addEntry(fi.fileName() + "/", EntryDir, fi.absoluteFilePath());
	}

	const QStringList patterns = filterPatterns.isEmpty() ? QStringList() : filterPatterns.at(filterIdx);

	for (const QFileInfo &fi : currentDir.entryInfoList(patterns, QDir::Files, sort))
	{
		addEntry(fi.fileName(), EntryFile, fi.absoluteFilePath());
	}

	int row = (list->count() > firstReal) ? firstReal : 0;

	if (!selectPath.isEmpty())
	{
		for (int i = 0; i < list->count(); i++)
		{
			if (list->item(i)->data(ROLE_PATH).toString() == selectPath)
			{
				row = i;
				break;
			}
		}
	}

	if (list->count() > 0)
	{
		list->setCurrentRow(row);
		list->scrollToItem(list->currentItem(), QAbstractItemView::PositionAtCenter);
	}
}

void GamePadFileBrowser::showPlaces(void)
{
	placesMode = true;

	list->clear();

	pathLabel->setText(tr("Places"));

	for (const QString &p : places)
	{
		if (!QDir(p).exists())
		{
			continue;
		}
		QString name = QFileInfo(p).fileName();

		addEntry(name.isEmpty() ? p : (name + "    " + p), EntryPlace, p);
	}

	if (list->count() > 0)
	{
		list->setCurrentRow(0);
	}
}

void GamePadFileBrowser::goToParent(void)
{
	if (placesMode)
	{
		populate(currentDir.absolutePath());
		return;
	}

	if (currentDir.isRoot())
	{
		if (!places.isEmpty())
		{
			showPlaces();
		}
		return;
	}
	const QString previous = currentDir.absolutePath();

	if (currentDir.cdUp())
	{
		populate(previous);
	}
}

void GamePadFileBrowser::cycleFilter(int delta)
{
	const int n = filterPatterns.size();

	if ((n <= 1) || placesMode)
	{
		return;
	}
	QListWidgetItem *cur = list->currentItem();
	const QString curPath = (cur != nullptr) ? cur->data(ROLE_PATH).toString() : QString();

	filterIdx = (filterIdx + delta + n) % n;

	updateFilterLabel();
	populate(curPath);
}

void GamePadFileBrowser::onItemActivated(QListWidgetItem *item)
{
	if (item == nullptr)
	{
		return;
	}
	const int type = item->data(ROLE_TYPE).toInt();
	const QString path = item->data(ROLE_PATH).toString();

	switch (type)
	{
		case EntryParent:
			goToParent();
			break;
		case EntryPlaces:
			showPlaces();
			break;
		case EntryPlace:
		case EntryDir:
			currentDir = QDir(path);
			populate();
			break;
		case EntryFile:
			chosenFile = path;
			accept();
			break;
	}
}

void GamePadFileBrowser::onOpenClicked(void)
{
	onItemActivated(list->currentItem());
}

void GamePadFileBrowser::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Backspace)
	{
		goToParent();
		event->accept();
		return;
	}
	QDialog::keyPressEvent(event);
}

void GamePadFileBrowser::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	pumpTimer.start();

	list->setFocus();

	if (list->currentItem() != nullptr)
	{
		list->scrollToItem(list->currentItem(), QAbstractItemView::PositionAtCenter);
	}
}

void GamePadFileBrowser::hideEvent(QHideEvent *event)
{
	pumpTimer.stop();
	repeatTimer.stop();
	repeatButton = GamePadEventHub::NumButtons;

	QDialog::hideEvent(event);
}

void GamePadFileBrowser::sendKey(int key)
{
	QCoreApplication::postEvent(list, new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier));
	QCoreApplication::postEvent(list, new QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier));
}

void GamePadFileBrowser::onGamePadButton(int devIdx, GamePadEventHub::Button button, bool pressed)
{
	(void)devIdx;

	if (!pressed)
	{
		if (button == repeatButton)
		{
			repeatTimer.stop();
			repeatButton = GamePadEventHub::NumButtons;
		}
		return;
	}

	// Pad input is also reported while another window has focus
	if (!isVisible() || !isActiveWindow())
	{
		return;
	}

	switch (button)
	{
		case GamePadEventHub::X:
			goToParent();
			return;
		case GamePadEventHub::L1:
			cycleFilter(-1);
			return;
		case GamePadEventHub::R1:
			cycleFilter(1);
			return;
		default:
			break;
	}

	const int key = buttonToKey(button);

	if (key == 0)
	{
		return;
	}
	sendKey(key);

	if (isRepeatable(button))
	{
		repeatButton = button;
		repeatKey = key;
		repeatTimer.start(REPEAT_DELAY_MS);
	}
}

void GamePadFileBrowser::onRepeatTimeout(void)
{
	if (!isVisible() || (repeatButton == GamePadEventHub::NumButtons))
	{
		repeatTimer.stop();
		return;
	}
	repeatTimer.setInterval(REPEAT_INTERVAL_MS);

	sendKey(repeatKey);
}
