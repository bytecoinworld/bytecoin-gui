// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include "walletdparamsdialog.h"
#include "ui_walletdparamsdialog.h"
#include "settings.h"
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPushButton>
#include <QIcon>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include "common.h"

namespace WalletGUI
{

WalletdParamsDialog::WalletdParamsDialog(bool allowToRestart, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::WalletdParamsDialog)
    , allowToRestart_(allowToRestart)
{
    ui->setupUi(this);
    connect(ui->helpButton, &QPushButton::clicked, this, &WalletdParamsDialog::showHelp);
    connect(ui->applyButton, &QPushButton::clicked, this, &WalletdParamsDialog::applyParams);
    connect(ui->applyButton, &QPushButton::clicked, this, &WalletdParamsDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &WalletdParamsDialog::reject);
    connect(ui->addPublicNodeButton, &QPushButton::clicked, this, &WalletdParamsDialog::addPublicNode);
    connect(ui->browseDataFolderButton, &QPushButton::clicked, this, &WalletdParamsDialog::browseDataFolder);
    setWindowTitle(tr("Wallet daemon parameters"));
    setMinimumSize(940, 680);
    resize(980, 720);
    ui->verticalLayout_2->setStretch(0, 2);
    ui->verticalLayout_2->setStretch(3, 3);
    ui->horizontalLayout_2->setStretch(0, 4);
    ui->horizontalLayout_2->setStretch(1, 1);
//    ui->applyButton->setEnabled(allowToRestart);
    ui->applyButton->setText(allowToRestart_ ? tr("Save and restart walletd") : tr("Save"));
    ui->helpButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/help.svg")));
    ui->applyButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/save.svg")));
    ui->cancelButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/cancel.svg")));
    ui->addPublicNodeButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/add-node.svg")));
    ui->browseDataFolderButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/folder.svg")));
    ui->addPublicNodeButton->setFixedSize(148, 40);
    ui->addPublicNodeButton->setStyleSheet(
        QStringLiteral("QPushButton { padding: 2px 8px; min-height: 0; }"));
    for (QPushButton* button : {ui->helpButton, ui->applyButton, ui->cancelButton,
                                ui->addPublicNodeButton, ui->browseDataFolderButton})
        button->setIconSize(QSize(18, 18));
    ui->paramsEdit->setText(Settings::instance().getWalletdParams());
    ui->walletSyncStartHeightEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9]{0,10}")), ui->walletSyncStartHeightEdit));
    const QRegularExpression startHeightOption(
        QStringLiteral(R"((?:^|\s)--wallet-sync-start-height=(\d+)(?=\s|$))"));
    const QRegularExpressionMatch startHeightMatch =
        startHeightOption.match(ui->paramsEdit->toPlainText().simplified());
    ui->walletSyncStartHeightEdit->setText(
        startHeightMatch.hasMatch() ? startHeightMatch.captured(1) : QStringLiteral("0"));
    connect(ui->walletSyncStartHeightEdit, &QLineEdit::textChanged,
            this, &WalletdParamsDialog::hostChanged);
    ui->walletScanStartHeightWarningLabel->setToolTip(
        tr("The start height is only used when the wallet's synchronization cache is empty. "
           "Changing it does not rescan an existing wallet cache."));
    ui->paramsEdit->setPlaceholderText(
        tr("--seed-node-address=host:port\n--priority-node-address=host:port"));
    ui->paramsEdit->setToolTip(tr("Enter one or more walletd command-line options. Separate options with spaces; put values containing spaces in double quotes."));
    ui->radioBuiltinBytecoind->setToolTip(tr("Use the blockchain node managed by this wallet daemon."));
    ui->radioExternalBytecoind->setToolTip(tr("Use an existing remote bytecoind RPC endpoint instead of starting the built-in node."));
    ui->radioMainNet->setToolTip(tr("Connect to the production Bytecoin network."));
    ui->radioStageNet->setToolTip(tr("Connect to the public staging network; its wallets and chain data are separate from mainnet."));
    ui->radioTestNet->setToolTip(tr("Connect to the test network; use testnet wallet data only."));
    ui->editHost->setToolTip(tr("External bytecoind RPC URL, for example https://node01.bytecoin.org:8091."));
    ui->helpButton->setToolTip(tr("Show common walletd options and examples."));

    const NetworkType net = Settings::instance().getNetworkType();
    switch (net) {
    case NetworkType::MAIN:  ui->radioMainNet->setChecked(true);  break;
    case NetworkType::STAGE: ui->radioStageNet->setChecked(true); break;
    case NetworkType::TEST:  ui->radioTestNet->setChecked(true);  break;
    default: ui->radioMainNet->setChecked(true);  break;
    }

