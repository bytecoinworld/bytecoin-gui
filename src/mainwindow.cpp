// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <cstring>

#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDataWidgetMapper>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QMetaMethod>
#include <QMovie>
#include <QSessionManager>
#include <QUrlQuery>
#include <QDesktopWidget>
#include <QToolTip>
#include <QStyle>
#include <QMap>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "mainwindow.h"

#include "logger.h"
#include "aboutdialog.h"
#include "walletmodel.h"
#include "settings.h"
#include "common.h"
#include "JsonRpc/JsonRpcClient.h"
#include "logframe.h"
#include "addressbookmanager.h"
#include "addressbookmodel.h"
#include "addressbooksortedmodel.h"
#include "sendconfirmationdialog.h"
#include "askpassworddialog.h"
#include "popup.h"

#include "ui_mainwindow.h"

namespace WalletGUI {

namespace {

//const int MAX_RECENT_WALLET_COUNT = 10;
const char COMMUNITY_FORUM_URL[] = "https://t.me/bytecoin_world";
const char REPORT_ISSUE_URL[] = "https://bytecoin.org/contact";
const char DOWNLOAD_URL[] = "https://github.com/bytecoinworld/bytecoin-gui/releases";

//        "QPushButton {border: none;}"
//        "QPushButton:checked {background-color: %1; color: #FFFFFF}";

const char WINDOW_MAIN_ICON_PATH[] = ":images/bytecoin";
const char WINDOW_STAGE_ICON_PATH[] = ":images/bytecoin_stage";
const char WINDOW_TEST_ICON_PATH[] = ":images/bytecoin_test";

const char LOGO_LABEL_MAIN_ICON_PATH[] = ":icons/dark/logo";
const char LOGO_LABEL_STAGE_ICON_PATH[] = ":icons/logo_stage";
const char LOGO_LABEL_TEST_ICON_PATH[] = ":icons/logo_test";

#ifdef Q_OS_WIN
void applyWindowsDarkTitleBar(QWidget* window)
{
    using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwmApi = LoadLibraryW(L"dwmapi.dll");
    if (dwmApi == nullptr)
        return;

    const auto setWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
            GetProcAddress(dwmApi, "DwmSetWindowAttribute"));
    if (setWindowAttribute != nullptr)
    {
        const BOOL useDarkMode = TRUE;
        const HWND handle = reinterpret_cast<HWND>(window->winId());
        if (FAILED(setWindowAttribute(handle, 20, &useDarkMode, sizeof(useDarkMode))))
            setWindowAttribute(handle, 19, &useDarkMode, sizeof(useDarkMode));
    }

    FreeLibrary(dwmApi);
}
#endif

QIcon navigationIcon(const QString& name)
{
    const auto drawIcon = [&name](const QColor& color)
    {
        QPixmap pixmap(48, 48);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(2.0, 2.0);
        painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        if (name == QStringLiteral("overview"))
        {
            painter.drawRoundedRect(QRectF(3.5, 3.5, 7, 7), 1.5, 1.5);
            painter.drawRoundedRect(QRectF(13.5, 3.5, 7, 7), 1.5, 1.5);
            painter.drawRoundedRect(QRectF(3.5, 13.5, 7, 7), 1.5, 1.5);
            painter.drawRoundedRect(QRectF(13.5, 13.5, 7, 7), 1.5, 1.5);
        }
        else if (name == QStringLiteral("addresses"))
        {
            painter.drawRoundedRect(QRectF(3.5, 3.5, 17, 17), 2.5, 2.5);
            painter.drawEllipse(QPointF(9, 9), 2, 2);
            painter.drawArc(QRectF(5.5, 12, 7, 6), 0, 180 * 16);
            painter.drawLine(QPointF(14, 8), QPointF(18, 8));
            painter.drawLine(QPointF(14, 12), QPointF(18, 12));
            painter.drawLine(QPointF(14, 16), QPointF(18, 16));
        }
        else if (name == QStringLiteral("send"))
        {
            QPainterPath path;
            path.moveTo(3.5, 12);
            path.lineTo(20, 4);
            path.lineTo(15, 20);
            path.lineTo(11.5, 13.5);
            path.closeSubpath();
            painter.drawPath(path);
            painter.drawLine(QPointF(11.5, 13.5), QPointF(20, 4));
        }
        else if (name == QStringLiteral("contacts"))
        {
            painter.drawEllipse(QPointF(12, 8), 3.1, 3.1);
            painter.drawArc(QRectF(5, 12, 14, 10), 0, 180 * 16);
            painter.drawArc(QRectF(2, 5, 7, 10), 70 * 16, 150 * 16);
            painter.drawArc(QRectF(15, 5, 7, 10), -40 * 16, 150 * 16);
        }
        else
        {
            QPainterPath terminal;
            terminal.moveTo(4, 6);
            terminal.lineTo(10, 12);
            terminal.lineTo(4, 18);
            painter.drawPath(terminal);
            painter.drawLine(QPointF(12.5, 18), QPointF(20, 18));
        }

        return pixmap;
    };

    QIcon icon;
    icon.addPixmap(drawIcon(QColor("#A7A2B0")), QIcon::Normal, QIcon::Off);
    icon.addPixmap(drawIcon(QColor("#FF6AA8")), QIcon::Normal, QIcon::On);
    icon.addPixmap(drawIcon(QColor("#686572")), QIcon::Disabled, QIcon::Off);
    icon.addPixmap(drawIcon(QColor("#686572")), QIcon::Disabled, QIcon::On);
    return icon;
}

}

