#-------------------------------------------------
#
# Project created by QtCreator 2017-10-02T15:01:03
#
#-------------------------------------------------

QT       += core gui network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bytecoin-gui
TEMPLATE = app

!win32: QMAKE_CXXFLAGS += -std=c++14 -Wall -Wextra -pedantic
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.13
macx: ICON = images/bytecoin.icns
win32: RC_ICONS = images/bytecoin.ico
win32: VERSION = 3.6.1.0

#QMAKE_CXXFLAGS += -fno-omit-frame-pointer -fsanitize=address,undefined
#LIBS += -lasan -lubsan

CONFIG += c++14 strict_c++ no-opengl
DEFINES += QT_FORCE_ASSERTS

DESTDIR = $$PWD/../bin

# copy walletd adjacent to bytecoin-gui binary on all 3 platforms
win32 {
WALLETD_BY_SRC_PATH = $$shell_path($$clean_path("$$PWD/../../bytecoin/bin/walletd.exe"))
WALLETD2_BY_SRC_PATH = $$shell_path($$clean_path("$$PWD/../../bytecoin/bin/walletd.pdb"))
BYTECOIND_BY_SRC_PATH = $$shell_path($$clean_path("$$PWD/../../bytecoin/bin/bytecoind.exe"))
BYTECOIND2_BY_SRC_PATH = $$shell_path($$clean_path("$$PWD/../../bytecoin/bin/bytecoind.pdb"))
Debug:BY_DST_PATH = $$shell_path($$clean_path("$$DESTDIR"))
Release:BY_DST_PATH = $$shell_path($$clean_path("$$DESTDIR"))
copywalletd.commands = $(COPY_FILE) $${WALLETD_BY_SRC_PATH} $${BY_DST_PATH}
copywalletd2.commands = $(COPY_FILE) $${WALLETD2_BY_SRC_PATH} $${BY_DST_PATH}
copybytecoind.commands = $(COPY_FILE) $${BYTECOIND_BY_SRC_PATH} $${BY_DST_PATH}
copybytecoind2.commands = $(COPY_FILE) $${BYTECOIND2_BY_SRC_PATH} $${BY_DST_PATH}
first.depends = $(first) copywalletd copywalletd2 copybytecoind copybytecoind2
QMAKE_EXTRA_TARGETS += first copywalletd copywalletd2 copybytecoind copybytecoind2
}else:macx {
copywalletd.commands += $(COPY_FILE) $$PWD/../../bytecoin/bin/walletd $$DESTDIR/bytecoin-gui.app/Contents/MacOS
copybytecoind.commands += $(COPY_FILE) $$PWD/../../bytecoin/bin/bytecoind $$DESTDIR/bytecoin-gui.app/Contents/MacOS
first.depends = copywalletd copybytecoind
copywalletd.depends = $(TARGET)
copybytecoind.depends = $(TARGET)
QMAKE_EXTRA_TARGETS += first copywalletd copybytecoind
}else {
copywalletd.commands += $(COPY_FILE) $$PWD/../../bytecoin/bin/walletd $$DESTDIR
copybytecoind.commands += $(COPY_FILE) $$PWD/../../bytecoin/bin/bytecoind $$DESTDIR
first.depends = $(first) copywalletd copybytecoind
QMAKE_EXTRA_TARGETS += first copywalletd copybytecoind
}
#export(first.depends)
#export(copywalletd.commands)
#export(copybytecoind.commands)

SOURCES += main.cpp\
    mainwindow.cpp \
    signalhandler.cpp \
    overviewframe.cpp \
    aboutdialog.cpp \
    JsonRpc/JsonRpcClient.cpp \
    JsonRpc/JsonRpcNotification.cpp \
    JsonRpc/JsonRpcObject.cpp \
    JsonRpc/JsonRpcObjectFactory.cpp \
    JsonRpc/JsonRpcRequest.cpp \
    JsonRpc/JsonRpcResponse.cpp \
    application.cpp \
    logger.cpp \
    okbutton.cpp \
    statusbar.cpp \
    windoweditemmodel.cpp \
    walletmodel.cpp \
    sendframe.cpp \
    transferframe.cpp \
    resizablescrollarea.cpp \
    common.cpp \
    connectionoptionsframe.cpp \
    settings.cpp \
    connectselectiondialog.cpp \
    walletd.cpp \
    rpcapi.cpp \
    progressbar.cpp \
    addressbookframe.cpp \
    addressbookmodel.cpp \
    addressbooksortedmodel.cpp \
    newaddressdialog.cpp \
    addressbookmanager.cpp \
    balanceoverviewframe.cpp \
    crashdialog.cpp \
    changepassworddialog.cpp \
    sendconfirmationdialog.cpp \
    addressbookdialog.cpp \
    popup.cpp \
    logframe.cpp \
    askpassworddialog.cpp \
    importkeydialog.cpp \
    questiondialog.cpp \
    createproofdialog.cpp \
    checkproofdialog.cpp \
    walletdparamsdialog.cpp \
    exportkeydialog.cpp \
    filedownloader.cpp \
    version.cpp \
    mnemonicdialog.cpp \
    elidedlabel.cpp \
    myaddressesframe.cpp \
    newmyaddressdialog.cpp \
    qrutils.cpp

