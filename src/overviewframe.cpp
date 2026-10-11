// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QClipboard>
#include <QPushButton>
#include <QAbstractItemModel>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QTextStream>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QComboBox>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMenu>
#include <QPainter>
#include <QToolTip>
#include <QCursor>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QUrl>
#include <QIcon>


#include "overviewframe.h"
#include "walletmodel.h"
#include "rpcapi.h"

#include "ui_overviewframe.h"

namespace WalletGUI {

class TransactionFilterModel : public QSortFilterProxyModel
{
public:
    explicit TransactionFilterModel(QObject* parent)
        : QSortFilterProxyModel(parent)
        , statusFilter_(0)
    {
        setDynamicSortFilter(true);
    }

    void setSearchText(const QString& text)
    {
        searchText_ = text.trimmed();
        invalidateFilter();
    }

    void setStatusFilter(int status)
    {
        statusFilter_ = status;
        invalidateFilter();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::ToolTipRole && index.isValid()
            && index.column() == WalletModel::COLUMN_HASH)
        {
            return tr("Click the hash to open the explorer. Use the copy button to copy it or the details button to inspect the transaction.");
        }
        if (role == Qt::ForegroundRole && index.isValid()
            && index.column() == WalletModel::COLUMN_AMOUNT)
        {
            const QModelIndex hashIndex = sourceModel()->index(index.row(), WalletModel::COLUMN_HASH);
            const QStringList recipients = sourceModel()->data(hashIndex, WalletModel::ROLE_RECIPIENTS).toStringList();
            if (!recipients.isEmpty())
                return QBrush(QColor(QStringLiteral("#FF9A9A")));
            if (sourceModel()->data(hashIndex, WalletModel::ROLE_AMOUNT).toULongLong() > 0)
                return QBrush(QColor(QStringLiteral("#72D7A0")));
        }
        return QSortFilterProxyModel::data(index, role);
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        const QModelIndex statusIndex = sourceModel()->index(sourceRow, WalletModel::COLUMN_TX_STATUS, sourceParent);
        const QString status = sourceModel()->data(statusIndex, WalletModel::ROLE_TX_STATUS).toString();
        if (statusFilter_ == 1 && status != tr("Pending") && !status.startsWith(tr("Confirming")))
            return false;
        if (statusFilter_ == 2 && status != tr("Confirmed"))
            return false;
        if (searchText_.isEmpty())
            return true;

        for (int column = WalletModel::COLUMN_UNLOCK_TIME; column <= WalletModel::COLUMN_TX_STATUS; ++column)
        {
            const QModelIndex index = sourceModel()->index(sourceRow, column, sourceParent);
            if (sourceModel()->data(index).toString().contains(searchText_, Qt::CaseInsensitive))
                return true;
            if (column == WalletModel::COLUMN_PAYMENT_ID
                && sourceModel()->data(index, WalletModel::ROLE_PAYMENT_ID).toString().contains(searchText_, Qt::CaseInsensitive))
                return true;
            if (column == WalletModel::COLUMN_HASH
                && sourceModel()->data(index, WalletModel::ROLE_RECIPIENTS).toStringList().join(' ').contains(searchText_, Qt::CaseInsensitive))
                return true;
        }
        return false;
    }

private:
    QString searchText_;
    int statusFilter_;
};

