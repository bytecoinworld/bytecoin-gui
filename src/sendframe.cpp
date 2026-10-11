// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QMetaMethod>
#include <QUrl>
#include <QUrlQuery>
#include <QStyle>
#include <QFileDialog>
#include <QImage>
#include <QIcon>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QPushButton>
#include <QSet>
#include <QRegularExpression>
#include <exception>
#include <string>

#include <BarcodeFormat.h>
#include <DecodeHints.h>
#include <ImageView.h>
#include <ReadBarcode.h>

#include "sendframe.h"
#include "transferframe.h"
#include "walletmodel.h"
#include "mainwindow.h"
#include "common.h"
#include "rpcapi.h"
#include "addressbookmanager.h"
#include "qrutils.h"

#include "ui_sendframe.h"

namespace WalletGUI
{

namespace {

//const char SEND_FRAME_STYLE_SHEET[] =
//  "WalletGui--SendFrame {"
//    "background-color: #ffffff;"
//    "border: none;"
//  "}"

//  "WalletGui--SendFrame #m_sendScrollarea {"
//    "background-color: %backgroundColorGray%;"
//    "border: none;"
//    "border-bottom: 1px solid %borderColor%;"
//  "}"

//  "WalletGui--SendFrame #m_sendScrollarea QScrollBar:vertical {"
//    "margin-top: 0px;"
//    "border-top: none;"
//  "}"

//  "WalletGui--SendFrame #scrollAreaWidgetContents {"
//    "background-color: #ffffff;"
//    "border: none;"
//  "}"

//  "WalletGui--SendFrame #m_sendFeeFrame {"
//    "border: none;"
//    "border-top: 1px solid %borderColor%;"
//    "border-bottom: 1px solid %borderColor%;"
//  "}";

//const char PAYMENT_URL_AMOUNT_TAG[] = "amount";
//const char PAYMENT_URL_PAYMENT_ID_TAG[] = "payment_id";
//const char PAYMENT_URL_MESSAGE_TAG[] = "message";
//const char PAYMENT_URL_LABEL_TAG[] = "label";
const char MIXIN_CRITICAL_COLOR[] = "#ea161f";
const char MIXIN_NORMAL_COLOR[] = "#fdce00";
const char MIXIN_GOOD_COLOR[] = "#6ca025";
const char MIXIN_SLIDER_STYLE_SHEET_TEMPLATE[] =
  "QSlider::sub-page:horizontal {"
      "background: %COLOR%;"
      "border: 1px solid %COLOR%;"
  "}";

bool isValidPaymentId(const QString& /*paymentIdString*/)
{
    return true;
}

}

SendFrame::SendFrame(QWidget* parent)
    : QFrame(parent)
    , m_ui(new Ui::SendFrame)
    , m_mainWindow(nullptr)
    , walletModel_(nullptr)
    , addressBookModel_(nullptr)
    , addressBookManager_(nullptr)
{
    m_ui->setupUi(this);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    auto* settingsLayout = m_ui->m_sendFeeFrame->findChild<QGridLayout*>(QStringLiteral("gridLayout_2"));
    if (settingsLayout)
    {
        if (QLayoutItem* oldActions = settingsLayout->itemAtPosition(0, 2))
        {
            settingsLayout->removeItem(oldActions);
            delete oldActions;
        }
    }

    auto* recipientHeader = new QFrame(this);
    recipientHeader->setObjectName(QStringLiteral("m_sendRecipientHeader"));
    auto* headerLayout = new QHBoxLayout(recipientHeader);
    headerLayout->setContentsMargins(18, 12, 18, 12);
    headerLayout->setSpacing(8);

    auto* headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(2);
    auto* recipientTitle = new QLabel(tr("Recipients"), recipientHeader);
    recipientTitle->setObjectName(QStringLiteral("m_sendSectionTitle"));
    auto* recipientHint = new QLabel(tr("Enter the Bytecoin address and amount for each payment."), recipientHeader);
    recipientHint->setObjectName(QStringLiteral("m_sendSectionHint"));
    recipientHint->setWordWrap(true);
    auto* availableBalanceLabel = new QLabel(recipientHeader);
    availableBalanceLabel->setObjectName(QStringLiteral("m_sendAvailableBalance"));
    availableBalanceLabel->setText(tr("Available balance: %1").arg(tr("Not connected")));
    headerTextLayout->addWidget(recipientTitle);
    headerTextLayout->addWidget(recipientHint);
    headerTextLayout->addWidget(availableBalanceLabel);
    headerLayout->addLayout(headerTextLayout, 1);

    m_ui->m_pastePaymentUriButton->setIcon(QIcon(QStringLiteral(":/icons/send/paste.svg")));
    m_ui->m_scanPaymentQrButton->setIcon(QIcon(QStringLiteral(":/icons/send/qr.svg")));
    m_ui->m_addRecipientButton->setIcon(QIcon(QStringLiteral(":/icons/send/add.svg")));
    m_ui->m_pastePaymentUriButton->setIconSize(QSize(17, 17));
    m_ui->m_scanPaymentQrButton->setIconSize(QSize(17, 17));
    m_ui->m_addRecipientButton->setIconSize(QSize(17, 17));
    m_ui->m_addRecipientButton->setProperty("sendAction", QStringLiteral("primary"));
    headerLayout->addWidget(m_ui->m_pastePaymentUriButton);
    headerLayout->addWidget(m_ui->m_scanPaymentQrButton);
    headerLayout->addWidget(m_ui->m_addRecipientButton);

    if (mainLayout)
    {
        if (mainLayout->count() > 1)
        {
            QLayoutItem* spacer = mainLayout->takeAt(1);
            delete spacer;
        }
        mainLayout->insertWidget(0, recipientHeader);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(18, 14, 18, 16);
    }

    m_ui->m_transfersLayout->setContentsMargins(8, 8, 8, 8);
    m_ui->m_transfersLayout->setSpacing(10);
    m_ui->m_sendScrollarea->setMinimumHeight(140);
    m_ui->m_sendScrollarea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (settingsLayout)
    {
        settingsLayout->setContentsMargins(16, 14, 16, 16);
        settingsLayout->setHorizontalSpacing(16);
        settingsLayout->setVerticalSpacing(10);
    }
    if (auto* privacyLayout = qobject_cast<QGridLayout*>(m_ui->m_mixinFrame->layout()))
    {
        privacyLayout->setContentsMargins(18, 16, 18, 16);
        privacyLayout->setHorizontalSpacing(14);
        privacyLayout->setVerticalSpacing(8);
    }

    m_ui->m_mixinTextLabel->setText(tr("Privacy level"));
    m_ui->m_mixinTextLabel->setToolTip(tr("Choose the transaction anonymity level. Larger values can increase transaction size and fee."));
    m_ui->m_paymentIdTextLabel->setText(tr("Payment ID (optional)"));
    m_ui->m_paymentIdEdit->setPlaceholderText(tr("Enter an ID only if the recipient requested one"));
    m_ui->label_2->setText(tr("Total to send"));
    m_ui->label_2->setObjectName(QStringLiteral("m_sendTotalCaption"));
    m_ui->m_totalAmountLabel->setObjectName(QStringLiteral("m_sendTotalAmount"));
    m_ui->m_sendButton->setIcon(QIcon(QStringLiteral(":/icons/send/arrow.svg")));
    m_ui->m_sendButton->setIconSize(QSize(18, 18));
    m_ui->m_sendButton->setMinimumWidth(176);
    m_ui->m_sendButton->setMaximumWidth(QWIDGETSIZE_MAX);
    m_ui->m_sendButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ui->m_sendButton->setMinimumHeight(46);
    m_ui->m_sendButton->setCursor(Qt::PointingHandCursor);

    m_ui->m_mixinSpin->setMaximum(MAX_MIXIN_VALUE);
    connect(m_ui->m_scanPaymentQrButton, &QPushButton::clicked, this, &SendFrame::scanPaymentQr);
    connect(m_ui->m_pastePaymentUriButton, &QPushButton::clicked, this, &SendFrame::pastePaymentUri);
    mixinValueChanged(m_ui->m_mixinSlider->value());
//    setStyleSheet(Settings::instance().getCurrentStyle().makeStyleSheet(SEND_FRAME_STYLE_SHEET));

    addRecipientClicked();
}

SendFrame::~SendFrame()
{
    m_transfers.clear();
}

void SendFrame::addRecipient(const QString& address, const QString& label)
{
    if (m_transfers.size() == 1 && m_transfers[0]->getAddress().isEmpty())
    {
        m_transfers[0]->setAddress(address);
        if (!label.isEmpty())
            m_transfers[0]->setLabel(label);
    }
    else
    {
        addRecipientClicked();
        m_ui->m_sendScrollarea->widget()->updateGeometry();
        m_ui->m_sendScrollarea->updateGeometry();
        m_transfers.last()->setAddress(address);
        if (!label.isEmpty())
            m_transfers.last()->setLabel(label);
    }
}

void SendFrame::setMainWindow(MainWindow* mainWindow)
{
    m_mainWindow = mainWindow;
    for (TransferFrame* transfer : m_transfers)
        transfer->setMainWindow(mainWindow);
}

void SendFrame::setAddressBookModel(QAbstractItemModel* model)
{
    addressBookModel_ = model;
    for (TransferFrame* transfer : m_transfers)
        transfer->setAddressBookModel(model);
}

void SendFrame::setAddressBookManager(AddressBookManager* manager)
{
    addressBookManager_ = manager;
    for (TransferFrame* transfer : m_transfers)
        transfer->setAddressBookManager(manager);
}

void SendFrame::setWalletModel(WalletModel *model)
{
    if (walletModel_)
        disconnect(walletModel_, nullptr, this, nullptr);
    walletModel_ = model;
    if (walletModel_)
        connect(walletModel_, &WalletModel::dataChanged, this, &SendFrame::updateFee);
    updateFee(QModelIndex(), QModelIndex(), {});
    feeValueChanged(m_ui->m_feeSlider->value());
}

void SendFrame::updateFee(const QModelIndex &/*topLeft*/, const QModelIndex &/*bottomRight*/, const QVector<int> &roles)
{
    auto* availableBalanceLabel = findChild<QLabel*>(QStringLiteral("m_sendAvailableBalance"));
    if (availableBalanceLabel)
    {
        if (!walletModel_ || !walletModel_->isConnected())
        {
            availableBalanceLabel->setText(tr("Available balance: %1").arg(tr("Not connected")));
        }
        else
        {
            const quint64 spendable = walletModel_->data(
                walletModel_->index(0, WalletModel::COLUMN_SPENDABLE),
                WalletModel::ROLE_SPENDABLE).toULongLong();
            availableBalanceLabel->setText(
                tr("Available balance: %1").arg(formatUnsignedAmount(spendable)));
        }
    }

    if (roles.contains(WalletModel::ROLE_RECOMMENDED_FEE_PER_BYTE))
        feeValueChanged(m_ui->m_feeSlider->value());
}

void SendFrame::addRecipientClicked()
{
    TransferFrame* newTransferFrame = new TransferFrame(m_ui->m_sendScrollarea->widget());
    newTransferFrame->setMainWindow(m_mainWindow);
    newTransferFrame->setAddressBookModel(addressBookModel_);
    newTransferFrame->setAddressBookManager(addressBookManager_);
    m_ui->m_transfersLayout->insertWidget(m_transfers.size(), newTransferFrame);
    m_transfers.append(newTransferFrame);
    m_transfers.first()->hideBorder();
    if (m_transfers.size() == 1)
         newTransferFrame->disableRemoveButton(true);
    else
        m_transfers[0]->disableRemoveButton(false);

    connect(newTransferFrame, &TransferFrame::amountStringChangedSignal, this, &SendFrame::amountStringChanged);
    connect(newTransferFrame, &TransferFrame::addressChangedSignal, this, &SendFrame::addressChanged);
    connect(newTransferFrame, &TransferFrame::destroyed, [this, newTransferFrame](QObject* obj) {
        if (m_transfers.isEmpty())
            return;

        Q_ASSERT(newTransferFrame == obj);
        m_transfers.removeOne(/*static_cast<TransferFrame*>(obj)*/newTransferFrame);

        if (m_transfers.size() == 1)
            m_transfers[0]->disableRemoveButton(true);

        m_transfers.first()->hideBorder();
        m_ui->m_transfersLayout->removeWidget(newTransferFrame);
        m_ui->m_sendScrollarea->widget()->updateGeometry();
        m_ui->m_sendScrollarea->updateGeometry();
        amountStringChanged(QString());
        m_ui->m_sendButton->setEnabled(readyToSend());
    });

    amountStringChanged(QString());
    m_ui->m_sendScrollarea->widget()->adjustSize();
    m_ui->m_sendScrollarea->widget()->updateGeometry();
    m_ui->m_sendScrollarea->updateGeometry();
    m_ui->m_sendButton->setEnabled(false);
}

void SendFrame::clearAll()
{
    for (TransferFrame* transfer : m_transfers)
    {
        m_ui->m_transfersLayout->removeWidget(transfer);
        transfer->close();
        transfer->deleteLater();
    }

    QApplication::processEvents();
    m_transfers.clear();
//    m_ui->m_feeSpin->setValue(m_ui->m_feeSpin->minimum());
    m_ui->m_paymentIdEdit->clear();
    m_ui->m_mixinSlider->setValue(DEFAULT_MIXIN_VALUE);
    m_ui->m_sendScrollarea->widget()->adjustSize();
    m_ui->m_sendScrollarea->widget()->updateGeometry();
    m_ui->m_sendScrollarea->updateGeometry();

    m_ui->m_sendButton->setText(tr("Send"));
    addRecipientClicked();
}

void SendFrame::cancelSend()
{
    m_ui->m_sendButton->setText(tr("Send"));
    m_ui->m_sendButton->setEnabled(readyToSend());
}

void SendFrame::sendClicked()
{
//    Q_ASSERT(m_mainWindow != nullptr);
    Q_ASSERT(walletModel_ != nullptr);

    m_ui->m_sendButton->setEnabled(false);
    m_ui->m_sendButton->setText(tr("Checking..."));

    quint64 transferSum = 0;
    QList<RpcApi::Transfer> trs;
    for (TransferFrame* transfer : m_transfers)
    {
        const QString address = transfer->getAddress();
        const QString label = transfer->getLabel();
        if (!label.isEmpty() && addressBookManager_->findAddressByAddress(address) == INVALID_ADDRESS_INDEX &&
                                addressBookManager_->findAddressByLabel(label) == INVALID_ADDRESS_INDEX)
            addressBookManager_->addAddress(label, address);

//        const double damount = transfer->getAmountString().toDouble();
        qint64 amount = 0;
        if (!parseAmount(transfer->getAmountString(), amount))
            return;
        if (amount <= 0)
        {
            transfer->setAmountFormatError(true);
            m_ui->m_sendScrollarea->ensureWidgetVisible(transfer);
            m_ui->m_sendButton->setEnabled(readyToSend());
            m_ui->m_sendButton->setText(tr("Send"));
            return;
        }
        transferSum += amount;

        RpcApi::Transfer tr;
        tr.address = address;
        tr.amount = amount;
        trs << tr;
    }
    const QString paymentId = m_ui->m_paymentIdEdit->text();

    RpcApi::Transaction tx;
    tx.transfers = trs;
    tx.payment_id = paymentId;
    tx.anonymity = m_ui->m_mixinSlider->value();
    tx.unlock_block_or_timestamp = 0;

    emit createTxSignal(tx, getFeeFromSlider(m_ui->m_feeSlider->value()), m_ui->m_subtractFeeFromAmountCheckBox->isChecked());
}

void SendFrame::mixinValueChanged(int value)
{
    if (m_ui->m_mixinSlider->value() != value)
        m_ui->m_mixinSlider->setValue(value);

    if (m_ui->m_mixinSpin->value() != value)
        m_ui->m_mixinSpin->setValue(value);

    setMixinError(false);
    updateMixinSliderStyleSheet();
}

void SendFrame::feeValueChanged(int value)
{
    const quint64 fee = getFeeFromSlider(value);
    static const QStringList priorities = {
        tr("No fee"), tr("Low"), tr("Recommended"), tr("High"), tr("Highest")
    };
    m_ui->m_feeTextLabel->setText(tr("%1 fee rate: %2 atomic units per byte")
        .arg(priorities.value(value, tr("Recommended")))
        .arg(fee));
}

void SendFrame::scanPaymentQr()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("Scan payment QR image"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif);;All files (*)"));
    if (fileName.isEmpty())
        return;