HEADERS  += mainwindow.h \
    signalhandler.h \
    overviewframe.h \
    aboutdialog.h \
    JsonRpc/JsonRpcClient.h \
    JsonRpc/JsonRpcNotification.h \
    JsonRpc/JsonRpcObject.h \
    JsonRpc/JsonRpcObjectFactory.h \
    JsonRpc/JsonRpcRequest.h \
    JsonRpc/JsonRpcResponse.h \
    application.h \
    logger.h \
    okbutton.h \
    statusbar.h \
    windoweditemmodel.h \
    walletmodel.h \
    sendframe.h \
    transferframe.h \
    resizablescrollarea.h \
    common.h \
    connectionoptionsframe.h \
    settings.h \
    connectselectiondialog.h \
    walletd.h \
    rpcapi.h \
    progressbar.h \
    addressbookframe.h \
    addressbookmodel.h \
    addressbooksortedmodel.h \
    newaddressdialog.h \
    addressbookmanager.h \
    balanceoverviewframe.h \
    crashdialog.h \
    changepassworddialog.h \
    sendconfirmationdialog.h \
    addressbookdialog.h \
    popup.h \
    logframe.h \
    askpassworddialog.h \
    importkeydialog.h \
    questiondialog.h \
    createproofdialog.h \
    checkproofdialog.h \
    walletdparamsdialog.h \
    exportkeydialog.h \
    version.h \
    filedownloader.h \
    mnemonicdialog.h \
    elidedlabel.h \
    myaddressesframe.h \
    newmyaddressdialog.h \
    qrutils.h

FORMS    += mainwindow.ui \
    overviewframe.ui \
    aboutdialog.ui \
    sendframe.ui \
    transferframe.ui \
    connectionoptionsframe.ui \
    optionsdialog.ui \
    connectselectiondialog.ui \
    addressbookframe.ui \
    newaddressdialog.ui \
    balanceoverviewframe.ui \
    crashdialog.ui \
    changepassworddialog.ui \
    addressbookdialog.ui \
    logframe.ui \
    askpassworddialog.ui \
    importkeydialog.ui \
    questiondialog.ui \
    createproofdialog.ui \
    checkproofdialog.ui \
    walletdparamsdialog.ui \
    exportkeydialog.ui \
    mnemonicdialog.ui \
    myaddressesframe.ui \
    newmyaddressdialog.ui

RESOURCES += \
    resources.qrc \

ZXING_ROOT = $$(ZXING_ROOT)
isEmpty(ZXING_ROOT): ZXING_ROOT = $$OUT_PWD/../zxing-install
!exists($$ZXING_ROOT/include/ZXing/ReadBarcode.h): error("ZXing-C++ 1.4.0 is required. Set ZXING_ROOT to its installation prefix.")
INCLUDEPATH += $$ZXING_ROOT/include/ZXing
LIBS += -L$$ZXING_ROOT/lib -lZXing
unix: LIBS += -pthread

unix|win32: LIBS += -L$$PWD/../../bytecoin/libs/ -lbytecoin-crypto

INCLUDEPATH += $$PWD/../../bytecoin/src
DEPENDPATH += $$PWD/../../bytecoin/src

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../bytecoin/libs/bytecoin-crypto.lib
else:unix|win32-g++: PRE_TARGETDEPS += $$PWD/../../bytecoin/libs/libbytecoin-crypto.a

# to add necessary dependencies,
# 1. delete built bytecoin-gui.app to delete old dependencies (dylibs and frameworks)
# 2. build
# 3. run /Users/user/Qt/5.9.2/clang_64/bin/macdeployqt bytecoin-gui.app
# P.S. in 3, change path accroding to your Qt installation location