MainWindow::MainWindow(
        WalletModel* walletModel,
        const QString& styleSheetTemplate,
        AddressBookManager* addressBookManager,
        IAddressBookManager* myAddressesManager,
        QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_styleSheetTemplate(styleSheetTemplate)
    , m_addressesMapper(new QDataWidgetMapper(this))
    , m_balanceMapper(new QDataWidgetMapper(this))
    , m_syncMovie(new QMovie(QString(":icons/dark/wallet-sync"), QByteArray(), this))
    , addressBookManager_(addressBookManager)
    , myAddressesManager_(myAddressesManager)
    , copiedToolTip_(new CopiedToolTip(this))
    , walletModel_(walletModel)
    , netColor_(MAIN_NET_COLOR)
{
    m_ui->setupUi(this);

#ifdef Q_OS_WIN
    applyWindowsDarkTitleBar(this);
#endif

    m_ui->m_overviewButton->setIcon(navigationIcon(QStringLiteral("overview")));
    m_ui->m_myAddressesButton->setIcon(navigationIcon(QStringLiteral("addresses")));
    m_ui->m_sendButton->setIcon(navigationIcon(QStringLiteral("send")));
    m_ui->m_addressBookButton->setIcon(navigationIcon(QStringLiteral("contacts")));
    m_ui->m_logButton->setIcon(navigationIcon(QStringLiteral("console")));
    const QSize navigationIconSize(20, 20);
    m_ui->m_overviewButton->setIconSize(navigationIconSize);
    m_ui->m_myAddressesButton->setIconSize(navigationIconSize);
    m_ui->m_sendButton->setIconSize(navigationIconSize);
    m_ui->m_addressBookButton->setIconSize(navigationIconSize);
    m_ui->m_logButton->setIconSize(navigationIconSize);

    themeChanged();

    m_ui->m_overviewButton->setToolTip(m_ui->m_overviewButton->text());
    m_ui->m_sendButton->setToolTip(m_ui->m_sendButton->text());
    m_ui->m_addressBookButton->setToolTip(m_ui->m_addressBookButton->text());
    m_ui->m_myAddressesButton->setToolTip(m_ui->m_myAddressesButton->text());
    m_ui->m_logButton->setToolTip(m_ui->m_logButton->text());

    m_ui->m_updateLabel->setText("");
    m_ui->m_updateLabel->hide();
    m_ui->m_topBlockLabel->setText("");
    m_ui->m_topBlockLabel->hide();

//    m_ui->m_viewOnlyLabel->setText("");
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->size(), qApp->desktop()->availableGeometry()));
//    setWindowIcon(QIcon(WINDOW_MAIN_ICON_PATH));
    clearTitle();

    netChanged(RpcApi::MAIN_NET_NAME);
//    m_ui->m_overviewButton->setStyleSheet(QString{BUTTON_STYLE_SHEET}.arg(MAIN_NET_COLOR));
//    m_ui->m_sendButton->setStyleSheet(QString{BUTTON_STYLE_SHEET}.arg(MAIN_NET_COLOR));
//    m_ui->m_addressBookButton->setStyleSheet(QString{BUTTON_STYLE_SHEET}.arg(MAIN_NET_COLOR));
//    m_ui->m_logButton->setStyleSheet(QString{BUTTON_STYLE_SHEET}.arg(MAIN_NET_COLOR));

    m_ui->m_walletViewOnlyLabel->hide();
    m_addressBookModel = new AddressBookModel(addressBookManager_, this);
    m_sortedAddressBookModel = new SortedAddressBookModel(m_addressBookModel, this);
    m_myAddressesModel = new AddressBookModel(myAddressesManager_, this);
    m_ui->m_overviewFrame->setMainWindow(this);
    m_ui->m_overviewFrame->setTransactionsModel(walletModel_);
    m_ui->m_overviewFrame->hide();
    m_ui->m_walletFrame->show();

