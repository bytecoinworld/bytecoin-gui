// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QUrl>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>

//#include <CryptoNoteConfig.h>

#include "connectionoptionsframe.h"
#include "settings.h"
#include "common.h"

#include "ui_connectionoptionsframe.h"

namespace WalletGUI {

namespace {

//const char CONNECTION_OPTIONS_STYLE_SHEET_TEMPLATE[] =
//  "WalletGui--ConnectionOptionsFrame {"
//    "background: #ffffff;"
//  "}"

//  "WalletGui--ConnectionOptionsFrame QRadioButton {"
//    "spacing: 20px;"
//  "}"

//  "WalletGui--ConnectionOptionsFrame QRadioButton,"
//  "WalletGui--ConnectionOptionsFrame QLineEdit,"
//  "WalletGui--ConnectionOptionsFrame QSpinBox {"
//    "min-height: 25px;"
//    "max-height: 25px;"
//  "}";

//const char VALIDATION_LABEL_STYLE_SHEET[] =
//    "WalletGUI--ConnectionOptionsFrame QLabel"
//    "{"
//        "color : red;"
//    "}";

enum class ErrorFlags
{
    NO_ERROR = 0,
    WALLET = 1,
    HOST = 2,
};

}

ConnectionOptionsFrame::ConnectionOptionsFrame(QWidget* parent)
    : QFrame(parent)
    , m_ui(new Ui::ConnectionOptionsFrame)
{
    m_ui->setupUi(this);
    connect(m_ui->m_walletdProfileCombo, QOverload<int>::of(&QComboBox::activated),
            this, &ConnectionOptionsFrame::loadWalletdProfile);
    connect(m_ui->m_walletdProfileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            m_ui->m_deleteWalletdProfileButton, [this](int index) {
                m_ui->m_deleteWalletdProfileButton->setEnabled(index > 0);
            });
    connect(m_ui->m_saveWalletdProfileButton, &QPushButton::clicked,
            this, &ConnectionOptionsFrame::saveWalletdProfile);
    connect(m_ui->m_deleteWalletdProfileButton, &QPushButton::clicked,
            this, &ConnectionOptionsFrame::deleteWalletdProfile);
}

ConnectionOptionsFrame::~ConnectionOptionsFrame()
{}

void ConnectionOptionsFrame::load()
{
    refreshWalletdProfiles();

    const QUrl remoteUrl = QUrl::fromUserInput(Settings::instance().getRemoteWalletdEndPoint());
    if (!remoteUrl.isValid() || remoteUrl.host().isEmpty())
    {
        m_ui->m_remoteHostEdit->setText(QStringLiteral("127.0.0.1"));
        m_ui->m_remotePortSpin->setValue(Settings::instance().getDefaultWalletdPort());
    }
    else
    {
        m_ui->m_remoteHostEdit->setText(remoteUrl.host());
        m_ui->m_remotePortSpin->setValue(remoteUrl.port(Settings::instance().getDefaultWalletdPort()));
    }
    remoteHostNameChanged(m_ui->m_remoteHostEdit->text());
}

void ConnectionOptionsFrame::refreshWalletdProfiles()
{
    const QVariantMap profiles = Settings::instance().getWalletdProfiles();
    m_ui->m_walletdProfileCombo->clear();
    m_ui->m_walletdProfileCombo->addItem(tr("Custom endpoint"));
    for (auto it = profiles.cbegin(); it != profiles.cend(); ++it)
        m_ui->m_walletdProfileCombo->addItem(it.key());
    m_ui->m_deleteWalletdProfileButton->setEnabled(m_ui->m_walletdProfileCombo->currentIndex() > 0);
}

void ConnectionOptionsFrame::save()
{
    Settings::instance().setWalletdConnectionMethod(ConnectionMethod::REMOTE);
    Settings::instance().setRemoteWalletdEndPoint(m_ui->m_remoteHostEdit->text(), m_ui->m_remotePortSpin->value());

    switch (Settings::instance().getWalletdConnectionMethod())
    {
    case ConnectionMethod::BUILTIN:
        break;
    case ConnectionMethod::LOCAL:
    case ConnectionMethod::REMOTE:
        Settings::instance().addRecentWallet(Settings::instance().getWalletdEndPoint());
        break;
    }
}

void ConnectionOptionsFrame::loadWalletdProfile(int index)
{
    if (index <= 0)
        return;

    const QVariantMap profile = Settings::instance().getWalletdProfiles()
        .value(m_ui->m_walletdProfileCombo->itemText(index)).toMap();
    m_ui->m_remoteHostEdit->setText(profile.value(QStringLiteral("host")).toString());
    m_ui->m_remotePortSpin->setValue(profile.value(QStringLiteral("port")).toInt());
}

void ConnectionOptionsFrame::saveWalletdProfile()
{
    if (!canAccept())
    {
        QMessageBox::warning(this, tr("Invalid wallet daemon endpoint"),
            tr("Enter a valid host name or IP address before saving a profile."));
        return;
    }

    const QString name = QInputDialog::getText(this, tr("Save wallet daemon profile"),
        tr("Profile name:"), QLineEdit::Normal, QString(), nullptr).trimmed();
    if (name.isEmpty())
        return;

    QVariantMap profiles = Settings::instance().getWalletdProfiles();
    profiles.insert(name, QVariantMap{
        {QStringLiteral("host"), m_ui->m_remoteHostEdit->text().trimmed()},
        {QStringLiteral("port"), m_ui->m_remotePortSpin->value()}
    });
    Settings::instance().setWalletdProfiles(profiles);
    refreshWalletdProfiles();
    const int index = m_ui->m_walletdProfileCombo->findText(name);
    m_ui->m_walletdProfileCombo->setCurrentIndex(index);
}

void ConnectionOptionsFrame::deleteWalletdProfile()
{
    const int index = m_ui->m_walletdProfileCombo->currentIndex();
    if (index <= 0)
        return;

    const QString name = m_ui->m_walletdProfileCombo->itemText(index);
    QVariantMap profiles = Settings::instance().getWalletdProfiles();
    profiles.remove(name);
    Settings::instance().setWalletdProfiles(profiles);
    refreshWalletdProfiles();
}

bool ConnectionOptionsFrame::canAccept() const
{
    return isIpOrHostName(m_ui->m_remoteHostEdit->text());
}

bool ConnectionOptionsFrame::remoteHostNameChanged(const QString& host)
{
    bool valid = true;
    valid = isIpOrHostName(host);
//    const QString errMsg = valid ? QString() : tr("Invalid host name.");
    emit disableAcceptButtonSignal(!valid);
    return valid;
}

bool ConnectionOptionsFrame::walletFileNameChanged(const QString& /*wallet*/)
{
    return true;
}

void ConnectionOptionsFrame::connectionButtonClicked(int /*buttonId*/)
{
    remoteHostNameChanged(m_ui->m_remoteHostEdit->text());
}

void ConnectionOptionsFrame::showOpenWalletFileDialog()
{
    const QString fileName = QFileDialog::getOpenFileName(
                this,
                tr("Open wallet file"),
                QString(),
                tr("Wallet files (*.wallet);;All files (*)"));
    if (fileName.isEmpty())
        return;
}

void ConnectionOptionsFrame::showCreateWalletFileDialog()
{
    const QString fileName = QFileDialog::getSaveFileName(
                this,
                tr("Create wallet file"),
                QString(),
                tr("Wallet files (*.wallet);;All files (*)"));
    if (fileName.isEmpty())
        return;
}

}
