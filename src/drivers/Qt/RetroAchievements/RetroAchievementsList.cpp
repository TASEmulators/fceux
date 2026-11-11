// RetroAchievementsList.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Qt/RetroAchievements/RetroAchievementsImages.h"
#include "Qt/RetroAchievements/RetroAchievementsList.h"

static QPointer<RetroAchievementsListDialog_t> openDialog;

static const int badgeSize = 48;

//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::open(QWidget *parent)
{
	if (RetroAchievements::instance() == nullptr)
	{
		return;
	}
	if (openDialog.isNull())
	{
		openDialog = new RetroAchievementsListDialog_t(parent);
	}
	openDialog->show();
	openDialog->raise();
	openDialog->activateWindow();
}
//----------------------------------------------------------------------------
RetroAchievementsListDialog_t::RetroAchievementsListDialog_t(QWidget *parent)
	: QDialog(parent)
	, selectedBoard(0)
{
	RetroAchievements *ra = RetroAchievements::instance();
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QHBoxLayout *header = new QHBoxLayout();
	QVBoxLayout *headerText = new QVBoxLayout();
	QTabWidget *tabs = new QTabWidget();
	QSplitter *splitter = new QSplitter(Qt::Horizontal);
	QWidget *entryPane = new QWidget();
	QVBoxLayout *entryLayout = new QVBoxLayout();
	QHBoxLayout *buttons = new QHBoxLayout();
	QPushButton *closeButton = new QPushButton(tr("Close"));
	QFont titleFont;

	setWindowTitle(tr("RetroAchievements"));
	setWindowIcon(QIcon(":/retroachievements/ra-icon.png"));
	setAttribute(Qt::WA_DeleteOnClose);
	resize(760, 620);

	badgeLabel = new QLabel();
	titleLabel = new QLabel();
	summaryLabel = new QLabel();
	presenceLabel = new QLabel();
	entryStatus = new QLabel();

	badgeLabel->setFixedSize(64, 64);
	badgeLabel->setScaledContents(true);
	titleFont = titleLabel->font();
	titleFont.setBold(true);
	titleFont.setPointSizeF(titleFont.pointSizeF() + 3.0);
	titleLabel->setFont(titleFont);
	presenceLabel->setWordWrap(true);

	headerText->addWidget(titleLabel);
	headerText->addWidget(summaryLabel);
	headerText->addWidget(presenceLabel);
	headerText->addStretch();
	header->addWidget(badgeLabel);
	header->addLayout(headerText, 1);

	achTree = new QTreeWidget();
	achTree->setColumnCount(3);
	achTree->setHeaderLabels({ tr("Achievement"), tr("Points"), tr("Status") });
	achTree->setIconSize(QSize(badgeSize, badgeSize));
	achTree->setRootIsDecorated(true);
	achTree->setAlternatingRowColors(true);
	achTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	achTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	achTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	boardTree = new QTreeWidget();
	boardTree->setColumnCount(3);
	boardTree->setHeaderLabels({ tr("Leaderboard"), tr("Description"), tr("State") });
	boardTree->setRootIsDecorated(false);
	boardTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	boardTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
	boardTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	entryTree = new QTreeWidget();
	entryTree->setColumnCount(4);
	entryTree->setHeaderLabels({ tr("Rank"), tr("User"), tr("Score"), tr("Submitted") });
	entryTree->setRootIsDecorated(false);
	entryTree->setAlternatingRowColors(true);

	entryLayout->setContentsMargins(0, 0, 0, 0);
	entryLayout->addWidget(entryStatus);
	entryLayout->addWidget(entryTree);
	entryPane->setLayout(entryLayout);

	splitter->addWidget(boardTree);
	splitter->addWidget(entryPane);
	splitter->setStretchFactor(0, 3);
	splitter->setStretchFactor(1, 2);

	tabs->addTab(achTree, tr("Achievements"));
	tabs->addTab(splitter, tr("Leaderboards"));

	buttons->addStretch();
	buttons->addWidget(closeButton);

	mainLayout->addLayout(header);
	mainLayout->addWidget(tabs, 1);
	mainLayout->addLayout(buttons);
	setLayout(mainLayout);

	// Several events can arrive together, so refreshing is deferred and merged.
	refreshTimer.setSingleShot(true);
	refreshTimer.setInterval(100);
	presenceTimer.setInterval(5000);

	connect(&refreshTimer, &QTimer::timeout, this, &RetroAchievementsListDialog_t::refresh);
	connect(&presenceTimer, &QTimer::timeout, this, &RetroAchievementsListDialog_t::updatePresence);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
	connect(boardTree, &QTreeWidget::currentItemChanged, this, &RetroAchievementsListDialog_t::boardSelected);

	// Queued so the window is never updated while the GUI thread holds the emulator lock.
	connect(ra, &RetroAchievements::achievementUnlocked, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameIdentified, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameUnrecognized, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameUnloaded, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::loginStateChanged, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::leaderboardEntriesReady, this, &RetroAchievementsListDialog_t::entriesReady,
		Qt::QueuedConnection);
	connect(ra->images(), &RaImageCache::imageReady, this, &RetroAchievementsListDialog_t::imageReady);

	refresh();
	presenceTimer.start();
}
//----------------------------------------------------------------------------
RetroAchievementsListDialog_t::~RetroAchievementsListDialog_t(void)
{
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::setItemImage(QTreeWidgetItem *item, const QString &url)
{
	RetroAchievements *ra = RetroAchievements::instance();
	QPixmap pixmap;

	if (ra != nullptr)
	{
		pixmap = ra->images()->get(url);
	}
	if (pixmap.isNull())
	{
		pixmap = QPixmap(badgeSize, badgeSize);
		pixmap.fill(Qt::transparent);
	}
	item->setIcon(0, QIcon(pixmap.scaled(badgeSize, badgeSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::refresh(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra == nullptr)
	{
		return;
	}
	const RaGameSummary game = ra->getGameSummary();
	const QList<RaAchievementBucket> buckets = ra->getAchievements();
	const QList<RaLeaderboardInfo> boards = ra->getLeaderboards();
	QFont boldFont = achTree->font();

	boldFont.setBold(true);

	if (!game.loaded)
	{
		if (!ra->isEnabled())
		{
			titleLabel->setText(tr("RetroAchievements is disabled"));
		}
		else if (game.unrecognized)
		{
			titleLabel->setText(tr("Unrecognized game (not found on RetroAchievements)"));
		}
		else
		{
			titleLabel->setText(tr("No game loaded"));
		}
		summaryLabel->setText(QString());
		badgeLabel->clear();
		gameBadgeUrl.clear();
	}
	else
	{
		titleLabel->setText(game.title);
		summaryLabel->setText(tr("%1 of %2 achievements, %3 of %4 points")
			.arg(game.numUnlocked).arg(game.numAchievements).arg(game.pointsUnlocked).arg(game.pointsTotal));
		gameBadgeUrl = game.badgeUrl;
		badgeLabel->setPixmap(ra->images()->get(game.badgeUrl));
	}
	presenceLabel->setText(game.richPresence);

	imageItems.clear();
	achTree->clear();

	for (const RaAchievementBucket &bucket : buckets)
	{
		QTreeWidgetItem *top = new QTreeWidgetItem(achTree);

		top->setText(0, QString("%1 (%2)").arg(bucket.label).arg(bucket.achievements.size()));
		top->setFont(0, boldFont);

		for (const RaAchievementInfo &ach : bucket.achievements)
		{
			QTreeWidgetItem *item = new QTreeWidgetItem(top);

			item->setText(0, QString("%1\n%2").arg(ach.title, ach.description));
			item->setText(1, QString::number(ach.points));
			item->setText(2, ach.unlocked ? tr("Unlocked") : ach.measuredProgress);
			item->setToolTip(0, ach.description);

			setItemImage(item, ach.badgeUrl);
			imageItems[ach.badgeUrl].append(item);
		}
		top->setExpanded(true);
	}

	boardTree->clear();

	for (const RaLeaderboardInfo &board : boards)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(boardTree);

		item->setText(0, board.title);
		item->setText(1, board.description);
		item->setText(2, board.tracking ? tr("Tracking") : QString());
		item->setData(0, Qt::UserRole, board.id);
	}
	entryTree->clear();
	entryStatus->setText(boards.isEmpty() ? tr("This game has no leaderboards.") : tr("Select a leaderboard to see the top scores."));
	selectedBoard = 0;
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::updatePresence(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra != nullptr)
	{
		presenceLabel->setText(ra->getGameSummary().richPresence);
	}
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::imageReady(const QString &url)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra == nullptr)
	{
		return;
	}
	if (url == gameBadgeUrl)
	{
		badgeLabel->setPixmap(ra->images()->get(url));
	}
	const QList<QTreeWidgetItem*> items = imageItems.value(url);

	for (QTreeWidgetItem *item : items)
	{
		setItemImage(item, url);
	}
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::boardSelected(void)
{
	QTreeWidgetItem *item = boardTree->currentItem();
	RetroAchievements *ra = RetroAchievements::instance();

	if ((item == nullptr) || (ra == nullptr))
	{
		return;
	}
	selectedBoard = item->data(0, Qt::UserRole).toUInt();

	entryTree->clear();
	entryStatus->setText(tr("Loading..."));

	ra->fetchLeaderboardEntries(selectedBoard);
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::entriesReady(unsigned int boardId, const QList<RaLeaderboardEntry> &entries)
{
	if (boardId != selectedBoard)
	{
		return;
	}
	entryTree->clear();
	entryStatus->setText(entries.isEmpty() ? tr("No entries, or they could not be loaded.") : QString());

	for (const RaLeaderboardEntry &entry : entries)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(entryTree);

		item->setText(0, QString::number(entry.rank));
		item->setText(1, entry.user);
		item->setText(2, entry.score);
		item->setText(3, entry.submitted.toLocalTime().toString(Qt::TextDate));
	}
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