namespace {

const char MAIN_TX_HASH_URL[] = "https://explorer.bytecoin.world/tx?hash=%1";
const char MAIN_BLOCK_HASH_URL[] = "https://explorer.bytecoin.world/block?hash=%1";
const char MAIN_BLOCK_HEIGHT_URL[] = "https://explorer.bytecoin.world/block?height=%1";

QRect transactionHashCopyButtonRect(const QRect& cellRect)
{
    return QRect(cellRect.right() - 72, cellRect.top() + (cellRect.height() - 28) / 2, 30, 28);
}

QRect transactionDetailsButtonRect(const QRect& cellRect)
{
    return QRect(cellRect.right() - 36, cellRect.top() + (cellRect.height() - 28) / 2, 30, 28);
}

class TransactionHashDelegate : public QStyledItemDelegate
{
public:
    explicit TransactionHashDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem textOption(option);
        textOption.rect.setRight(transactionHashCopyButtonRect(option.rect).left() - 8);
        QStyledItemDelegate::paint(painter, textOption, index);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const auto* view = qobject_cast<const QAbstractItemView*>(parent());
        const QPoint mousePosition = view
            ? view->viewport()->mapFromGlobal(QCursor::pos())
            : QPoint(-1, -1);
        const auto drawButton = [painter, &mousePosition](const QRect& rect, bool details) {
            const bool hovered = rect.contains(mousePosition);
            painter->setPen(QPen(hovered ? QColor(QStringLiteral("#FF6AA8"))
                                         : QColor(QStringLiteral("#51404E")), 1));
            painter->setBrush(hovered ? QColor(QStringLiteral("#55364A")) : QColor(QStringLiteral("#292630")));
            painter->drawRoundedRect(rect, 6, 6);

            QPen iconPen(QColor(QStringLiteral("#FFD1E5")), 1.7);
            iconPen.setCapStyle(Qt::RoundCap);
            iconPen.setJoinStyle(Qt::RoundJoin);
            painter->setPen(iconPen);
            const qreal centerX = rect.center().x();
            const qreal centerY = rect.center().y();
            if (details)
            {
                painter->drawRoundedRect(QRectF(centerX - 7, centerY - 9, 13, 18), 2, 2);
                painter->drawLine(QPointF(centerX - 4, centerY - 3), QPointF(centerX + 2, centerY - 3));
                painter->drawLine(QPointF(centerX - 4, centerY + 1), QPointF(centerX + 2, centerY + 1));
                painter->drawLine(QPointF(centerX - 4, centerY + 5), QPointF(centerX, centerY + 5));
            }
            else
            {
                painter->drawRoundedRect(QRectF(centerX - 6, centerY - 7, 11, 13), 1.5, 1.5);
                painter->drawRoundedRect(QRectF(centerX - 2, centerY - 5, 9, 12), 1.5, 1.5);
            }
        };
        drawButton(transactionHashCopyButtonRect(option.rect), false);
        drawButton(transactionDetailsButtonRect(option.rect), true);
        painter->restore();
    }

};

QString csvEscape(QString value)
{
    value.replace('"', QStringLiteral("\"\""));
    if (value.contains(';') || value.contains('"') || value.contains('\n') || value.contains('\r'))
        value = QLatin1Char('"') + value + QLatin1Char('"');
    return value;
}

QString tsvEscape(QString value)
{
    value.replace('\\', QStringLiteral("\\\\"));
    value.replace('\t', QStringLiteral("\\t"));
    value.replace('\r', QStringLiteral("\\r"));
    value.replace('\n', QStringLiteral("\\n"));
    return value;
}

//const char OVERVIEW_STYLE_SHEET_TEMPLATE[] =
//  "* {"
//    "font-family: %fontFamily%;"
//  "}"

//  "WalletGui--OverviewFrame {"
//    "background-color: #ffffff;"
//    "border: none;"
//  "}"

//  "WalletGui--OverviewFrame #m_allTransactionsButton {"
//    "margin-top: 2px;"
//  "}"

//  "WalletGui--OverviewFrame #m_newsFrame {"
//    "min-height: 130px;"
//    "max-height: 130px;"
//    "border: none;"
//    "border-top: 1px solid %borderColor%;"
//    "background-color: %backgroundColorGray%;"
//  "}";

}

//const QMap<int, int> indices =
//        {{ WalletModel::COLUMN_AMOUNT, 0 },
//         { WalletModel::COLUMN_FEE, 1 },
//         { WalletModel::COLUMN_HASH, 2 },
//         { WalletModel::COLUMN_BLOCK_HEIGHT, 3 },
//         { WalletModel::COLUMN_BLOCK_HASH, 4 },
//         { WalletModel::COLUMN_TIMESTAMP, 5 },
//         { WalletModel::COLUMN_UNLOCK_TIME, 6 },
//         { WalletModel::COLUMN_PROOF, 7}};

const QList<int> columns_order =
    { WalletModel::COLUMN_TX_STATUS,
      WalletModel::COLUMN_TIMESTAMP,
      WalletModel::COLUMN_AMOUNT,
      WalletModel::COLUMN_FEE,
      WalletModel::COLUMN_HASH,
      WalletModel::COLUMN_PROOF };