    const QImage image(fileName);
    if (image.isNull())
    {
        QMessageBox::critical(this, tr("QR scan failed"), tr("The selected image could not be read."));
        return;
    }

    const QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    const ZXing::ImageView imageView(
        rgb.constBits(), rgb.width(), rgb.height(), ZXing::ImageFormat::RGB,
        rgb.bytesPerLine());
    ZXing::DecodeHints hints;
    try
    {
        const ZXing::Result result = ZXing::ReadBarcode(
            imageView, hints.setFormats(ZXing::BarcodeFormat::QRCode).setTryHarder(true));
        if (!result.isValid())
        {
            QMessageBox::warning(this, tr("QR scan failed"), tr("No readable QR code was found in the selected image."));
            return;
        }

        if (!applyPaymentUri(QString::fromStdWString(result.text())))
            return;
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(this, tr("QR scan failed"), QString::fromLocal8Bit(error.what()));
    }
}

void SendFrame::pastePaymentUri()
{
    const QString paymentUri = QApplication::clipboard()->text().trimmed();
    if (!paymentUri.isEmpty())
        applyPaymentUri(paymentUri);
}

bool SendFrame::applyPaymentUri(const QString& paymentUri)
{
    const QUrl uri = QUrl::fromEncoded(paymentUri.toUtf8(), QUrl::StrictMode);
    const QString scheme = uri.scheme();
    if (!uri.isValid() || uri.hasFragment()
        || (!scheme.isEmpty() && scheme.compare(QStringLiteral("bytecoin"), Qt::CaseInsensitive) != 0))
    {
        QMessageBox::warning(this, tr("Invalid payment request"), tr("The QR code is not a valid Bytecoin payment URI."));
        return false;
    }

    const QString address = uri.path().isEmpty() ? uri.host() : uri.path();
    if (address.isEmpty())
    {
        QMessageBox::warning(this, tr("Invalid payment request"), tr("The payment request does not contain a recipient address."));
        return false;
    }
    if (address.contains(QLatin1Char('/')) || address.contains(QLatin1Char('?')))
    {
        QMessageBox::warning(this, tr("Invalid payment request"), tr("The payment request contains an invalid recipient address."));
        return false;
    }

    QUrlQuery query(uri);
    QString amount;
    QString paymentId;
    QString label;
    bool hasAmount = false;
    QSet<QString> seen;
    const auto items = query.queryItems(QUrl::FullyDecoded);
    for (const auto& item : items)
    {
        const QString key = item.first.toLower();
        if (seen.contains(key))
        {
            QMessageBox::warning(this, tr("Invalid payment request"), tr("The payment request contains a duplicate '%1' field.").arg(key));
            return false;
        }
        seen.insert(key);
        if (key == QStringLiteral("amount"))
        {
            hasAmount = true;
            amount = item.second;
        }
        else if (key == QStringLiteral("payment_id"))
            paymentId = item.second;
        else if (key == QStringLiteral("label"))
            label = item.second;
        else
        {
            QMessageBox::warning(this, tr("Unsupported payment request"), tr("This wallet cannot apply the '%1' field.").arg(key));
            return false;
        }
    }

    qint64 atomicAmount = 0;
    if (hasAmount && (amount.isEmpty() || !parseAmount(amount, atomicAmount) || atomicAmount <= 0))
    {
        QMessageBox::warning(this, tr("Invalid payment request"), tr("The requested amount is not a valid positive BCN amount."));
        return false;
    }

    clearAll();
    addRecipient(address, label);
    if (hasAmount)
    {
        bool amountOk = false;
        const qreal amountValue = amount.toDouble(&amountOk);
        if (!amountOk)
        {
            QMessageBox::warning(this, tr("Invalid payment request"), tr("The requested amount cannot be represented by this wallet."));
            return false;
        }
        m_transfers.first()->setAmount(amountValue);
        if (m_transfers.first()->getAmount() != static_cast<quint64>(atomicAmount))
        {
            clearAll();
            QMessageBox::warning(this, tr("Invalid payment request"),
                tr("The requested amount exceeds the precision supported by the amount field."));
            return false;
        }
    }
    m_ui->m_paymentIdEdit->setText(paymentId);
    m_ui->m_sendButton->setEnabled(readyToSend());
    return true;
}

