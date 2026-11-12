// RetroAchievementsList.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPair>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include "../../../fceu.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RetroAchievements/RetroAchievementsImages.h"
#include "Qt/RetroAchievements/RetroAchievementsList.h"

static QPointer<RetroAchievementsListDialog_t> openDialog;

static const int badgeSize = 48;
static const int avatarSize = 32;
// Number of leaderboard entries fetched until the user picks another one.
static const int defaultEntryCount = 50;

static QString boardStateText(const RaLeaderboardInfo &board)
{
	QString text;

	switch (board.state)
	{
		case RC_CLIENT_LEADERBOARD_STATE_TRACKING:
			text = QObject::tr("Tracking");
		break;
		case RC_CLIENT_LEADERBOARD_STATE_ACTIVE:
			text = QObject::tr("Ready");
		break;
		default:
			text = QObject::tr("Inactive");
		break;
	}
	if (!board.lastResult.isEmpty())
	{
		text += QObject::tr(" (last: %1").arg(board.lastResult);

		if (!board.bestResult.isEmpty())
		{
			text += QObject::tr(", best: %1").arg(board.bestResult);
		}
		text += ')';
	}
	return text;
}

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
	// Without this the last column (State) is stretched as well and shares the spare width with Description.
	boardTree->header()->setStretchLastSection(false);
	boardTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	boardTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
	boardTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

	entryTree = new QTreeWidget();
	entryTree->setColumnCount(4);
	entryTree->setHeaderLabels({ tr("Rank"), tr("Score"), tr("User"), tr("Submitted") });
	entryTree->setRootIsDecorated(false);
	entryTree->setAlternatingRowColors(true);
	entryTree->setIconSize(QSize(avatarSize, avatarSize));
	entryTree->header()->setStretchLastSection(false);
	entryTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	entryTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	entryTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
	entryTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

	entryMode = new QComboBox();
	entryMode->addItem(tr("Top scores"));
	entryMode->addItem(tr("Around my score"));

	entryCountBox = new QComboBox();

	for (const int count : { 20, 50, 100, 200, 500 })
	{
		entryCountBox->addItem(tr("%1 entries").arg(count), count);
	}
	entryCountBox->setCurrentIndex(entryCountBox->findData(defaultEntryCount));

	QHBoxLayout *entryOptions = new QHBoxLayout();

	entryOptions->addWidget(entryMode, 1);
	entryOptions->addWidget(entryCountBox);

	entryLayout->setContentsMargins(0, 0, 0, 0);
	entryLayout->addLayout(entryOptions);
	entryLayout->addWidget(entryStatus);
	entryLayout->addWidget(entryTree);
	entryPane->setLayout(entryLayout);

	splitter = new QSplitter(Qt::Horizontal);
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

	{
		QSettings settings;

		restoreGeometry(settings.value("retroAchievementsListWindow/geometry").toByteArray());
		splitter->restoreState(settings.value("retroAchievementsListWindow/splitter").toByteArray());

		const int countIndex = entryCountBox->findData(settings.value("retroAchievementsListWindow/entryCount", defaultEntryCount).toInt());

		if (countIndex >= 0)
		{
			entryCountBox->setCurrentIndex(countIndex);
		}
	}

	// Several events can arrive together, so refreshing is deferred and merged.
	refreshTimer.setSingleShot(true);
	refreshTimer.setInterval(100);
	presenceTimer.setInterval(5000);

	connect(&refreshTimer, &QTimer::timeout, this, &RetroAchievementsListDialog_t::refresh);
	connect(&presenceTimer, &QTimer::timeout, this, &RetroAchievementsListDialog_t::updatePresence);
	// Also polled, so the states are right even when no leaderboard event arrived.
	connect(&presenceTimer, &QTimer::timeout, this, &RetroAchievementsListDialog_t::updateBoardStates);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
	connect(boardTree, &QTreeWidget::currentItemChanged, this, &RetroAchievementsListDialog_t::boardSelected);
	connect(entryMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &RetroAchievementsListDialog_t::fetchEntries);
	connect(entryCountBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &RetroAchievementsListDialog_t::fetchEntries);

	// Queued so the window is never updated while the GUI thread holds the emulator lock.
	connect(ra, &RetroAchievements::achievementUnlocked, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameIdentified, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameUnrecognized, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameUnloaded, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::loginStateChanged, &refreshTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
	connect(ra, &RetroAchievements::leaderboardStateChanged, this, &RetroAchievementsListDialog_t::updateBoardStates,
		Qt::QueuedConnection);
	connect(ra, &RetroAchievements::leaderboardSubmitted, this, &RetroAchievementsListDialog_t::boardSubmitted,
		Qt::QueuedConnection);
	connect(ra, &RetroAchievements::leaderboardEntriesReady, this, &RetroAchievementsListDialog_t::entriesReady,
		Qt::QueuedConnection);
	connect(ra->images(), &RaImageCache::imageReady, this, &RetroAchievementsListDialog_t::imageReady);

	refresh();
	presenceTimer.start();
}
//----------------------------------------------------------------------------
RetroAchievementsListDialog_t::~RetroAchievementsListDialog_t(void)
{
	QSettings settings;

	settings.setValue("retroAchievementsListWindow/geometry", saveGeometry());
	settings.setValue("retroAchievementsListWindow/splitter", splitter->saveState());
	settings.setValue("retroAchievementsListWindow/entryCount", entryCountBox->currentData().toInt());
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
		item->setToolTip(1, board.description);
		item->setText(2, boardStateText(board));
		item->setData(0, Qt::UserRole, board.id);
	}
	entryTree->clear();
	avatarItems.clear();
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
	const QList<QTreeWidgetItem*> avatarRows = avatarItems.value(url);

	for (QTreeWidgetItem *item : avatarRows)
	{
		setAvatar(item, url);
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

	fetchEntries();
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::fetchEntries(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if ((selectedBoard == 0) || (ra == nullptr))
	{
		return;
	}
	entryTree->clear();
	avatarItems.clear();
	entryStatus->setText(tr("Loading..."));

	ra->fetchLeaderboardEntries(selectedBoard, entryMode->currentIndex() == 1, entryCount());
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::updateBoardStates(void)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra == nullptr)
	{
		return;
	}
	const QList<RaLeaderboardInfo> boards = ra->getLeaderboards();

	// Only the state column is touched, so the selected leaderboard and its scores stay as they are.
	for (int i = 0; i < boardTree->topLevelItemCount(); i++)
	{
		QTreeWidgetItem *item = boardTree->topLevelItem(i);
		const unsigned int id = item->data(0, Qt::UserRole).toUInt();

		for (const RaLeaderboardInfo &board : boards)
		{
			if (board.id == id)
			{
				const QString text = boardStateText(board);

				if (item->text(2) != text)
				{
					bool debugLog = false;

					g_config->getOption("SDL.RetroAchDebug", &debugLog);

					if (debugLog)
					{
						FCEU_printf("RetroAchievements: leaderboard %u '%s' state: %s\n", id,
							item->text(0).toUtf8().constData(), text.toUtf8().constData());
					}
					item->setText(2, text);
				}
				break;
			}
		}
	}
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::boardSubmitted(unsigned int boardId)
{
	// The new score is on the server now.
	if (boardId == selectedBoard)
	{
		fetchEntries();
	}
}
//----------------------------------------------------------------------------
static bool containsUser(const QList<RaLeaderboardEntry> &entries, const QString &name)
{
	for (const RaLeaderboardEntry &entry : entries)
	{
		if (!name.isEmpty() && (entry.user.compare(name, Qt::CaseInsensitive) == 0))
		{
			return true;
		}
	}
	return false;
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::showLocalResults(const QString &userName)
{
	RetroAchievements *ra = RetroAchievements::instance();
	const QList<RaLeaderboardInfo> boards = (ra != nullptr) ? ra->getLeaderboards() : QList<RaLeaderboardInfo>();

	entryTree->clear();
	avatarItems.clear();

	for (const RaLeaderboardInfo &board : boards)
	{
		if (board.id != selectedBoard)
		{
			continue;
		}
		const QPair<QString, QString> rows[] = { { tr("Best"), board.bestResult }, { tr("Last"), board.lastResult } };

		for (const QPair<QString, QString> &row : rows)
		{
			if (!row.second.isEmpty())
			{
				QTreeWidgetItem *item = new QTreeWidgetItem(entryTree);

				item->setText(0, row.first);
				item->setText(1, row.second);
				item->setText(2, userName);
				item->setText(3, tr("Saved locally"));
			}
		}
	}
	entryStatus->setText(entryTree->topLevelItemCount() > 0
		? tr("You have no ranked score on this leaderboard. Showing the scores saved locally.")
		: tr("You have no score on this leaderboard."));
}
//----------------------------------------------------------------------------
unsigned int RetroAchievementsListDialog_t::entryCount(void) const
{
	return entryCountBox->currentData().toUInt();
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::fillEntries(const QList<RaLeaderboardEntry> &entries, const QString &userName)
{
	entryTree->clear();
	avatarItems.clear();

	for (const RaLeaderboardEntry &entry : entries)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(entryTree);

		item->setText(0, QString::number(entry.rank));
		item->setText(1, entry.score);
		item->setText(2, entry.user);
		item->setText(3, entry.local ? tr("Saved locally") : entry.submitted.toLocalTime().toString(Qt::TextDate));

		// Make the logged in user's own score stand out.
		if (entry.local || (!userName.isEmpty() && (entry.user.compare(userName, Qt::CaseInsensitive) == 0)))
		{
			QFont bold = item->font(0);

			bold.setBold(true);

			for (int col = 0; col < entryTree->columnCount(); col++)
			{
				item->setFont(col, bold);
			}
		}
		if (!entry.avatarUrl.isEmpty())
		{
			avatarItems[entry.avatarUrl].append(item);
			setAvatar(item, entry.avatarUrl);
		}
	}
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::showProjection(const QList<RaLeaderboardEntry> &entries, const QString &userName)
{
	RetroAchievements *ra = RetroAchievements::instance();
	const QList<RaLeaderboardInfo> boards = (ra != nullptr) ? ra->getLeaderboards() : QList<RaLeaderboardInfo>();

	for (const RaLeaderboardInfo &board : boards)
	{
		int position = 0;
		unsigned int rank = 0;

		if ((board.id != selectedBoard) || board.bestResult.isEmpty())
		{
			continue;
		}
		rank = RetroAchievements::projectRank(entries, board.bestResult, board.lowerIsBetter, position);

		// Not in the list that was loaded, so the rank is unknown.
		if ((rank == 0) || ((position >= entries.size()) && (entries.size() >= static_cast<int>(entryCount()))))
		{
			continue;
		}
		RaLeaderboardEntry mine;
		QList<RaLeaderboardEntry> shown = entries;

		mine.rank = rank;
		mine.user = tr("%1 (local best)").arg(userName);
		mine.score = board.bestResult;
		mine.local = true;
		shown.insert(position, mine);

		fillEntries(shown, userName);

		entryStatus->setText(tr("You have no ranked score on this leaderboard. Your best saved score would rank %1.").arg(rank));
		entryTree->setCurrentItem(entryTree->topLevelItem(position));
		entryTree->scrollToItem(entryTree->topLevelItem(position), QAbstractItemView::PositionAtCenter);
		return;
	}
	showLocalResults(userName);
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::entriesReady(unsigned int boardId, bool aroundUser, unsigned int count,
	const QList<RaLeaderboardEntry> &entries)
{
	const bool aroundMode = (entryMode->currentIndex() == 1);
	RetroAchievements *ra = RetroAchievements::instance();
	const QString myName = (ra != nullptr) ? ra->getUserName() : QString();

	if (boardId != selectedBoard)
	{
		return;
	}
	if (count != entryCount())
	{
		return;
	}
	// In this mode the top scores are only asked for to find where the local score would rank.
	if (aroundMode && !aroundUser)
	{
		showProjection(entries, myName);
		return;
	}
	if (aroundUser != aroundMode)
	{
		return;
	}
	// The server lists the top scores instead when it has no entry of the user, for example when the
	// emulator is not approved and nothing was ranked. Where the local score would rank is looked up then.
	if (aroundUser && (ra != nullptr) && !containsUser(entries, myName))
	{
		entryTree->clear();
		avatarItems.clear();
		entryStatus->setText(tr("Loading..."));

		ra->fetchLeaderboardEntries(selectedBoard, false, entryCount());
		return;
	}
	entryStatus->setText(entries.isEmpty() ? tr("No entries, or they could not be loaded.") : QString());

	fillEntries(entries, myName);
}
//----------------------------------------------------------------------------
void RetroAchievementsListDialog_t::setAvatar(QTreeWidgetItem *item, const QString &url)
{
	RetroAchievements *ra = RetroAchievements::instance();
	QPixmap pixmap;

	if (ra != nullptr)
	{
		pixmap = ra->images()->get(url);
	}
	if (pixmap.isNull())
	{
		// Keeps the row height and the text position the same until the image has arrived.
		pixmap = QPixmap(avatarSize, avatarSize);
		pixmap.fill(Qt::transparent);
	}
	item->setIcon(2, QIcon(pixmap.scaled(avatarSize, avatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