OverviewFrame::OverviewFrame(QWidget* parent)
    : QFrame(parent)
    , m_ui(new Ui::OverviewFrame)
    , m_mainWindow(nullptr)
    , m_transactionsModel(nullptr)
    , m_csvExporter(nullptr)
    , m_transactionsFilter(new TransactionFilterModel(this))
{
    m_ui->setupUi(this);
    m_ui->m_walletHealthWalletIcon->setPixmap(QIcon(QStringLiteral(":/icons/footer/wallet.svg")).pixmap(16, 16));
    m_ui->m_walletHealthWalletIcon->setToolTip(tr("Wallet daemon"));
    m_ui->m_walletHealthNodeIcon->setPixmap(QIcon(QStringLiteral(":/icons/footer/node.svg")).pixmap(16, 16));
    m_ui->m_walletHealthNodeIcon->setToolTip(tr("Blockchain node"));
    m_ui->m_walletHealthSyncIcon->setPixmap(QIcon(QStringLiteral(":/icons/footer/sync.svg")).pixmap(16, 16));
    m_ui->m_walletHealthSyncIcon->setToolTip(tr("Wallet synchronization"));
    m_ui->m_walletHealthPeersIcon->setPixmap(QIcon(QStringLiteral(":/icons/footer/peers.svg")).pixmap(16, 16));
    m_ui->m_walletHealthPeersIcon->setToolTip(tr("Peer connections"));

    m_ui->m_recentTransactionsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->m_recentTransactionsView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->m_recentTransactionsView->viewport()->setMouseTracking(true);
    m_ui->m_recentTransactionsView->viewport()->installEventFilter(this);

    connect(m_ui->m_balanceOverviewFrame, &BalanceOverviewFrame::copiedToClipboardSignal, this, &OverviewFrame::copiedToClipboardSignal);
    connect(m_ui->m_searchTransactionsEdit, &QLineEdit::textChanged, m_transactionsFilter, &TransactionFilterModel::setSearchText);
    connect(m_ui->m_transactionStatusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_transactionsFilter, &TransactionFilterModel::setStatusFilter);
    connect(m_ui->m_recentTransactionsView, &QTableView::doubleClicked, this, &OverviewFrame::showTransactionDetails);
    auto* exportMenu = new QMenu(m_ui->m_exportButton);
    const auto addExportAction = [this, exportMenu](const QString& label, TransactionExportFormat format) {
        QAction* action = exportMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, format]() { exportTransactions(format); });
    };
    addExportAction(tr("Comma-separated values (CSV)"), TransactionExportFormat::CSV);
    addExportAction(tr("Tab-separated values (TSV)"), TransactionExportFormat::TSV);
    addExportAction(tr("JSON"), TransactionExportFormat::JSON);
    m_ui->m_exportButton->setText(tr("Export activity"));
    m_ui->m_exportButton->setMenu(exportMenu);
}

OverviewFrame::~OverviewFrame()
{}

void OverviewFrame::setTransactionsModel(WalletModel* model)
{
    m_transactionsModel = model;
    m_transactionsFilter->setSourceModel(model);
    m_ui->m_recentTransactionsView->setModel(m_transactionsFilter);
    m_ui->m_recentTransactionsView->setItemDelegateForColumn(
        WalletModel::COLUMN_HASH, new TransactionHashDelegate(m_ui->m_recentTransactionsView));
    const int columns = m_transactionsModel->columnCount();
    // hide all columns
    for (int i = 0; i < columns; ++i)
        m_ui->m_recentTransactionsView->setColumnHidden(i, true);

    QHeaderView& header = *m_ui->m_recentTransactionsView->horizontalHeader();
    header.setResizeContentsPrecision(-1);
    header.setMinimumSectionSize(36);
    m_ui->m_recentTransactionsView->verticalHeader()->setDefaultSectionSize(40);
    header.setSectionResizeMode(WalletModel::COLUMN_HASH, QHeaderView::Stretch);

    for (int i = 0; i < columns_order.size(); ++i)
    {
        const int column = columns_order[i];
        header.moveSection(header.visualIndex(column), i);
        header.setSectionResizeMode(column, column == WalletModel::COLUMN_HASH ? QHeaderView::Stretch : QHeaderView::ResizeToContents);

        // then unhide needed ones
        m_ui->m_recentTransactionsView->setColumnHidden(column, false);
    }
    m_ui->m_recentTransactionsView->setColumnHidden(WalletModel::COLUMN_PAYMENT_ID, false);
    header.setSectionResizeMode(WalletModel::COLUMN_PAYMENT_ID, QHeaderView::ResizeToContents);

    delete m_csvExporter;
    m_csvExporter = new CSVTransactionsExporter(model, this);
}