//    m_ui->m_addressLabel->installEventFilter(this);
    m_ui->m_copyWalletAddressLabel->installEventFilter(this);

//    m_ui->m_balanceIconLabel->setPixmap(QPixmap(QString(":icons/light/balance")));
//    m_ui->m_logoLabel->setPixmap(QPixmap(QString(LOGO_LABEL_MAIN_ICON_PATH)));
    m_ui->statusBar->setWalletModel(walletModel_);
    m_ui->m_syncProgress->setWalletModel(walletModel_);

//    m_ui->m_addressesCountLabel->hide();
//    m_ui->m_creationTimestampLabel->hide();
    m_addressesMapper->setModel(walletModel_);
    m_addressesMapper->addMapping(m_ui->m_addressLabel, WalletModel::COLUMN_FIRST_ADDRESS, "text");
//    m_addressesMapper->addMapping(m_ui->m_addressesCountLabel, WalletModel::COLUMN_TOTAL_ADDRESS_COUNT, "text");
//    m_addressesMapper->addMapping(m_ui->m_creationTimestampLabel, WalletModel::COLUMN_WALLET_CREATION_TIMESTAMP, "text");
//    m_addressesMapper->addMapping(m_ui->m_viewOnlyLabel, WalletModel::COLUMN_VIEW_ONLY, "text");
    m_addressesMapper->toFirst();
    connect(walletModel_, &QAbstractItemModel::modelReset, m_addressesMapper, &QDataWidgetMapper::toFirst);

    m_balanceMapper->setModel(walletModel_);
    m_balanceMapper->addMapping(m_ui->m_balanceLabel, WalletModel::COLUMN_TOTAL, "text");
    m_balanceMapper->toFirst();
    connect(walletModel_, &QAbstractItemModel::modelReset, m_balanceMapper, &QDataWidgetMapper::toFirst);

//    QFont font = m_ui->m_balanceLabel->font();
//    font.setBold(true);
//    m_ui->m_balanceLabel->setFont(font);

    connect(m_ui->m_exitAction, &QAction::triggered, qApp, &QApplication::quit);
    connect(m_ui->m_bytecoinWorldSiteAction, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://bytecoin.world/")));
    });
    connect(m_ui->m_bytecoinWorldExplorerAction, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://explorer.bytecoin.world/")));
    });
    connect(m_ui->m_bytecoinWorldReleasesAction, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromUserInput(DOWNLOAD_URL));
    });

    m_ui->m_sendFrame->setWalletModel(walletModel_);
    m_ui->m_sendFrame->setMainWindow(this);
    m_ui->m_sendFrame->setAddressBookModel(m_sortedAddressBookModel);
    m_ui->m_sendFrame->setAddressBookManager(addressBookManager_);
    m_ui->m_sendFrame->hide();
    connect(m_ui->m_sendFrame, &SendFrame::createTxSignal, this , &MainWindow::createTx);
    m_ui->m_overviewFrame->setWalletModel(walletModel_);
    m_ui->m_overviewFrame->hide();
    m_ui->m_addressBookFrame->setMainWindow(this);
    m_ui->m_addressBookFrame->setAddressBookManager(addressBookManager_);
    m_ui->m_addressBookFrame->setSortedAddressBookModel(m_sortedAddressBookModel);
    m_ui->m_addressBookFrame->hide();
    m_ui->m_myAddressesFrame->setMainWindow(this);
    m_ui->m_myAddressesFrame->setWalletModel(walletModel_);
    m_ui->m_myAddressesFrame->setAddressBookManager(myAddressesManager_);
    m_ui->m_myAddressesFrame->setSortedAddressBookModel(m_myAddressesModel);
    m_ui->m_myAddressesFrame->hide();

    m_ui->m_sendButton->setEnabled(false);
    m_ui->m_overviewButton->setEnabled(false);
    m_ui->m_checkProofAction->setEnabled(false);
    m_ui->m_changePasswordAction->setEnabled(false);
    m_ui->m_exportKeysAction->setEnabled(false);
    m_ui->m_exportViewOnlyKeysAction->setEnabled(false);

    m_ui->m_logButton->setChecked(true);
    m_ui->m_logFrame->show();

}

