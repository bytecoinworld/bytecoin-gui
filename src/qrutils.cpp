// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include "qrutils.h"

#include <QDialog>
#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPixmap>
#include <QFontDatabase>
#include <QToolTip>
#include <QTextOption>
#include <QVBoxLayout>

#include <BitMatrix.h>
#include <BarcodeFormat.h>
#include <MultiFormatWriter.h>

#include <exception>

namespace WalletGUI {

void showQrCodeDialog(QWidget* parent, const QString& contents, const QString& address, const QString& title)
{
    try
    {
        ZXing::MultiFormatWriter writer(ZXing::BarcodeFormat::QRCode);
        const ZXing::BitMatrix matrix = writer.setMargin(4).encode(contents.toUtf8().toStdString(), 360, 360);
        QImage image(matrix.width(), matrix.height(), QImage::Format_RGB32);
        for (int y = 0; y < matrix.height(); ++y)
            for (int x = 0; x < matrix.width(); ++x)
                image.setPixel(x, y, matrix.get(x, y) ? qRgb(0, 0, 0) : qRgb(255, 255, 255));

        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        QVBoxLayout layout(&dialog);
        QLabel imageLabel(&dialog);
        imageLabel.setAlignment(Qt::AlignCenter);
        imageLabel.setPixmap(QPixmap::fromImage(image));
        QLabel addressCaption(QObject::tr("Wallet address"), &dialog);
        QPlainTextEdit addressField(&dialog);
        addressField.setPlainText(address);
        addressField.setReadOnly(true);
        addressField.setLineWrapMode(QPlainTextEdit::WidgetWidth);
        addressField.setWordWrapMode(QTextOption::WrapAnywhere);
        addressField.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        addressField.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        addressField.setFixedHeight(72);
        addressField.setToolTip(address);
        addressField.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        addressField.setStyleSheet(QStringLiteral(
            "QPlainTextEdit { padding: 6px; border: 1px solid palette(mid); border-radius: 4px; }"));

        QHBoxLayout buttonsLayout;
        QPushButton saveButton(QObject::tr("Save QR image..."), &dialog);
        QPushButton copyButton(QObject::tr("Copy address"), &dialog);
        buttonsLayout.addWidget(&saveButton);
        buttonsLayout.addWidget(&copyButton);

        layout.addWidget(&imageLabel);
        layout.addWidget(&addressCaption);
        layout.addWidget(&addressField);
        layout.addLayout(&buttonsLayout);

        QObject::connect(&saveButton, &QPushButton::clicked, &dialog, [&dialog, &image]() {
            const QString fileName = QFileDialog::getSaveFileName(
                &dialog, QObject::tr("Save QR image"), QStringLiteral("bytecoin-address-qr.png"),
                QObject::tr("PNG image (*.png)"));
            if (fileName.isEmpty())
                return;

            if (!image.save(fileName, "PNG"))
                QMessageBox::warning(&dialog, QObject::tr("Save QR image"),
                    QObject::tr("The QR image could not be saved to the selected file."));
        });
        QObject::connect(&copyButton, &QPushButton::clicked, &dialog, [address, &dialog]() {
            QApplication::clipboard()->setText(address);
            QToolTip::showText(QCursor::pos(), QObject::tr("Wallet address copied"), &dialog);
        });
        dialog.exec();
    }
    catch (const std::exception& error)
    {
        QMessageBox::critical(parent, QObject::tr("QR generation failed"), QString::fromLocal8Bit(error.what()));
    }
}

}