void OverviewFrame::setWalletModel(WalletModel* walletModel)
{
    m_ui->m_balanceOverviewFrame->setWalletModel(walletModel);
    connect(walletModel, &QAbstractItemModel::dataChanged, this, &OverviewFrame::updateWalletHealth);
    updateWalletHealth();
}

void OverviewFrame::updateWalletHealth()
{
    WalletModel* walletModel = m_transactionsModel;
    if (!walletModel)
        return;

    const QString walletState = walletModel->isConnected() ? tr("Connected") : tr("Disconnected");
    const QString nodeError = walletModel->getLowerLevelError();
    const QString nodeState = nodeError.isEmpty()
        ? tr("Node: No reported errors")
        : tr("Node: RPC unavailable");
    const bool nodeStatusUnavailable = nodeError == QStringLiteral("CONNECTION_FAILED")
        || nodeError == QStringLiteral("CONNECTION_HTTP_FAILED")
        || nodeError == QStringLiteral("AUTHORIZATION_FAILED")
        || nodeError == QStringLiteral("INCOMPATIBLE_DAEMON_VERSION");

    QString syncState;
    if (nodeStatusUnavailable)
        syncState = tr("Sync: Unavailable");
    else switch (walletModel->getSyncStatus())
    {
    case WalletModel::SyncStatus::SYNCED:
        syncState = tr("Sync: Block %1").arg(walletModel->getLastBlockHeight());
        break;
    case WalletModel::SyncStatus::LAGGED:
        syncState = tr("Sync: Tip may be stale (%1)").arg(walletModel->getFormattedTimeSinceLastBlock());
        break;
    case WalletModel::SyncStatus::NOT_SYNCED:
        syncState = walletModel->getLastBlockHeight() == 0
            ? tr("Sync: Waiting for blocks")
            : tr("Sync: %1 blocks behind").arg(walletModel->getBlocksLeftToSync());
        break;
    }

    m_ui->m_walletConnectionLabel->setText(tr("Wallet: %1").arg(walletState));
    m_ui->m_walletHealthWalletIcon->setToolTip(walletModel->isConnected()
        ? tr("Wallet daemon connected")
        : tr("Wallet daemon disconnected"));
    m_ui->m_nodeHealthLabel->setText(nodeState);
    m_ui->m_walletHealthNodeIcon->setToolTip(nodeStatusUnavailable
        ? tr("Node RPC unavailable: %1").arg(nodeError)
        : tr("Node RPC responding"));
    m_ui->m_nodeHealthLabel->setToolTip(nodeStatusUnavailable
        ? tr("Node RPC error: %1\nThe wallet daemon is reachable, but its Bytecoin node RPC request failed. Check the node host, RPC port, network, firewall, and any RPC authorization settings.").arg(nodeError)
        : QString{});
    m_ui->m_syncHealthLabel->setText(syncState);
    const QString syncIconPath = !nodeStatusUnavailable
            && walletModel->getSyncStatus() == WalletModel::SyncStatus::SYNCED
        ? QStringLiteral(":/icons/footer/synced.svg")
        : QStringLiteral(":/icons/footer/sync.svg");
    m_ui->m_walletHealthSyncIcon->setPixmap(QIcon(syncIconPath).pixmap(16, 16));
    m_ui->m_walletHealthSyncIcon->setToolTip(nodeStatusUnavailable
        ? tr("Synchronization status unavailable")
        : walletModel->getSyncStatus() == WalletModel::SyncStatus::SYNCED
            ? tr("Wallet synchronized")
            : tr("Wallet synchronizing"));
    m_ui->m_peerHealthLabel->setText(nodeStatusUnavailable
        ? tr("Peers: Unavailable")
        : tr("Peers: %1 incoming, %2 outgoing")
            .arg(walletModel->getPeerCountIncoming())
            .arg(walletModel->getPeerCountOutgoing()));
    m_ui->m_walletHealthPeersIcon->setToolTip(nodeStatusUnavailable
        ? tr("Peer status unavailable")
        : tr("Peer connections available"));
}

