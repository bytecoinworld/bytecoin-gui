// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QToolTip>
#include <functional>

#include "myaddressesframe.h"
#include "addressbookmanager.h"
#include "walletmodel.h"
#include "questiondialog.h"
#include "addressbookmodel.h"
#include "newmyaddressdialog.h"
#include "ui_myaddressesframe.h"
#include "mainwindow.h"
#include "qrutils.h"

#include <QUrl>
#include <QUrlQuery>

namespace WalletGUI
{

namespace {

QRect addressCopyIconRect(const QRect& cellRect)
{
    return QRect(cellRect.center().x() - 8, cellRect.center().y() - 8, 16, 16);
}

class AddressCopyDelegate : public QStyledItemDelegate
{
public:
    explicit AddressCopyDelegate(std::function<void(const QString&)> copyAddress, QObject* parent)
        : QStyledItemDelegate(parent)
        , copyAddress_(std::move(copyAddress))
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);
        if (index.column() != AddressBookModel::COLUMN_COPY_ACTION)
            return;

        const QRect iconRect = addressCopyIconRect(option.rect);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QPen iconPen(option.state & QStyle::State_MouseOver
            ? QColor(QStringLiteral("#FFB5D6"))
            : QColor(QStringLiteral("#FF8CC4")), 1.5);
        iconPen.setCapStyle(Qt::RoundCap);
        iconPen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(iconPen);
        const qreal x = iconRect.center().x();
        const qreal y = iconRect.center().y();
        painter->setBrush(option.palette.base().color());
        painter->drawRoundedRect(QRectF(x - 2, y - 6, 8, 10), 1.2, 1.2);
        painter->drawRoundedRect(QRectF(x - 6, y - 3, 8, 10), 1.2, 1.2);
        painter->drawLine(QPointF(x - 4, y), QPointF(x, y));
        painter->drawLine(QPointF(x - 4, y + 3), QPointF(x, y + 3));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(size.height(), 40));
        return size;
    }

    bool editorEvent(QEvent* event, QAbstractItemModel*, const QStyleOptionViewItem& option,
        const QModelIndex& index) override
    {
        if (event->type() != QEvent::MouseButtonRelease)
            return false;

        const auto* mouseEvent = static_cast<const QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton
            || index.column() != AddressBookModel::COLUMN_COPY_ACTION
            || !option.rect.contains(mouseEvent->pos()))
            return false;

        const QString address = index.data(AddressBookModel::ROLE_ADDRESS).toString();
        if (address.isEmpty())
            return false;

        copyAddress_(address);
        QToolTip::showText(QCursor::pos(), QObject::tr("Address copied"));
        return true;
    }

private:
    std::function<void(const QString&)> copyAddress_;
};

}

MyAddressesFrame::MyAddressesFrame(QWidget* parent)
    : QFrame(parent)
    , m_ui(new Ui::MyAddressesFrame)
//    , m_addressBookManager(nullptr)
    , mainWindow_(nullptr)
    , m_sortedAddressBookModel(nullptr)
    , walletModel_(nullptr)
{
    m_ui->setupUi(this);
    m_ui->m_addAddressButton->setIcon(QIcon(QStringLiteral(":/icons/address-generate.svg")));
    m_ui->m_addAddressButton->setIconSize(QSize(18, 18));
    m_ui->m_addAddressButton->setCursor(Qt::PointingHandCursor);
    m_ui->m_addAddressButton->setToolTip(tr(
        "Generate an address using this wallet's format. Legacy wallets create legacy addresses; "
        "HD/Amethyst wallets create addresses beginning with bcnZ."));

    auto* formatHint = new QLabel(tr("Wallet address format:"), this);
    formatHint->setObjectName(QStringLiteral("m_addressFormatHint"));
    auto* formatValue = new QLabel(this);
    formatValue->setObjectName(QStringLiteral("m_addressFormatValue"));
    formatValue->setMinimumWidth(190);

    if (auto* actionsLayout = findChild<QHBoxLayout*>(QStringLiteral("horizontalLayout")))
    {
        actionsLayout->insertWidget(0, formatHint);
        actionsLayout->insertWidget(1, formatValue);
    }
}

MyAddressesFrame::~MyAddressesFrame() {
}

void MyAddressesFrame::setAddressBookManager(IAddressBookManager* manager)
{
    m_addressBookManager = manager;
}

void MyAddressesFrame::setMainWindow(MainWindow* _mainWindow)
{
  mainWindow_ = _mainWindow;
}

void MyAddressesFrame::setWalletModel(WalletModel* model)
{
    if (walletModel_)
        disconnect(walletModel_, nullptr, this, nullptr);
    walletModel_ = model;
    if (walletModel_)
    {
        connect(walletModel_, &QAbstractItemModel::dataChanged,
            this, &MyAddressesFrame::updateAddressFormat);
        connect(walletModel_, &QAbstractItemModel::modelReset,
            this, &MyAddressesFrame::updateAddressFormat);
        connect(walletModel_, &WalletModel::statusUpdatedSignal,
            this, &MyAddressesFrame::updateAddressFormat);
    }
    updateAddressFormat();
}