MainWindow::~MainWindow()
{}

void MainWindow::netChanged(const QString& net)
{
    if (net == RpcApi::MAIN_NET_NAME)
    {
        netColor_ = MAIN_NET_COLOR;
        setWindowIcon(QIcon(WINDOW_MAIN_ICON_PATH));
        m_ui->m_logoLabel->setPixmap(QPixmap(QString(LOGO_LABEL_MAIN_ICON_PATH)));
//        m_ui->m_createNewWalletAction->setEnabled(false);
//        m_ui->m_restoreWalletFromMnemonicAction->setEnabled(false);
    }
    else if (net == RpcApi::STAGE_NET_NAME)
    {
        netColor_ = STAGE_NET_COLOR;
        setWindowIcon(QIcon(WINDOW_STAGE_ICON_PATH));
        m_ui->m_logoLabel->setPixmap(QPixmap(QString(LOGO_LABEL_STAGE_ICON_PATH)));
//        m_ui->m_createNewWalletAction->setEnabled(true);
//        m_ui->m_restoreWalletFromMnemonicAction->setEnabled(true);
    }
    else if (net == RpcApi::TEST_NET_NAME)
    {
        netColor_ = TEST_NET_COLOR;
        setWindowIcon(QIcon(WINDOW_TEST_ICON_PATH));
        m_ui->m_logoLabel->setPixmap(QPixmap(QString(LOGO_LABEL_TEST_ICON_PATH)));
//        m_ui->m_createNewWalletAction->setEnabled(true);
//        m_ui->m_restoreWalletFromMnemonicAction->setEnabled(true);
    }
    m_ui->m_syncProgress->setColor(netColor_);
}