void OverviewFrame::showTransactionDetails(const QModelIndex& proxyIndex)
{
    if (!proxyIndex.isValid() || !m_transactionsModel)
        return;

    const QModelIndex index = m_transactionsFilter->mapToSource(proxyIndex);
    const auto value = [this, &index](WalletModel::Columns column, int role = Qt::DisplayRole) {
        return m_transactionsModel->data(m_transactionsModel->index(index.row(), column), role).toString();
    };

    QStringList details;
    details << tr("Network: %1").arg(value(WalletModel::COLUMN_NET));
    details << tr("Status: %1").arg(value(WalletModel::COLUMN_TX_STATUS));
    details << tr("Date: %1").arg(value(WalletModel::COLUMN_TIMESTAMP));
    details << tr("Amount: %1 BCN").arg(value(WalletModel::COLUMN_AMOUNT));
    details << tr("Fee: %1 BCN").arg(value(WalletModel::COLUMN_FEE));
    details << tr("Transaction hash: %1").arg(value(WalletModel::COLUMN_HASH));
    details << tr("Block height: %1").arg(value(WalletModel::COLUMN_BLOCK_HEIGHT));
    details << tr("Block hash: %1").arg(value(WalletModel::COLUMN_BLOCK_HASH));
    details << tr("Payment ID: %1").arg(value(WalletModel::COLUMN_PAYMENT_ID));
    details << tr("Anonymity level: %1").arg(value(WalletModel::COLUMN_ANONYMITY));
    details << tr("Unlock time: %1").arg(value(WalletModel::COLUMN_UNLOCK_TIME));
    details << tr("Public key: %1").arg(value(WalletModel::COLUMN_PK));
    details << tr("Coinbase: %1").arg(value(WalletModel::COLUMN_COINBASE));
    details << tr("Extra data: %1").arg(value(WalletModel::COLUMN_EXTRA));
    const QStringList recipients = m_transactionsModel->data(
        m_transactionsModel->index(index.row(), WalletModel::COLUMN_HASH),
        WalletModel::ROLE_RECIPIENTS).toStringList();
    if (!recipients.isEmpty())
        details << tr("Recipient(s): %1").arg(recipients.join(", "));

    QMessageBox message(QMessageBox::Information, tr("Transaction details"), details.join('\n'), QMessageBox::Ok, this);
    message.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    message.exec();
}

void OverviewFrame::rowsInserted(const QModelIndex& /*parent*/, int /*first*/, int /*last*/)
{
//    m_ui->m_recentTransactionsView->resizeColumnsToContents();
}

void OverviewFrame::setMainWindow(QWidget* mainWindow)
{
    m_mainWindow = mainWindow;
}

void OverviewFrame::exportTransactions(TransactionExportFormat format)
{
    m_csvExporter->start(format);
}

