// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "application.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    WalletGUI::WalletApplication app(argc, argv);
    try
    {
        if (!app.init())
            return 0;
        return app.exec();
    }
    catch (const std::exception& _error)
    {
        QMessageBox::critical(nullptr, QObject::tr("Wallet error"),
                              QString::fromLocal8Bit(_error.what()));
        return 1;
    }
}