    const ConnectionMethod bytecoindConnection = Settings::instance().getBytecoindConnectionMethod();
    switch (bytecoindConnection) {
    case ConnectionMethod::BUILTIN:  ui->radioBuiltinBytecoind->setChecked(true);  break;
    case ConnectionMethod::REMOTE: ui->radioExternalBytecoind->setChecked(true); break;
    default: ui->radioBuiltinBytecoind->setChecked(true);  break;
    }

    QUrl url{Settings::instance().getBytecoindHost()};
    url.setPort(Settings::instance().getBytecoindPort());
    ui->editHost->setText(url.toString());

    publicNodes_ = Settings::instance().getPublicBytecoindNodes();
    const QString currentNode = ui->editHost->text().trimmed();
    if (!currentNode.isEmpty() && !publicNodes_.contains(currentNode, Qt::CaseInsensitive))
        publicNodes_.prepend(currentNode);
    rebuildPublicNodeList();
    externalBytecoindToggled(ui->radioExternalBytecoind->isChecked());
}

WalletdParamsDialog::~WalletdParamsDialog()
{
    delete ui;
}

void WalletdParamsDialog::saveParams()
{
    QString params = ui->paramsEdit->toPlainText().simplified();
    params.replace(QRegularExpression(QStringLiteral(R"((?:^|\s)--wallet-sync-start-height=\S+)")),
        QString{});
    params = params.simplified();
    const qulonglong startHeight = ui->walletSyncStartHeightEdit->text().toULongLong();
    if (startHeight != 0)
        params += (params.isEmpty() ? QString{} : QStringLiteral(" "))
            + QStringLiteral("--wallet-sync-start-height=") + QString::number(startHeight);
    Settings::instance().setWalletdParams(params);
    Settings::instance().setNetworkType(
                ui->radioMainNet->isChecked() ?
                    NetworkType::MAIN :
                    ui->radioStageNet->isChecked() ?
                        NetworkType::STAGE :
                        ui->radioTestNet->isChecked() ?
                            NetworkType::TEST :
                            NetworkType::MAIN);
    Settings::instance().setBytecoindConnectionMethod(
                ui->radioBuiltinBytecoind->isChecked() ?
                    ConnectionMethod::BUILTIN :
                    ui->radioExternalBytecoind->isChecked() ?
                        ConnectionMethod::REMOTE :
                        ConnectionMethod::BUILTIN);

    const QUrl url{ui->editHost->text()};
    Settings::instance().setBytecoindEndPoint(url.toString(QUrl::RemovePort), url.port(Settings::instance().getDefaultBytecoindPort()));
}

void WalletdParamsDialog::applyParams()
{
    saveParams();
    if (allowToRestart_)
        emit restartWalletd();
}

void WalletdParamsDialog::hostChanged()
{
    const QUrl url = QUrl::fromUserInput(ui->editHost->text().trimmed());
    const bool valid = url.isValid() && isIpOrHostName(url.host())
        && (url.port(-1) == -1 || (url.port() > 0 && url.port() <= 65535));
    const bool external = ui->radioExternalBytecoind->isChecked();
    bool validStartHeight = false;
    const qulonglong startHeight = ui->walletSyncStartHeightEdit->text().toULongLong(&validStartHeight);
    validStartHeight = validStartHeight && startHeight <= 4294967295ULL;
    ui->applyButton->setEnabled((!external || valid) && validStartHeight);
    ui->addPublicNodeButton->setEnabled(external && valid && validStartHeight);
    updatePublicNodeListState();
}

void WalletdParamsDialog::externalBytecoindToggled(bool checked)
{
    ui->browseDataFolderButton->setEnabled(!checked);
    updatePublicNodeListState();
    hostChanged();
}