bool OverviewFrame::eventFilter(QObject* object, QEvent* event)
{
    const bool needToProcess =
            object == m_ui->m_recentTransactionsView->viewport()
            && (event->type() == QEvent::MouseButtonRelease
                || event->type() == QEvent::MouseMove
                || event->type() == QEvent::ToolTip
                || event->type() == QEvent::Leave);
    if (!needToProcess)
        return false;

    QWidget* widget = qobject_cast<QWidget*>(object);
    if(!widget)
        return false;
    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(widget->parent());
    if(!view)
        return false;

    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* e = (QMouseEvent*)event;
        const QModelIndex proxyIndex = view->indexAt(e->pos());
        if (!proxyIndex.isValid())
            return false;
        const QModelIndex modelIndex = m_transactionsFilter->mapToSource(proxyIndex);
        if (e->button() == Qt::LeftButton && modelIndex.column() == WalletModel::COLUMN_HASH)
        {
            const QString hash = m_transactionsModel->data(modelIndex, WalletModel::ROLE_HASH).toString();
            if (hash.isEmpty())
                return false;

            const QRect cellRect = view->visualRect(proxyIndex);
            if (transactionHashCopyButtonRect(cellRect).contains(e->pos()))
            {
                QApplication::clipboard()->setText(hash);
                QToolTip::showText(e->globalPos(), tr("Transaction hash copied"), view);
                return true;
            }
            if (transactionDetailsButtonRect(cellRect).contains(e->pos()))
            {
                showTransactionDetails(proxyIndex);
                return true;
            }

            if (m_transactionsModel->data(modelIndex, WalletModel::ROLE_NET).toString() == RpcApi::MAIN_NET_NAME)
            {
                const QUrl url = QUrl::fromUserInput(QString::fromLatin1(MAIN_TX_HASH_URL).arg(hash));
                if (!QDesktopServices::openUrl(url))
                    QMessageBox::warning(this, tr("Explorer unavailable"),
                        tr("Could not open the Bytecoin World explorer link."));
                return true;
            }
        }
        const QString net = m_transactionsModel->data(modelIndex, WalletModel::ROLE_NET).toString();
        const bool isMainnet = (net == RpcApi::MAIN_NET_NAME);
        const bool explorableColumns =
                isMainnet &&
                ((modelIndex.column() == WalletModel::COLUMN_BLOCK_HASH && !m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HASH).toString().isEmpty()) ||
                (modelIndex.column() == WalletModel::COLUMN_BLOCK_HEIGHT && !m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HEIGHT).toString().isEmpty()));
        const bool proof = m_transactionsModel->data(modelIndex, WalletModel::ROLE_PROOF).toBool();
        const bool proofColumn = (modelIndex.column() == WalletModel::COLUMN_PROOF && proof);
        const bool valid = explorableColumns || proofColumn;
        if(!valid)
            return false;

        if (explorableColumns)
        {
            switch(modelIndex.column())
            {
            case WalletModel::COLUMN_BLOCK_HASH:
                QDesktopServices::openUrl(QUrl::fromUserInput(QString{MAIN_BLOCK_HASH_URL}.arg(m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HASH).toString())));
                break;
            case WalletModel::COLUMN_BLOCK_HEIGHT:
                QDesktopServices::openUrl(QUrl::fromUserInput(QString{MAIN_BLOCK_HEIGHT_URL}.arg(m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HEIGHT).toString())));
                break;
            }
        }
        if (proofColumn)
        {
            const QString txHash = m_transactionsModel->data(modelIndex, WalletModel::ROLE_HASH).toString();
            const QStringList addresses = m_transactionsModel->data(modelIndex, WalletModel::ROLE_RECIPIENTS).toStringList();
            emit createProofSignal(txHash, addresses, !proof);
        }
    }
    else if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        const QModelIndex proxyIndex = view->indexAt(helpEvent->pos());
        if (proxyIndex.isValid() && proxyIndex.column() == WalletModel::COLUMN_HASH)
        {
            const QRect cellRect = view->visualRect(proxyIndex);
            if (transactionHashCopyButtonRect(cellRect).contains(helpEvent->pos()))
            {
                QToolTip::showText(helpEvent->globalPos(), tr("Copy transaction hash"), view);
                return true;
            }
            if (transactionDetailsButtonRect(cellRect).contains(helpEvent->pos()))
            {
                QToolTip::showText(helpEvent->globalPos(), tr("View transaction details"), view);
                return true;
            }
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent* e = (QMouseEvent*)event;
        const QModelIndex proxyIndex = view->indexAt(e->pos());
        if (!proxyIndex.isValid())
        {
            setCursor(Qt::ArrowCursor);
            return false;
        }
        const QModelIndex modelIndex = m_transactionsFilter->mapToSource(proxyIndex);

        const QString net = m_transactionsModel->data(modelIndex, WalletModel::ROLE_NET).toString();
        const bool isMainnet = (net == RpcApi::MAIN_NET_NAME);
        const bool explorableColumns =
                isMainnet &&
                ((modelIndex.column() == WalletModel::COLUMN_BLOCK_HASH && !m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HASH).toString().isEmpty()) ||
                (modelIndex.column() == WalletModel::COLUMN_BLOCK_HEIGHT && !m_transactionsModel->data(modelIndex, WalletModel::ROLE_BLOCK_HEIGHT).toString().isEmpty()));
        const bool proof = m_transactionsModel->data(modelIndex, WalletModel::ROLE_PROOF).toBool();
        const bool proofColumn = (modelIndex.column() == WalletModel::COLUMN_PROOF && proof);
        const bool hashAction = modelIndex.column() == WalletModel::COLUMN_HASH
            && !m_transactionsModel->data(modelIndex, WalletModel::ROLE_HASH).toString().isEmpty();
        const bool showHandCursor = explorableColumns || proofColumn || hashAction;

        setCursor(showHandCursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
    else if (event->type() == QEvent::Leave)
        setCursor(Qt::ArrowCursor);

    return false;
}