void MainWindow::themeChanged()
{
    const QMap<QString, QString> colors{
        {"%addressFontColor%", "#E8D9E1"},
        {"%backgroundColorBlueHover%", "#C43D76"},
        {"%backgroundColorBlueNormal%", "#A9275B"},
        {"%backgroundColorBluePressed%", "#7E1D45"},
        {"%backgroundColorGray%", "#0B0C12"},
        {"%balanceFontColor%", "#FFF9FC"},
        {"%borderColor%", "#292632"},
        {"%borderColorDark%", "#3D3948"},
        {"%fontColorBlueHover%", "#F08DB5"},
        {"%fontColorGray%", "#C0BBC7"},
        {"%surfaceColor%", "#171620"},
        {"%elevatedSurfaceColor%", "#201F2B"},
        {"%surfaceHoverColor%", "#2A2937"},
        {"%textColor%", "#F5F3F7"},
        {"%mutedTextColor%", "#B8B4C0"},
        {"%disabledTextColor%", "#77727F"},
        {"%headerBackgroundColor%", "#101018"},
        {"%headerBorderColor%", "#2D2936"},
        {"%selectionColor%", "#873051"},
        {"%statusBarBackgroundColor%", "#0E0E15"},
        {"%toolBarBorderColor%", "#292632"},
        {"%toolButtonBackgroundColorHover%", "#191923"},
        {"%toolButtonBackgroundColorNormal%", "#0E0E15"},
        {"%toolButtonBackgroundColorPressed%", "#281923"},
        {"%toolButtonFontColorDisabled%", "#77727F"},
        {"%toolButtonFontColorNormal%", "#D0CBD5"},
        {"%scrollTrackColor%", "#101018"},
        {"%warningSurfaceColor%", "#30251B"}
    };

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#0B0C12"));
    palette.setColor(QPalette::WindowText, QColor("#F5F3F7"));
    palette.setColor(QPalette::Base, QColor("#12121A"));
    palette.setColor(QPalette::AlternateBase, QColor("#191923"));
    palette.setColor(QPalette::ToolTipBase, QColor("#191923"));
    palette.setColor(QPalette::ToolTipText, QColor("#F5F3F7"));
    palette.setColor(QPalette::Text, QColor("#F5F3F7"));
    palette.setColor(QPalette::Button, QColor("#191923"));
    palette.setColor(QPalette::ButtonText, QColor("#F5F3F7"));
    palette.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Highlight, QColor("#873051"));
    palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    palette.setColor(QPalette::Link, QColor("#F08DB5"));
    palette.setColor(QPalette::LinkVisited, QColor("#D783A7"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#77727F"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#77727F"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#77727F"));
    qApp->setPalette(palette);

    QString styleSheet = m_styleSheetTemplate;
    for (auto it = colors.cbegin(); it != colors.cend(); ++it)
        styleSheet.replace(it.key(), it.value());

    styleSheet.replace("%fontFamily%", "Work Sans");
    styleSheet.replace("%fontSizeTiny%", "9pt");
    styleSheet.replace("%fontSizeSmall%", "10pt");
    styleSheet.replace("%fontSizeNormal%", "11pt");
    styleSheet.replace("%fontSizeLarge%", "13pt");
    styleSheet.replace("%fontSizeHuge%", "21pt");
    styleSheet.replace("%fontSizeDisplay%", "25pt");
    styleSheet.replace("%currentTheme%", "dark");
    qApp->setStyleSheet(styleSheet);

    m_ui->m_syncProgress->setColor(netColor_);
    m_syncMovie->setFileName(":icons/dark/wallet-sync");
    m_syncMovie->setScaledSize(QSize(16, 16));
    m_syncMovie->stop();
    m_ui->m_toolFrame->setProperty("compact", width() < 960);
    m_ui->m_toolFrame->style()->unpolish(m_ui->m_toolFrame);
    m_ui->m_toolFrame->style()->polish(m_ui->m_toolFrame);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    const bool compact = event->size().width() < 960;
    m_ui->m_toolFrame->setProperty("compact", compact);
    m_ui->m_toolFrame->setMinimumWidth(compact ? 76 : 173);
    m_ui->m_toolFrame->setMaximumWidth(compact ? 76 : 173);

    const QList<QPushButton*> navigationButtons = {
        m_ui->m_overviewButton,
        m_ui->m_sendButton,
        m_ui->m_addressBookButton,
        m_ui->m_myAddressesButton,
        m_ui->m_logButton
    };
    for (QPushButton* button : navigationButtons)
    {
        button->setText(compact ? QString{} : button->toolTip());
        button->setIconSize(compact ? QSize(24, 24) : QSize(20, 20));
    }

    m_ui->m_toolFrame->style()->unpolish(m_ui->m_toolFrame);
    m_ui->m_toolFrame->style()->polish(m_ui->m_toolFrame);
}

void MainWindow::statusChanged()
{
    const bool isThereAnyBlock = walletModel_->isThereAnyBlock();
    const QString formattedTimeDiff = isThereAnyBlock ? walletModel_->getFormattedTimeSinceLastBlock() : tr("unknown");
    const QString blockchainAge = isThereAnyBlock ? tr("%1 ago").arg(formattedTimeDiff) : tr("%1").arg(formattedTimeDiff);
    m_ui->m_topBlockLabel->setText(tr("Top block: %1").arg(blockchainAge));

    if (!walletModel_->isConnected())
        return;

    const WalletModel::SyncStatus status = walletModel_->getSyncStatus();
    switch(status)
    {
    case WalletModel::SyncStatus::NOT_SYNCED:
        m_ui->m_walletStatusPicLabel->setPixmap(QPixmap{QString{":/icons/not_synced"}});
        m_ui->m_walletStatusLabel->setText(tr("Wallet out of sync"));
        break;
    case WalletModel::SyncStatus::LAGGED:
        m_ui->m_walletStatusPicLabel->setPixmap(QPixmap{QString{":/icons/sync_lag"}});
        m_ui->m_walletStatusLabel->setText(tr("Wallet lagging"));
        break;
    case WalletModel::SyncStatus::SYNCED:
        m_ui->m_walletStatusPicLabel->setPixmap(QPixmap{QString{":/icons/synced"}});
        m_ui->m_walletStatusLabel->setText(tr("Wallet synchronized"));
        break;
    }


    m_ui->m_balanceLabelText->setVisible(true);
    m_ui->m_balanceLabel->setVisible(true);
    m_ui->m_addressLabelText->setVisible(true);
    m_ui->m_addressLabel->setVisible(true);
    m_ui->m_copyWalletAddressLabel->setVisible(true);
    m_ui->m_walletStatusPicLabel->setVisible(true);
    m_ui->m_walletStatusLabel->setVisible(true);
    m_ui->m_topBlockLabel->setVisible(true);
}

QString MainWindow::getAddress() const
{
    Q_ASSERT(walletModel_ != nullptr);
    return walletModel_->getAddress();
}

void MainWindow::addRecipient(const QString& address, const QString& label)
{
    if (walletModel_->isViewOnly())
        return;
    m_ui->m_sendFrame->addRecipient(address, label);
    m_ui->m_sendButton->click();
}

void MainWindow::createTx(const RpcApi::Transaction& tx, quint64 fee, bool subtractFee)
{
    RpcApi::CreateTransaction::Request req;
    req.any_spend_address = true;
    req.transaction = tx;
    req.change_address = getAddress();
    req.confirmed_height_or_depth = -static_cast<qint32>(CONFIRMATIONS) - 1;
    req.fee_per_byte = fee;
    req.save_history = true;
    req.subtract_fee_from_amount = subtractFee;

    emit createTxSignal(req, QPrivateSignal{});
}

void MainWindow::createTxReceived(const RpcApi::CreatedTx& tx)
{
    showSendConfirmation(tx);
}

void MainWindow::showSendConfirmation(const RpcApi::CreatedTx& tx)
{
//    qDebug("raw tx = %s", qPrintable(tx.binary_transaction));

    QStringList summary;
    summary << tr("Review this payment before sending:");
    qint64 ourAmount = 0;
    for (const RpcApi::Transfer& tf: tx.transaction.transfers)
    {
        if (tf.ours)
        {
            ourAmount += tf.amount;
            continue;
        }
        const QString& amountStr = formatAmount(tf.amount);
        const QString& addressStr = tf.address;
        summary << tr("Recipient: %1").arg(addressStr);
        summary << tr("Amount: %1").arg(amountStr);
    }
    summary << tr("Network fee: %1").arg(formatAmount(tx.transaction.fee));
    summary << tr("Total debited: %1").arg(formatAmount(-ourAmount));

    SendConfirmationDialog dlg(
                tr("Confirm send coins"),
                summary.join(QStringLiteral("\n\n")),
                5,
                this);
    dlg.exec();
    QMessageBox::StandardButton button = static_cast<QMessageBox::StandardButton>(dlg.result());
    if(button != QMessageBox::Yes)
    {
        m_ui->m_sendFrame->cancelSend();
        return;
    }

    if (Settings::instance().getWalletdConnectionMethod() == ConnectionMethod::REMOTE)
    {
        QMessageBox::warning(this, tr("Password verification unavailable"),
            tr("Password verification is disabled for remote walletd connections. Connect to a local walletd to send coins."));
        m_ui->m_sendFrame->cancelSend();
        return;
    }

    pendingSendTransaction_ = tx.binary_transaction;
    requestWalletPassword();
}

void MainWindow::requestWalletPassword()
{
    if (pendingSendTransaction_.isEmpty())
        return;

    AskPasswordDialog dialog(false, this);
    dialog.setWindowTitle(tr("Confirm transaction"));
    if (dialog.exec() != QDialog::Accepted)
    {
        clearPendingSend();
        return;
    }

    RpcApi::VerifyWalletPassword::Request request;
    request.wallet_password = dialog.getPassword();
    m_ui->m_sendFrame->setEnabled(false);
    emit verifyWalletPasswordSignal(request, QPrivateSignal{});
    request.wallet_password.fill(QChar(0));
}

void MainWindow::walletPasswordVerified(const RpcApi::VerifyWalletPassword::Response& result)
{
    if (pendingSendTransaction_.isEmpty())
        return;

    if (!result.supported)
    {
        QMessageBox::warning(this, tr("Password verification unavailable"),
            tr("This wallet type does not support wallet-password verification. For safety, the transaction was not sent."));
        clearPendingSend();
        return;
    }

    if (!result.valid)
    {
        const QMessageBox::StandardButton button = QMessageBox::warning(
            this, tr("Incorrect wallet password"),
            tr("The wallet password is incorrect. Would you like to enter it again?"),
            QMessageBox::Retry | QMessageBox::Cancel, QMessageBox::Retry);
        if (button == QMessageBox::Retry)
        {
            requestWalletPassword();
            return;
        }
        clearPendingSend();
        return;
    }

    const QString binaryTransaction = pendingSendTransaction_;
    clearPendingSend();
    emit sendTxSignal(RpcApi::SendTransaction::Request{binaryTransaction}, QPrivateSignal{});
    m_ui->m_sendFrame->clearAll();
    m_ui->m_overviewButton->click();
}

void MainWindow::walletPasswordVerificationFailed(const QString& error)
{
    if (pendingSendTransaction_.isEmpty())
        return;

    QMessageBox::warning(this, tr("Password verification unavailable"),
        tr("The wallet daemon could not verify the password. For safety, the transaction was not sent.\n\n%1").arg(error));
    clearPendingSend();
}

void MainWindow::clearPendingSend()
{
    pendingSendTransaction_.fill(QChar(0));
    pendingSendTransaction_.clear();
    m_ui->m_sendFrame->setEnabled(true);
    m_ui->m_sendFrame->cancelSend();
}

bool MainWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == m_ui->m_copyWalletAddressLabel && event->type() == QEvent::MouseButtonRelease)
        copyAddress();