void WalletdParamsDialog::rebuildPublicNodeList()
{
    ui->publicNodeList->clear();
    const QString currentNode = ui->editHost->text().trimmed();

    for (const QString &node : publicNodes_)
    {
        auto *item = new QListWidgetItem;
        item->setSizeHint(QSize(0, 52));
        item->setData(Qt::UserRole, node);
        ui->publicNodeList->addItem(item);

        auto *row = new QWidget(ui->publicNodeList);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(6, 2, 6, 2);
        layout->setSpacing(6);

        auto *addressLabel = new QLabel(node, row);
        addressLabel->setMinimumWidth(0);
        addressLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        addressLabel->setToolTip(node);
        layout->addWidget(addressLabel, 1);

        const bool isCurrent = node.compare(currentNode, Qt::CaseInsensitive) == 0;
        auto *currentBadge = new QWidget(row);
        currentBadge->setObjectName(QStringLiteral("currentPublicNodeLabel"));
        currentBadge->setFixedSize(120, 40);
        currentBadge->setStyleSheet(QStringLiteral(
            "QWidget#currentPublicNodeLabel {"
            " background-color: rgba(85, 214, 161, 28);"
            " border: 1px solid #55D6A1; border-radius: 7px; }"));
        currentBadge->setToolTip(tr("This is the currently selected node."));
        auto *badgeLayout = new QHBoxLayout(currentBadge);
        badgeLayout->setContentsMargins(8, 2, 8, 2);
        badgeLayout->setSpacing(5);
        auto *currentIcon = new QLabel(currentBadge);
        currentIcon->setPixmap(QIcon(QStringLiteral(":/icons/footer/synced.svg")).pixmap(18, 18));
        currentIcon->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        badgeLayout->addWidget(currentIcon);
        auto *currentText = new QLabel(tr("Current"), currentBadge);
        currentText->setStyleSheet(QStringLiteral(
            "color: #55D6A1; background: transparent; border: none; font-weight: 600;"));
        badgeLayout->addWidget(currentText);
        layout->addWidget(currentBadge);

        auto *switchButton = new QPushButton(tr("Switch"), row);
        switchButton->setObjectName(QStringLiteral("switchPublicNodeButton"));
        switchButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/switch-node.svg")));
        switchButton->setIconSize(QSize(16, 16));
        switchButton->setFixedSize(120, 40);
        switchButton->setStyleSheet(
            QStringLiteral("QPushButton { padding: 2px 8px; min-height: 0; }"));
        switchButton->setToolTip(tr("Switch to this node and restart the wallet daemon."));
        layout->addWidget(switchButton);

        auto *removeButton = new QPushButton(tr("Remove"), row);
        removeButton->setObjectName(QStringLiteral("removePublicNodeButton"));
        removeButton->setIcon(QIcon(QStringLiteral(":/icons/daemon/remove-node.svg")));
        removeButton->setIconSize(QSize(16, 16));
        removeButton->setFixedSize(120, 40);
        removeButton->setStyleSheet(
            QStringLiteral("QPushButton { padding: 2px 8px; min-height: 0; }"));
        removeButton->setToolTip(tr("Remove this node from the saved list."));
        layout->addWidget(removeButton);

        connect(switchButton, &QPushButton::clicked, this,
                [this, node]() { switchToPublicNode(node); });
        connect(removeButton, &QPushButton::clicked, this,
                [this, node]() { removePublicNode(node); });
        currentBadge->setVisible(isCurrent);
        switchButton->setVisible(!isCurrent);
        ui->publicNodeList->setItemWidget(item, row);
    }
    updatePublicNodeListState();
}

void WalletdParamsDialog::updatePublicNodeListState()
{
    const bool external = ui->radioExternalBytecoind->isChecked();
    bool validStartHeight = false;
    const qulonglong startHeight = ui->walletSyncStartHeightEdit->text().toULongLong(&validStartHeight);
    validStartHeight = validStartHeight && startHeight <= 4294967295ULL;
    const QString currentNode = ui->editHost->text().trimmed();
    ui->publicNodeList->setEnabled(external);

    for (int i = 0; i < ui->publicNodeList->count(); ++i)
    {
        QListWidgetItem *item = ui->publicNodeList->item(i);
        QWidget *row = ui->publicNodeList->itemWidget(item);
        if (!row)
            continue;

        const QString node = item->data(Qt::UserRole).toString();
        const bool isCurrent = node.compare(currentNode, Qt::CaseInsensitive) == 0;
        auto *currentBadge = row->findChild<QWidget *>(QStringLiteral("currentPublicNodeLabel"));
        auto *switchButton = row->findChild<QPushButton *>(QStringLiteral("switchPublicNodeButton"));
        auto *removeButton = row->findChild<QPushButton *>(QStringLiteral("removePublicNodeButton"));
        if (currentBadge)
            currentBadge->setVisible(isCurrent);
        if (switchButton)
        {
            switchButton->setVisible(!isCurrent);
            switchButton->setEnabled(external && allowToRestart_ && !isCurrent && validStartHeight);
        }
        if (removeButton)
            removeButton->setEnabled(external);
    }
}

void WalletdParamsDialog::addPublicNode()
{
    const QString node = ui->editHost->text().trimmed();
    const QUrl url = QUrl::fromUserInput(node);
    if (!url.isValid() || !isIpOrHostName(url.host())
        || (url.port(-1) != -1 && (url.port() <= 0 || url.port() > 65535)))
    {
        QMessageBox::warning(this, tr("Invalid node address"), tr("Enter a valid node host or URL before adding it."));
        return;
    }
    if (publicNodes_.contains(node, Qt::CaseInsensitive))
    {
        Settings::instance().setPublicBytecoindNodes(publicNodes_);
        return;
    }

    publicNodes_.append(node);
    Settings::instance().setPublicBytecoindNodes(publicNodes_);
    rebuildPublicNodeList();
    ui->publicNodeList->setEnabled(ui->radioExternalBytecoind->isChecked());
}