quint64 SendFrame::getFeeFromSlider(int sliderValue) const
{
    Q_ASSERT(walletModel_ != nullptr);

    const quint64 recommendedFee = walletModel_->getRecommendedFeePerByte();
    quint64 fee = 0;
    switch(sliderValue)
    {
    case 0: fee = 0;                    break; // 0%
    case 1: fee = recommendedFee * 1/2; break; // 50%
    case 2: fee = recommendedFee;       break; // 100%
    case 3: fee = recommendedFee * 3/2; break; // 150%
    case 4: fee = recommendedFee * 2;   break; // 200%
    default: fee = recommendedFee; break;    // 100%
    }
    return fee;
}

void SendFrame::validatePaymentId(const QString& paymentId)
{
    bool validPaymentId = isValidPaymentId(paymentId);
    if (!validPaymentId)
    {
        setPaymentIdError(true);
        m_ui->m_sendButton->setEnabled(false);
        return;
    }

    setPaymentIdError(false);
    m_ui->m_sendButton->setEnabled(readyToSend());
}

void SendFrame::setPaymentIdError(bool error)
{
    m_ui->m_paymentIdEdit->setProperty("errorState", error);
    m_ui->m_paymentIdTextLabel->setProperty("errorState", error);
    if (error)
        m_ui->m_paymentIdTextLabel->setText(tr("INVALID FORMAT"));
    else
        m_ui->m_paymentIdTextLabel->setText(tr("Payment ID"));

    m_ui->m_paymentIdEdit->style()->unpolish(m_ui->m_paymentIdEdit);
    m_ui->m_paymentIdEdit->style()->polish(m_ui->m_paymentIdEdit);

    m_ui->m_paymentIdTextLabel->style()->unpolish(m_ui->m_paymentIdTextLabel);
    m_ui->m_paymentIdTextLabel->style()->polish(m_ui->m_paymentIdTextLabel);
    m_ui->m_paymentIdTextLabel->update();
}