CSVTransactionsExporter::CSVTransactionsExporter(WalletModel* transactionsModel, QWidget* parent)
    : parent_(parent)
    , file_(this)
    , output_(&file_)
    , exportProgressDlg_(nullptr)
    , transactionsModel_(transactionsModel)
    , format_(TransactionExportFormat::CSV)
    , firstJsonRecord_(true)
    , exportActive_(false)
    , exportedRowCount_(0)
{
}

void CSVTransactionsExporter::start(TransactionExportFormat format)
{
    format_ = format;
    QString filter;
    QString suggestedName;
    switch (format_)
    {
    case TransactionExportFormat::CSV:
        filter = tr("CSV files (*.csv)");
        suggestedName = tr("transactions.csv");
        break;
    case TransactionExportFormat::TSV:
        filter = tr("TSV files (*.tsv)");
        suggestedName = tr("transactions.tsv");
        break;
    case TransactionExportFormat::JSON:
        filter = tr("JSON files (*.json)");
        suggestedName = tr("transactions.json");
        break;
    }
    QString selectedFilter;
    const QString fileName = QFileDialog::getSaveFileName(
                parent_,
                tr("Export transaction history"),
                QDir::home().filePath(suggestedName),
                filter,
                &selectedFilter);
    if (fileName.isEmpty())
        return;

    QString outputFileName = fileName;
    if (QFileInfo(outputFileName).suffix().isEmpty())
        outputFileName += format_ == TransactionExportFormat::CSV ? QStringLiteral(".csv")
            : format_ == TransactionExportFormat::TSV ? QStringLiteral(".tsv")
            : QStringLiteral(".json");
    file_.setFileName(outputFileName);
    if (!file_.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(parent_, tr("Error"), tr("Failed to open the specified file (%1): %2")
            .arg(outputFileName, file_.errorString()));
        return;
    }
    output_.setCodec("UTF-8");
    output_.resetStatus();
    firstJsonRecord_ = true;
    exportedRowCount_ = 0;
    exportActive_ = true;

    exportProgressDlg_ = new QProgressDialog(parent_);
    exportProgressDlg_->setLabelText(tr("Preparing transaction history export..."));
    exportProgressDlg_->setCancelButtonText(tr("Cancel export"));
    exportProgressDlg_->setWindowModality(Qt::WindowModal);
    exportProgressDlg_->setRange(0, 0);
    exportProgressDlg_->setValue(0);
    exportProgressDlg_->setMinimumDuration(0);
    exportProgressDlg_->setAutoClose(false);
    exportProgressDlg_->setAutoReset(false);
    exportProgressDlg_->show();

    writeHeader();
    connect(exportProgressDlg_, &QProgressDialog::canceled, this, &CSVTransactionsExporter::cancel);

    if (transactionsModel_->canFetchMore())
    {
        connect(transactionsModel_, &WalletModel::fetchedSignal, this, &CSVTransactionsExporter::fetched);
        connect(transactionsModel_, &WalletModel::nothingToFetchSignal, this, &CSVTransactionsExporter::finish);
        connect(transactionsModel_, &WalletModel::modelAboutToBeReset, exportProgressDlg_, &QProgressDialog::cancel);
        transactionsModel_->fetchMore();
    }
    else
    {
        finish();
    }
}

void CSVTransactionsExporter::writeHeader()
{
    if (format_ == TransactionExportFormat::JSON)
    {
        output_ << "[\n";
        return;
    }

    QStringList columns;
    for(int i = 0; i < columns_order.size(); ++i)
    {
        const int column = columns_order[i];
        if (column != WalletModel::COLUMN_PROOF) // skip proof column
            columns << transactionsModel_->headerData(columns_order[i], Qt::Horizontal).toString();
    }
    const QChar separator = format_ == TransactionExportFormat::CSV ? QLatin1Char(';') : QLatin1Char('\t');
    for (QString& column : columns)
        column = format_ == TransactionExportFormat::CSV ? csvEscape(column) : tsvEscape(column);
    output_ << columns.join(separator) << '\n';
}