void WalletdParamsDialog::removePublicNode(const QString &node)
{
    const int index = publicNodes_.indexOf(node);
    if (index < 0)
        return;

    publicNodes_.removeAt(index);
    Settings::instance().setPublicBytecoindNodes(publicNodes_);
    rebuildPublicNodeList();
    ui->publicNodeList->setEnabled(ui->radioExternalBytecoind->isChecked());
}

void WalletdParamsDialog::switchToPublicNode(const QString &node)
{
    bool validStartHeight = false;
    const qulonglong startHeight = ui->walletSyncStartHeightEdit->text().toULongLong(&validStartHeight);
    validStartHeight = validStartHeight && startHeight <= 4294967295ULL;
    if (!allowToRestart_ || !validStartHeight || !publicNodes_.contains(node, Qt::CaseInsensitive))
        return;

    ui->editHost->setText(node);
    ui->radioExternalBytecoind->setChecked(true);
    saveParams();
    emit restartWalletd();
    accept();
}

void WalletdParamsDialog::browseDataFolder()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, tr("Choose blockchain data folder"), QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (directory.isEmpty())
        return;

    QString options = ui->paramsEdit->toPlainText();
    const QRegularExpression dataFolderOption(
        QStringLiteral(R"((^|\s)--data-folder=(?:"[^"]*"|'[^']*'|[^\s]+))"));
    const QRegularExpressionMatch match = dataFolderOption.match(options);
    const QString replacement = match.hasMatch()
        ? match.captured(1) + QStringLiteral("--data-folder=\"") + directory + QLatin1Char('"')
        : (options.isEmpty() ? QString{} : QStringLiteral("\n"))
            + QStringLiteral("--data-folder=\"") + directory + QLatin1Char('"');
    if (match.hasMatch())
        options.replace(match.capturedStart(), match.capturedLength(), replacement);
    else
        options.append(replacement);
    ui->paramsEdit->setPlainText(options);
}

void WalletdParamsDialog::showHelp()
{
    const QString help = tr(
        "<p>Additional command-line options are passed directly to <b>walletd</b>. "
        "Use the exact option names and <code>--option=value</code> form. "
        "A wrong option can stop the daemon from starting.</p>"
        "<p><b>Common options</b></p>"
        "<ul>"
        "<li><code>--data-folder=&lt;path&gt;</code> — wallet cache, blockchain, logs and peer database.</li>"
        "<li><code>--walletd-bind-address=&lt;ip:port&gt;</code> — wallet RPC listener (default: 127.0.0.1:8070).</li>"
        "<li><code>--bytecoind-bind-address=&lt;ip:port&gt;</code> — built-in node RPC listener (default: 127.0.0.1:8081).</li>"
        "<li><code>--p2p-bind-address=&lt;ip:port&gt;</code> — node peer-to-peer listener (default: 0.0.0.0:8080).</li>"
        "<li><code>--p2p-external-port=&lt;port&gt;</code> — externally forwarded P2P port.</li>"
        "<li><code>--seed-node-address=&lt;ip:port&gt;</code>, "
        "<code>--priority-node-address=&lt;ip:port&gt;</code> or "
        "<code>--exclusive-node-address=&lt;ip:port&gt;</code> — peer selection.</li>"
        "</ul>"
        "<p>Use one option per line or separate options with spaces. Put paths containing spaces "
        "in double quotes, for example <code>--data-folder=\"C:\\wallet data\"</code>. "
        "Network and external-node choices above are already set by this dialog; do not add "
        "conflicting duplicate options. Only enable public listeners if you understand the "
        "security and firewall implications.</p>"
        "<p>Use the saved public-node list to add, remove, or switch between external RPC nodes. "
        "The blockchain-folder picker fills in <code>--data-folder</code> for the built-in node.</p>"
        "<p>A non-zero wallet scan start height scans wallet activity from that block onward. "
        "Earlier transactions will not appear, and this setting only takes effect when the wallet "
        "synchronization cache is empty. The built-in node still downloads the full blockchain.</p>"
        "<p>For the complete option list, run <code>walletd --help</code> or consult the "
        "<a href=\"https://github.com/bcndev/bytecoin/blob/master/src/main_walletd.cpp\">walletd source documentation</a>.</p>");
    QMessageBox message(QMessageBox::Information, tr("Wallet daemon options"), help, QMessageBox::Ok, this);
    message.setTextFormat(Qt::RichText);
    message.setTextInteractionFlags(Qt::TextBrowserInteraction);
    if (QLabel *textLabel = message.findChild<QLabel *>(QStringLiteral("qt_msgbox_label")))
        textLabel->setMinimumWidth(820);
    message.setMinimumWidth(900);
    message.setIconPixmap(QIcon(QStringLiteral(":/icons/daemon/help.svg")).pixmap(32, 32));
    message.exec();
}

}
