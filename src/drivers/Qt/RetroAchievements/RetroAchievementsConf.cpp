// RetroAchievementsConf.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <string>

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPointer>
#include <QVBoxLayout>

#include "../../../fceu.h"
#include "Qt/AudioDriverInterface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RetroAchievements/RetroAchievements.h"
#include "Qt/RetroAchievements/RetroAchievementsConf.h"
#include "Qt/RetroAchievements/RetroAchievementsImages.h"

static const int avatarSize = 64;

static QPointer<RetroAchievementsDialog_t> openDialog;

//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::open(QWidget *parent)
{
	if (openDialog.isNull())
	{
		openDialog = new RetroAchievementsDialog_t(parent);
	}
	openDialog->show();
	openDialog->raise();
	openDialog->activateWindow();
}
//----------------------------------------------------------------------------
RetroAchievementsDialog_t::RetroAchievementsDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QGroupBox *accountBox = new QGroupBox(tr("Account"));
	QGridLayout *grid = new QGridLayout();
	QHBoxLayout *buttonBox = new QHBoxLayout();
	QPushButton *closeButton = new QPushButton(tr("Close"));
	std::string user;

	setWindowTitle(tr("RetroAchievements"));
	setWindowIcon(QIcon(":/retroachievements/ra-icon.png"));
	setAttribute(Qt::WA_DeleteOnClose);

	bool playSound = true;

	enableCbox = new QCheckBox(tr("Enable RetroAchievements"));
	hardcoreCbox = new QCheckBox(tr("Hardcore mode"));
	hardcoreCbox->setToolTip(tr("Disables cheats, rewind, slow motion, frame advance, loading save states, movie playback, scripting, "
		"the debugger and the memory tools while RetroAchievements is enabled. Turning it on resets the game."));
	encoreCbox = new QCheckBox(tr("Encore mode"));
	encoreCbox->setToolTip(tr("Lets achievements you have already unlocked be earned again. "
		"Changing it reloads the achievements of the running game."));
	soundCbox = new QCheckBox(tr("Play a sound when an achievement is unlocked"));
	userEdit = new QLineEdit();
	passwordEdit = new QLineEdit();
	loginButton = new QPushButton(tr("Log In"));
	logoutButton = new QPushButton(tr("Log Out"));
	statusLabel = new QLabel();
	avatarLabel = new QLabel();

	avatarLabel->setFixedSize(avatarSize, avatarSize);
	avatarLabel->setAlignment(Qt::AlignCenter);
	avatarLabel->hide();

	passwordEdit->setEchoMode(QLineEdit::Password);
	userEdit->setMinimumWidth(240);
	statusLabel->setWordWrap(true);
	statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	loginButton->setDefault(true);

	g_config->getOption("SDL.RetroAchUser", &user);
	userEdit->setText(QString::fromStdString(user));

	g_config->getOption("SDL.RetroAchSound", &playSound);
	soundCbox->setChecked(playSound);

	grid->addWidget(new QLabel(tr("Username:")), 0, 0);
	grid->addWidget(userEdit, 0, 1);
	grid->addWidget(new QLabel(tr("Password:")), 1, 0);
	grid->addWidget(passwordEdit, 1, 1);
	grid->addWidget(avatarLabel, 2, 0, Qt::AlignLeft | Qt::AlignVCenter);
	grid->addWidget(statusLabel, 2, 1);
	accountBox->setLayout(grid);

	buttonBox->addWidget(loginButton);
	buttonBox->addWidget(logoutButton);
	buttonBox->addStretch();
	buttonBox->addWidget(closeButton);

	QLabel *logoLabel = new QLabel();

	logoLabel->setPixmap(QPixmap(":/retroachievements/ra-logo.png"));
	logoLabel->setAlignment(Qt::AlignCenter);

	mainLayout->addWidget(logoLabel);
	mainLayout->addWidget(enableCbox);
	mainLayout->addWidget(hardcoreCbox);
	mainLayout->addWidget(encoreCbox);
	mainLayout->addWidget(accountBox);
	mainLayout->addWidget(soundCbox);
	mainLayout->addLayout(buttonBox);
	setLayout(mainLayout);

	connect(enableCbox, &QCheckBox::toggled, this, &RetroAchievementsDialog_t::enableToggled);
	connect(hardcoreCbox, &QCheckBox::clicked, this, &RetroAchievementsDialog_t::hardcoreToggled);
	connect(encoreCbox, &QCheckBox::clicked, this, &RetroAchievementsDialog_t::encoreToggled);
	connect(soundCbox, &QCheckBox::toggled, this, &RetroAchievementsDialog_t::soundToggled);
	connect(loginButton, &QPushButton::clicked, this, &RetroAchievementsDialog_t::loginClicked);
	connect(passwordEdit, &QLineEdit::returnPressed, this, &RetroAchievementsDialog_t::loginClicked);
	connect(logoutButton, &QPushButton::clicked, this, &RetroAchievementsDialog_t::logoutClicked);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

	RetroAchievements *ra = RetroAchievements::instance();

	connect(ra, &RetroAchievements::loginStateChanged, this, &RetroAchievementsDialog_t::loginStateChanged);
	connect(ra, &RetroAchievements::loginFailed, this, &RetroAchievementsDialog_t::loginFailed);
	connect(ra, &RetroAchievements::hardcoreChanged, this, &RetroAchievementsDialog_t::hardcoreChanged);
	connect(ra->images(), &RaImageCache::imageReady, this, &RetroAchievementsDialog_t::imageReady);

	enableCbox->setChecked(ra->isEnabled());

	updateControls();
}
//----------------------------------------------------------------------------
RetroAchievementsDialog_t::~RetroAchievementsDialog_t(void)
{
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::updateControls(void)
{
	RetroAchievements *ra = RetroAchievements::instance();
	const bool enabled = ra->isEnabled();
	const RetroAchievements::LoginState state = ra->getLoginState();
	const bool loggedOut = (state == RetroAchievements::LOGGED_OUT);

	userEdit->setEnabled(enabled && loggedOut);
	passwordEdit->setEnabled(enabled && loggedOut);
	loginButton->setEnabled(enabled && loggedOut);
	logoutButton->setEnabled(enabled && (state == RetroAchievements::LOGGED_IN));
	hardcoreCbox->setEnabled(enabled);
	hardcoreCbox->setChecked(ra->getHardcoreSetting());
	encoreCbox->setEnabled(enabled);
	encoreCbox->setChecked(ra->getEncoreSetting());

	if (!enabled)
	{
		statusLabel->setText(tr("RetroAchievements is disabled."));
	}
	else if (state == RetroAchievements::LOGIN_IN_PROGRESS)
	{
		statusLabel->setText(tr("Logging in..."));
	}
	else if (state == RetroAchievements::LOGGED_IN)
	{
		statusLabel->setText(tr("Logged in as %1.").arg(ra->getUserName()));
	}
	else
	{
		statusLabel->setText(tr("Not logged in."));
	}
	updateAvatar();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::updateAvatar(void)
{
	RetroAchievements *ra = RetroAchievements::instance();
	QPixmap avatar;

	if (ra->isEnabled() && (ra->getLoginState() == RetroAchievements::LOGGED_IN))
	{
		avatar = ra->getAvatar();
	}
	if (avatar.isNull())
	{
		avatarLabel->clear();
		avatarLabel->hide();
		return;
	}
	avatarLabel->setPixmap(avatar.scaled(avatarSize, avatarSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	avatarLabel->show();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::enableToggled(bool checked)
{
	RetroAchievements::instance()->setEnabled(checked);

	updateControls();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::hardcoreToggled(bool checked)
{
	RetroAchievements *ra = RetroAchievements::instance();

	// Hardcore resets a running game, so ask before throwing its progress away.
	if (checked && ra->isEnabled() && (GameInfo != nullptr))
	{
		const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Hardcore mode"),
			tr("Turning on hardcore mode closes the debugging tools and scripts, and resets the running game. Continue?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

		if (answer != QMessageBox::Yes)
		{
			hardcoreCbox->setChecked(false);
			return;
		}
	}
	ra->setHardcore(checked);
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::encoreToggled(bool checked)
{
	RetroAchievements::instance()->setEncore(checked);
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::hardcoreChanged(bool)
{
	updateControls();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::soundToggled(bool checked)
{
	g_config->setOption("SDL.RetroAchSound", checked ? 1 : 0);
	g_config->save();

	// Preview the sound when it is turned on
	if (checked)
	{
		AudioDriverBase::playChime();
	}
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::loginClicked(void)
{
	if (!loginButton->isEnabled() || userEdit->text().trimmed().isEmpty() || passwordEdit->text().isEmpty())
	{
		return;
	}
	RetroAchievements::instance()->loginWithPassword(userEdit->text().trimmed(), passwordEdit->text());

	// The password is not needed after the request is sent.
	passwordEdit->clear();

	updateControls();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::logoutClicked(void)
{
	RetroAchievements::instance()->logout();

	updateControls();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::loginStateChanged(int, const QString &)
{
	updateControls();
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::loginFailed(const QString &message)
{
	statusLabel->setText(tr("Login failed: %1").arg(message));
}
//----------------------------------------------------------------------------
void RetroAchievementsDialog_t::imageReady(const QString &url)
{
	if (url == RetroAchievements::instance()->getAvatarUrl())
	{
		updateAvatar();
	}
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