//    else if (object == m_ui->m_balanceLabel && event->type() == QEvent::MouseButtonRelease)
//        copyBalance();
    return false;
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    switch (event->type())
    {
    case QEvent::WindowStateChange:
        if(isMinimized() && false /*&& Settings::instance().isMinimizeToTrayEnabled()*/ )
            hide();
        break;
    default:
        break;
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
//#ifndef Q_OS_MAC
//    if (!Settings::instance().isCloseToTrayEnabled())
        m_ui->m_exitAction->trigger();
//#endif
    QMainWindow::closeEvent(event);
}

void MainWindow::aboutQt() {
  QMessageBox::aboutQt(this);
}

void MainWindow::about() {
  AboutDialog dlg(this);
  dlg.exec();
}

void MainWindow::copyAddress()
{
    if (!walletModel_->isConnected())
        return;
    QApplication::clipboard()->setText(walletModel_->index(0, WalletModel::COLUMN_FIRST_ADDRESS).data(WalletModel::ROLE_FIRST_ADDRESS).toString());
    copiedToClipboard();
}

void MainWindow::copyBalance()
{
    QString balanceString = walletModel_->index(0, WalletModel::COLUMN_TOTAL).data().toString();
    balanceString.remove(',');
    QApplication::clipboard()->setText(balanceString);
    copiedToClipboard();
}

