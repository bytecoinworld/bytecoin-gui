// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QDataWidgetMapper>
#include <QDateTime>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QAbstractItemModel>
#include <QPainter>
#include <QTimer>

#include "statusbar.h"
//#include "statusmodel.h"
#include "walletmodel.h"
#include "JsonRpc/JsonRpcClient.h"

namespace WalletGUI {

namespace {

//const char STATUS_BAR_STYLE_SHEET_TEMPLATE[] =
//  "WalletGui--WalletStatusBar, "
//  "WalletGui--WalletStatusBar QLabel {"
//    "background-color: %statusBarBackgroundColor%;"
//    "color: %statusBarFontColor%;"
//    "font-size: %fontSizeTiny%;"
//  "}";

const char STATUS_BAR_STYLE_SHEET_TEMPLATE[] =
    "WalletGUI--WalletStatusBar::item {"
    "border: none;"
    "}";
}

WalletStatusBar::WalletStatusBar(QWidget* parent)
    : QStatusBar(parent)
    , walletModel_(nullptr)
    , m_syncStatusLabel(new QLabel(this))
    , m_syncStatusIconLabel(new QLabel(this))
    , m_peerCountIconLabel(new QLabel(this))
//    , m_hdStatusLabel(new QLabel(this))
//    , m_encryptionStatusIconLabel(new QLabel(this))
    , m_peerCountLabel(new QLabel(this))
    , m_walletConnectionIconLabel(new QLabel(this))
    , m_walletConnectionLabel(new QLabel(this))
    , m_bytecoindConnectionIconLabel(new QLabel(this))
    , m_bytecoindConnectionLabel(new QLabel(this))
    , stateMapper_(new QDataWidgetMapper(this))
    , updateTimer_(new QTimer(this))
    , m_syncAnimationTimer(new QTimer(this))
    , m_syncIconPixmap(QIcon(QStringLiteral(":/icons/footer/sync.svg")).pixmap(16, 16))
    , m_syncAngle(0)
    , isSynchronized_(false)
{
    m_syncStatusLabel->setObjectName("m_syncStatusLabel");
    m_syncStatusIconLabel->setObjectName("m_syncStatusIconLabel");
    m_peerCountIconLabel->setPixmap(QIcon(QStringLiteral(":/icons/footer/peers.svg")).pixmap(16, 16));
    m_peerCountIconLabel->setToolTip(tr("Peer connections"));
//    m_encryptionStatusIconLabel->setObjectName("m_encryptionStatusIconLabel");
    m_peerCountLabel->setObjectName("m_peerCountLabel");
    m_walletConnectionIconLabel->setPixmap(QIcon(QStringLiteral(":/icons/footer/wallet.svg")).pixmap(16, 16));
    m_walletConnectionIconLabel->setToolTip(tr("Wallet daemon connection"));
    m_walletConnectionLabel->setObjectName("m_walletConnectionLabel");
    m_bytecoindConnectionIconLabel->setPixmap(QIcon(QStringLiteral(":/icons/footer/node.svg")).pixmap(16, 16));
    m_bytecoindConnectionIconLabel->setToolTip(tr("Blockchain node connection"));
    for (QLabel* iconLabel : {m_syncStatusIconLabel, m_peerCountIconLabel, m_walletConnectionIconLabel, m_bytecoindConnectionIconLabel})
    {
        iconLabel->setFixedSize(18, 18);
        iconLabel->setAlignment(Qt::AlignCenter);
    }

    addWidget(m_syncStatusIconLabel);
    addWidget(m_syncStatusLabel);
    addPermanentWidget(m_peerCountIconLabel);
    addPermanentWidget(m_peerCountLabel);
    addPermanentWidget(m_walletConnectionIconLabel);
    addPermanentWidget(m_walletConnectionLabel);
    addPermanentWidget(m_bytecoindConnectionIconLabel);
    addPermanentWidget(m_bytecoindConnectionLabel);
//    addPermanentWidget(m_encryptionStatusIconLabel);
//    addPermanentWidget(m_hdStatusLabel);

    updateTimer_->setInterval(30*1000); // 30 secs
    connect(updateTimer_, &QTimer::timeout, this, &WalletStatusBar::updateStatusDescription);
    updateTimer_->start();
    m_syncAnimationTimer->setInterval(80);
    connect(m_syncAnimationTimer, &QTimer::timeout, this, &WalletStatusBar::animateSyncIcon);

//    setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(STATUS_BAR_STYLE_SHEET_TEMPLATE));
    setStyleSheet(STATUS_BAR_STYLE_SHEET_TEMPLATE);
}

WalletStatusBar::~WalletStatusBar()
{}

void WalletStatusBar::setWalletModel(WalletModel* model)
{
    Q_ASSERT(model != nullptr);
    const bool needToDisconnect = walletModel_ != nullptr;
    if (needToDisconnect)
        disconnect(walletModel_, 0, this, 0); // disconnect all signals from walletModel_ to all our slots
    walletModel_ = model;
    stateMapper_->setModel(walletModel_); // clears all previously set mappings
    stateMapper_->addMapping(m_peerCountLabel, WalletModel::COLUMN_PEER_COUNT_SUM, "text");
    stateMapper_->addMapping(m_walletConnectionLabel, WalletModel::COLUMN_STATE, "text");
    stateMapper_->addMapping(m_bytecoindConnectionLabel, WalletModel::COLUMN_LOWER_LEVEL_ERROR, "text");
//    stateMapper_->addMapping(m_hdStatusLabel, WalletModel::COLUMN_DETERMINISTIC, "text");
    stateMapper_->toFirst();
    connect(walletModel_, &QAbstractItemModel::modelReset, stateMapper_, &QDataWidgetMapper::toFirst);
    connect(walletModel_, &QAbstractItemModel::dataChanged, this, &WalletStatusBar::nodeStateChanged);
    updateStatusDescription();
}

void WalletStatusBar::nodeStateChanged(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/, const QVector<int>& roles)
{
    if (roles.contains(WalletModel::ROLE_TXPOOL_VERSION) ||
        roles.contains(WalletModel::ROLE_TOP_BLOCK_HASH) ||
        roles.contains(WalletModel::ROLE_PEER_COUNT_SUM) ||
        roles.contains(WalletModel::ROLE_LOWER_LEVEL_ERROR))
            updateStatusDescription();
}

void WalletStatusBar::updateStatusDescription()
{
    Q_ASSERT(walletModel_ != nullptr);
    const quint32 knownBlockHeight = walletModel_->getKnownBlockHeight();
//    const quint32 lastBlockHeight = walletModel_->getLastBlockHeight();
//    const QDateTime lastBlockTimestampReceived = walletModel_->getLastBlockTimestamp();
//    const quint32 peerCount = walletModel_->getPeerCountSum();
//    const QString lowerLevelError = walletModel_->getLowerLevelError();

//    const QDateTime currentDateTime = QDateTime::currentDateTimeUtc();

//    const QDateTime lastBlockTimestamp = qMin(lastBlockTimestampReceived, currentDateTime);
//    const quint64 msecsSinceLastBlock = lastBlockTimestamp.msecsTo(currentDateTime);
//    const quint64 secsSinceLastBlock = lastBlockTimestamp.secsTo(currentDateTime);

    const bool isThereAnyBlock = walletModel_->isThereAnyBlock();
    const quint32 peerCount = walletModel_->getPeerCountSum();
    const QString formattedTimeDiff = isThereAnyBlock ? walletModel_->getFormattedTimeSinceLastBlock() : tr("unknown");
    const QString blockchainAge = isThereAnyBlock ? tr("%1 ago").arg(formattedTimeDiff) : tr("%1").arg(formattedTimeDiff);

    const quint32 blocksLeft = walletModel_->getBlocksLeftToSync();
    isSynchronized_ = walletModel_->isSyncronized();
    updateSyncState();

    QString warningString;
    if (walletModel_->getSyncStatus() == WalletModel::SyncStatus::LAGGED)
        warningString.append(tr(" Warning: the wallet is lagged."));

    if (peerCount == 0)
        warningString.append(tr(" No network connection."));

    const QString statusText = isSynchronized_ ?
                tr("Wallet synchronized. Top block height: %1  /  Received: %2 ago.%3")
                    .arg(knownBlockHeight)
                    .arg(formattedTimeDiff)
                    .arg(warningString) :
                tr("Synchronization: %1 blocks left (%2).")
                    .arg(blocksLeft)
                    .arg(blockchainAge);

    m_syncStatusLabel->setText(statusText);
}

void WalletStatusBar::updateSyncState()
{
    if (isSynchronized_)
    {
        m_syncAnimationTimer->stop();
        m_syncStatusIconLabel->setPixmap(QIcon(QStringLiteral(":/icons/footer/synced.svg")).pixmap(16, 16));
        m_syncStatusIconLabel->setToolTip(tr("Wallet synchronized"));
    }
    else
    {
        m_syncStatusIconLabel->setToolTip(tr("Synchronizing blocks"));
        if (!m_syncAnimationTimer->isActive())
        {
            m_syncAngle = 0;
            m_syncAnimationTimer->start();
            animateSyncIcon();
        }
    }
}

void WalletStatusBar::animateSyncIcon()
{
    QPixmap rotatedPixmap(18, 18);
    rotatedPixmap.fill(Qt::transparent);
    QPainter painter(&rotatedPixmap);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.translate(rotatedPixmap.width() / 2, rotatedPixmap.height() / 2);
    painter.rotate(m_syncAngle);
    painter.translate(-m_syncIconPixmap.width() / 2, -m_syncIconPixmap.height() / 2);
    painter.drawPixmap(0, 0, m_syncIconPixmap);
    m_syncStatusIconLabel->setPixmap(rotatedPixmap);
    m_syncAngle = (m_syncAngle + 12) % 360;
}

void WalletStatusBar::updateEncryptedState(bool /*isEncrypted*/)
{
}

}