void CSVTransactionsExporter::writeData()
{
    QStringList headers;
    for (int column : columns_order)
    {
        if (column != WalletModel::COLUMN_PROOF)
            headers << transactionsModel_->headerData(column, Qt::Horizontal).toString();
    }

    const int rowCount = transactionsModel_->rowCount();
    for(int row = exportedRowCount_; row < rowCount; ++row)
    {
        if (!exportActive_)
            return;
        QStringList columns;
        QJsonObject record;
        int headerIndex = 0;
        for(int i = 0; i < columns_order.size(); ++i)
        {
            const int column = columns_order[i];
            if (column == WalletModel::COLUMN_PROOF)
                continue;
            const QString value = transactionsModel_->data(transactionsModel_->index(row, column)).toString();
            columns << value;
            record.insert(headers.at(headerIndex++), value);
        }

        if (format_ == TransactionExportFormat::JSON)
        {
            if (!firstJsonRecord_)
                output_ << ",\n";
            output_ << QString::fromUtf8(QJsonDocument(record).toJson(QJsonDocument::Compact));
            firstJsonRecord_ = false;
        }
        else
        {
            const QChar separator = format_ == TransactionExportFormat::CSV ? QLatin1Char(';') : QLatin1Char('\t');
            for (QString& column : columns)
                column = format_ == TransactionExportFormat::CSV ? csvEscape(column) : tsvEscape(column);
            output_ << columns.join(separator) << '\n';
        }
        ++exportedRowCount_;
        if (exportedRowCount_ % 250 == 0)
        {
            if (exportProgressDlg_)
                exportProgressDlg_->setLabelText(tr("Exporting %1 transactions...").arg(exportedRowCount_));
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if (!exportActive_)
                return;
        }
    }
}

void CSVTransactionsExporter::close()
{
    if (exportProgressDlg_)
    {
        exportProgressDlg_->setMaximum(1);
        exportProgressDlg_->setValue(exportProgressDlg_->maximum());
        exportProgressDlg_->deleteLater();
        exportProgressDlg_ = nullptr;
    }
}

void CSVTransactionsExporter::fetched()
{
    if (!exportActive_)
        return;
    writeData();
    if (exportProgressDlg_)
    {
        exportProgressDlg_->setLabelText(tr("Exporting %1 transactions...").arg(exportedRowCount_));
        exportProgressDlg_->show();
    }
    if (transactionsModel_->canFetchMore())
        transactionsModel_->fetchMore();
}

void CSVTransactionsExporter::reset()
{
    exportActive_ = false;
    disconnect(transactionsModel_, &WalletModel::fetchedSignal, this, &CSVTransactionsExporter::fetched);
    disconnect(transactionsModel_, &WalletModel::nothingToFetchSignal, this, &CSVTransactionsExporter::finish);
    if (exportProgressDlg_)
    {
        disconnect(transactionsModel_, &WalletModel::modelAboutToBeReset, exportProgressDlg_, &QProgressDialog::cancel);
        disconnect(exportProgressDlg_, &QProgressDialog::canceled, this, &CSVTransactionsExporter::cancel);
    }
    close();
}

void CSVTransactionsExporter::finish()
{
    if (!exportActive_)
        return;
    writeData();
    if (!exportActive_)
        return;
    if (format_ == TransactionExportFormat::JSON)
        output_ << "\n]\n";
    output_.flush();
    const QTextStream::Status streamStatus = output_.status();
    reset();
    if (streamStatus == QTextStream::Ok && file_.commit())
        QMessageBox::information(parent_, tr("Info"), tr("Transaction history successfully exported."));
    else
    {
        file_.cancelWriting();
        QMessageBox::critical(parent_, tr("Export failed"), tr("Could not save the export file: %1")
            .arg(streamStatus == QTextStream::Ok ? file_.errorString() : tr("An error occurred while writing the export.")));
    }
}

void CSVTransactionsExporter::cancel()
{
    if (!exportActive_)
        return;
    reset();
    file_.cancelWriting();
    output_.resetStatus();
    QMessageBox::information(parent_, tr("Info"), tr("Export canceled. The destination file was not changed."));
}

}