void MainWindow::createLegacyWallet()
{
    emit createLegacyWalletSignal(this);
}

void MainWindow::createWallet()
{
    emit createWalletSignal(this);
}

void MainWindow::createHWWallet()
{
    emit createHWWalletSignal(this);
}

void MainWindow::openWallet()
{
    emit openWalletSignal(this);
}

void MainWindow::restoreWalletFromMnemonic()
{
    emit restoreWalletFromMnemonicSignal(this);
}

void MainWindow::remoteWallet()
{
    emit remoteWalletSignal(this);
}

void MainWindow::encryptWallet()
{
    emit encryptWalletSignal(this);
}

void MainWindow::importKeys()
{
    emit importKeysSignal(this);
}

void MainWindow::communityForumTriggered()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(COMMUNITY_FORUM_URL));
}

void MainWindow::reportIssueTriggered()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(REPORT_ISSUE_URL));
}

void MainWindow::splashMsg(const QString& msg)
{
    m_ui->m_logFrame->addGuiMessage(QString("\n\n\n\n\n") + msg + '\n');
    m_ui->m_walletStatusLabel->setText(msg);
    m_ui->m_walletStatusPicLabel->setVisible(false);
//    m_ui->m_addressLabel->setText(msg);
//    m_ui->m_addressesCountLabel->setText("");
//    m_ui->m_creationTimestampLabel->setText("");
    showLog();
}

void MainWindow::addDaemonOutput(const QString& msg)
{
    m_ui->m_logFrame->addDaemonOutput(msg);
}

void MainWindow::addDaemonError(const QString& msg)
{
    m_ui->m_logFrame->addDaemonError(msg);
}

void MainWindow::showLog()
{
    m_ui->m_logButton->setChecked(true);
//    activateWindow();
//    raise();
}

void MainWindow::setTitle()
{
    clearTitle();
#ifdef Q_OS_MAC
    if (Settings::instance().getWalletdConnectionMethod() == ConnectionMethod::BUILTIN)
        setWindowFilePath(Settings::instance().getWalletFile());
    else
        setWindowTitle(Settings::instance().getWalletdEndPoint());
#else
    const QString fileName =
            Settings::instance().getWalletdConnectionMethod() == ConnectionMethod::BUILTIN ?
                Settings::instance().getWalletFile() :
                Settings::instance().getWalletdEndPoint();

    setWindowTitle(fileName);
#endif
}