void MyAddressesFrame::updateAddressFormat()
{
    auto* formatValue = findChild<QLabel*>(QStringLiteral("m_addressFormatValue"));
    if (!formatValue)
        return;

    const bool hasWallet = walletModel_ && walletModel_->isConnected();
    const bool isAmethyst = hasWallet && walletModel_->isAmethyst();
    formatValue->setText(!hasWallet
        ? tr("Not connected")
        : isAmethyst ? tr("bcnZ (HD / Amethyst)") : tr("Legacy (starts with 2)"));
    formatValue->setToolTip(!hasWallet
        ? tr("Create or open a Legacy wallet for addresses starting with 2, or create or open an HD/Amethyst wallet for bcnZ addresses. Find wallet options in the Wallet menu.")
        : isAmethyst
            ? tr("This HD/Amethyst wallet generates bcnZ addresses. To use Legacy addresses, open or create a separate Legacy wallet from the Wallet menu.")
            : tr("This Legacy wallet generates addresses beginning with 2. Address format cannot be changed for an individual address. To use bcnZ, open or create a separate HD/Amethyst wallet from the Wallet menu."));
}

void MyAddressesFrame::setSortedAddressBookModel(QAbstractItemModel* _model) {
  m_sortedAddressBookModel = _model;
  m_ui->m_addressBookView->setModel(m_sortedAddressBookModel);
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_LABEL, QHeaderView::Fixed);
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_ADDRESS, QHeaderView::Stretch);
  m_ui->m_addressBookView->header()->setSectionResizeMode(AddressBookModel::COLUMN_COPY_ACTION, QHeaderView::Fixed);
  m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_COPY_ACTION, 42);
  m_ui->m_addressBookView->header()->setResizeContentsPrecision(-1);
  m_ui->m_addressBookView->header()->resizeSection(AddressBookModel::COLUMN_LABEL, 250);
  m_ui->m_addressBookView->header()->setMinimumSectionSize(40);
  m_ui->m_addressBookView->setUniformRowHeights(true);
  m_ui->m_addressBookView->setMouseTracking(true);
  m_ui->m_addressBookView->viewport()->setMouseTracking(true);
  m_ui->m_addressBookView->setItemDelegateForColumn(AddressBookModel::COLUMN_COPY_ACTION,
      new AddressCopyDelegate([](const QString& address) {
          QApplication::clipboard()->setText(address);
      }, m_ui->m_addressBookView));
}

void MyAddressesFrame::addClicked() {
    m_addressBookManager->addAddress(QString{}, QString{});
}

void MyAddressesFrame::editClicked(const QPersistentModelIndex& _index) {
    NewMyAddressDialog dlg{m_addressBookManager, _index, mainWindow_};
    if (dlg.exec() == QDialog::Accepted) {
        QString label = dlg.getLabel();
        m_addressBookManager->editAddress(_index.data(AddressBookModel::ROLE_ROW).toInt(), label, QString{});
    }
}

void MyAddressesFrame::contextMenu(const QPoint& _pos) {
  QPersistentModelIndex index = m_ui->m_addressBookView->indexAt(_pos);
  if (!index.isValid()) {
    return;
  }

  QMenu menu;
  menu.setObjectName("m_addressBookMenu");
  QAction* editAction = new QAction(tr("Edit"), &menu);
  QAction* showQrAction = new QAction(tr("Show address QR"), &menu);
  QAction* copyAction = new QAction(tr("Copy to clipboard"), &menu);
  menu.addAction(editAction);
  menu.addAction(showQrAction);
  menu.addAction(copyAction);

  connect(editAction, &QAction::triggered, [this, index]() {
      Q_EMIT this->editClicked(index);
    });
  connect(showQrAction, &QAction::triggered, [this, index]() {
      QUrl paymentUri;
      paymentUri.setScheme(QStringLiteral("bytecoin"));
      paymentUri.setPath(index.data(AddressBookModel::ROLE_ADDRESS).toString());
      const QString label = index.data(AddressBookModel::ROLE_LABEL).toString();
      if (!label.isEmpty())
      {
          QUrlQuery query;
          query.addQueryItem(QStringLiteral("label"), label);
          paymentUri.setQuery(query);
      }
      showQrCodeDialog(this, paymentUri.toString(QUrl::FullyEncoded),
          index.data(AddressBookModel::ROLE_ADDRESS).toString(), tr("Receive BCN"));
    });
  connect(copyAction, &QAction::triggered, [index]() {
      QApplication::clipboard()->setText(index.data(AddressBookModel::ROLE_ADDRESS).toString());
    });

  menu.exec(m_ui->m_addressBookView->mapToGlobal(_pos) + QPoint(-10, 20));
}

}
