# Bytecoin Wallet

**bytecoin.world Community Edition · Version 3.6.1**

A desktop wallet for Bytecoin, maintained by the bytecoin.world community.

[Browse releases](https://github.com/bytecoinworld/bytecoin-gui/releases)
· [Visit bytecoin.world](https://bytecoin.world/)
· [Open the Bytecoin Explorer](https://explorer.bytecoin.world/)

## Features

- Create, open, and restore wallets; view balances and transaction history.
- Receive using a wallet address and QR code. Read payment URIs and QR-code
  images when preparing a payment.
- Review payment recipients, amounts, fees, and privacy settings before
  sending.
- Search and filter activity, inspect transaction details, and export history.
- Create and verify transaction send proofs.
- Configure wallet-daemon connections and saved connection profiles.
- Choose **Tools → Light mode** to switch between light and dark appearances.
  The selected appearance is saved and restored on the next launch.
- Pool mining is not included in this wallet.

## Windows downloads

Download the **3.6.1 Windows installer** or the portable package from the
[releases page][releases]. Run the MSI and follow its prompts. To use the
portable version, extract the entire ZIP to a folder and run `bytecoin-gui.exe`
from there. Keep the extracted files together; the GUI needs its companion
programs and libraries.

Before running a download, compare its SHA-256 hash with the matching entry in
the release's `SHA256SUMS.txt`. Only use an installer when it is explicitly
listed for the same release version.

The MSI is not code-signed, so Windows may display an unknown-publisher
warning. Verify its checksum against the official release before opening it.

Linux and macOS packages are produced by the **Build Linux and macOS** GitHub
Actions workflow on manual runs and version-tag pushes. Download those packages
from the workflow run's artifacts; they are not automatically attached to the
GitHub release.

## Build from source

### Requirements

- CMake 3.14 or newer, a C++14-capable compiler, and Qt 5 development
  components (Core, Gui, Network, Widgets, and SVG).
- A compatible Bytecoin core checkout built alongside this repository at
  `../bytecoin`. The build expects its headers and `bytecoin-crypto` library
  in the standard locations referenced by `CMakeLists.txt`.
- Internet access during initial CMake configuration to fetch the pinned
  ZXing-C++ 1.4.0 dependency. QR processing is performed locally; wallet data
  and QR contents are not sent to a QR service.

### CMake

From the repository root, configure and build:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/qt
cmake --build build --config Release --parallel
```

The GUI executable is written to the repository's `bin` directory. On Windows,
use a Qt build and compiler architecture that match.

### Qt Creator / qmake

Open `src/bytecoin-gui.pro` in Qt Creator and select a kit compatible with the
Bytecoin core build. The qmake project requires a prebuilt static ZXing-C++
1.4.0 installation with reader and writer support. Set `ZXING_ROOT` to that
installation's prefix before building; its headers and library must be
available for the selected kit.

## Safety

Always verify the recipient address and amount on the final confirmation
screen. Keep wallet files, passwords, recovery words, and private keys secret,
and maintain a secure backup. Never share recovery information with anyone.

## License

This project is distributed under the GNU Lesser General Public License,
version 3 or (at your option) any later version. See [`LICENSE`](LICENSE) for
the complete license text. ZXing-C++ is a separate dependency distributed
under Apache License 2.0.

[releases]: https://github.com/bytecoinworld/bytecoin-gui/releases
