// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#pragma once

class QWidget;
class QString;

namespace WalletGUI {

void showQrCodeDialog(QWidget* parent, const QString& contents, const QString& address, const QString& title);

}
