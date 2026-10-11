// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#pragma once

#include <QPixmap>
#include <QStatusBar>

class QLabel;
class QAbstractItemModel;
class QDataWidgetMapper;
class QTimer;

namespace WalletGUI {

class WalletModel;

class WalletStatusBar : public QStatusBar
{
    Q_OBJECT
    Q_DISABLE_COPY(WalletStatusBar)

public:
    explicit WalletStatusBar(QWidget* parent);
    virtual ~WalletStatusBar();

    void setWalletModel(WalletModel* model);

private:
    WalletModel* walletModel_;
    QLabel* m_syncStatusLabel;
    QLabel* m_syncStatusIconLabel;
    QLabel* m_peerCountIconLabel;
//    QLabel* m_encryptionStatusIconLabel;
//    QLabel* m_hdStatusLabel;
    QLabel* m_peerCountLabel;
    QLabel* m_walletConnectionIconLabel;
    QLabel* m_walletConnectionLabel;
    QLabel* m_bytecoindConnectionIconLabel;
    QLabel* m_bytecoindConnectionLabel;
    QDataWidgetMapper* stateMapper_;
    QTimer* updateTimer_;
    QTimer* m_syncAnimationTimer;
    QPixmap m_syncIconPixmap;
    int m_syncAngle;
    bool isSynchronized_;

    void nodeStateChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void updateStatusDescription();
    void updateSyncState();
    void animateSyncIcon();
    void updateEncryptedState(bool isEncrypted);
};

}