void MainWindow::clearTitle()
{
#ifdef Q_OS_MAC
    setWindowFilePath(QString{});
    setWindowTitle(QString{});
#else
    setWindowTitle(QString{});
#endif
}

void MainWindow::setConnectedState()
{
    const bool viewOnly = walletModel_->isViewOnly();
    m_ui->m_walletViewOnlyLabel->setVisible(viewOnly);

    if (!viewOnly)
        m_ui->m_sendButton->setEnabled(true);
    m_ui->m_overviewButton->setEnabled(true);
    m_ui->m_addressBookButton->setEnabled(true);
    m_ui->m_myAddressesButton->setEnabled(true);
    m_ui->m_checkProofAction->setEnabled(true);
    if (m_ui->m_logFrame->isVisible())
        m_ui->m_overviewButton->click();

    walletModel_->isAmethyst() ?
                m_ui->m_exportKeysAction->setText(tr("Export mnemonic")) :
                m_ui->m_exportKeysAction->setText(tr("Export keys"));

    setTitle();
}

void MainWindow::setDisconnectedState()
{
    m_ui->m_logFrame->show();
    m_ui->m_logButton->setChecked(true);

    m_ui->m_sendButton->setEnabled(false);
    m_ui->m_overviewButton->setEnabled(false);
    m_ui->m_addressBookButton->setEnabled(false);
    m_ui->m_myAddressesButton->setEnabled(false);

    walletModel_->reset();

    m_ui->m_changePasswordAction->setEnabled(false);
    m_ui->m_checkProofAction->setEnabled(false);
    m_ui->m_exportKeysAction->setEnabled(false);
    m_ui->m_exportViewOnlyKeysAction->setEnabled(false);

    m_ui->m_balanceLabelText->setVisible(false);
    m_ui->m_balanceLabel->setVisible(false);
    m_ui->m_addressLabelText->setVisible(false);
    m_ui->m_addressLabel->setVisible(false);
    m_ui->m_copyWalletAddressLabel->setVisible(false);
    m_ui->m_walletStatusPicLabel->setVisible(false);
    m_ui->m_topBlockLabel->setVisible(false);

    clearTitle();
}

void MainWindow::builtinRun()
{
    m_ui->m_changePasswordAction->setEnabled(true);
    m_ui->m_exportKeysAction->setEnabled(true);
    m_ui->m_exportViewOnlyKeysAction->setEnabled(true);
}

void MainWindow::jsonErrorResponse(const QString& /*id*/, const JsonRpc::Error& error)
{
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Critical);
    msg.setWindowTitle(tr("Error"));
    msg.setText(QString{"(%1) %2 %3"}.arg(error.code).arg(error.message).arg(error.data));
    msg.exec();

    m_ui->m_sendFrame->cancelSend();
}

void MainWindow::copiedToClipboard()
{
    copiedToolTip_->showNearMouse();
}

void MainWindow::openDataFolder()
{
    const QDir dataDir = Settings::instance().getDefaultWorkDir();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dataDir.absolutePath()));
}

void MainWindow::packetSent(const QByteArray& data)
{
    m_ui->m_logFrame->addNetworkMessage(QString("--> ") + QString::fromUtf8(data) + '\n');
}

void MainWindow::packetReceived(const QByteArray& data)
{
    m_ui->m_logFrame->addNetworkMessage(QString("<-- ") + QString::fromUtf8(data) + '\n');
}

void MainWindow::createProof(const QString& txHash, const QStringList& addresses, bool needToFind)
{
    emit createProofSignal(txHash, addresses, needToFind);
}

void MainWindow::checkProof()
{
    emit checkProofSignal();
}

void MainWindow::showWalletdParams()
{
    emit showWalletdParamsSignal();
}

void MainWindow::exportViewOnlyKeys()
{
    emit exportViewOnlyKeysSignal(walletModel_->isAmethyst());
}

void MainWindow::exportKeys()
{
    emit exportKeysSignal(walletModel_->isAmethyst());
}

void MainWindow::updateIsReady(const QString& newVersion)
{
    m_ui->m_updateLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_ui->m_updateLabel->setOpenExternalLinks(true);
    m_ui->m_updateLabel->setText(QString("New version %1 of Bytecoin wallet is available.").arg(QString(QString("<a href=\"") + DOWNLOAD_URL + "\">%1</a>").arg(newVersion)));
    m_ui->m_updateLabel->show();
}

}