void SendFrame::setMixinError(bool error)
{
    m_ui->m_mixinTextLabel->setProperty("errorState", error);
    if (error)
        m_ui->m_mixinTextLabel->setText(tr("MIXIN IS TOO BIG"));
    else
        m_ui->m_mixinTextLabel->setText(tr("Anonymity level"));

    m_ui->m_mixinTextLabel->style()->unpolish(m_ui->m_mixinTextLabel);
    m_ui->m_mixinTextLabel->style()->polish(m_ui->m_mixinTextLabel);
    m_ui->m_mixinTextLabel->update();
}

void SendFrame::setFeeFormatError(bool /*error*/)
{
}

void SendFrame::updateMixinSliderStyleSheet()
{
    int mixinValue = m_ui->m_mixinSpin->value();
    QString color = MIXIN_GOOD_COLOR;
    if (mixinValue < CRITICAL_MIXIN_BOUND)
        color = MIXIN_CRITICAL_COLOR;
    else if (mixinValue < NORMAL_MIXIN_BOUND)
        color = MIXIN_NORMAL_COLOR;

    QString mixinSliderStyleSheet = QString(MIXIN_SLIDER_STYLE_SHEET_TEMPLATE).replace("%COLOR%", color);
    m_ui->m_mixinSlider->setStyleSheet(mixinSliderStyleSheet);
}

void SendFrame::amountStringChanged(const QString& /*amountString*/)
{
    quint64 amount = 0;
    for (TransferFrame* transfer : m_transfers)
        amount += transfer->getAmount();

//    m_ui->m_totalAmountLabel->setText(QString("%1 %2")
//            .arg(formatUnsignedAmount(amount, false /*trim*/))
//            .arg(QString(CURRENCY_TICKER).toUpper()));

    m_ui->m_totalAmountLabel->setText(formatUnsignedAmount(amount, false /*trim*/));
    m_ui->m_sendButton->setEnabled(readyToSend());
}

void SendFrame::addressChanged(const QString& /*address*/)
{
    m_ui->m_sendButton->setEnabled(readyToSend());
}

bool SendFrame::readyToSend() const
{
    bool readyToSend = true;
    for (const auto& transfer : m_transfers)
        readyToSend = readyToSend && transfer->readyToSend();

    readyToSend = readyToSend && isValidPaymentId(m_ui->m_paymentIdEdit->text());
    return readyToSend;
}

}
