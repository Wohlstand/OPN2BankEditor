# OPN2BankEditor
![OPN2 Editor Logo](src/resources/opn2_48.png)

A small cross-platform editor for the OPN family of FM synthesis soundchips (which were widely used in Sega Genesis (aka Mega Drive) game console), Fujitsu FM Towns home computer and NEC PC-88 and PC-98 home computer series).

# CI Build status
Linux | Windows (32-bit) | macOS
------------ | ------------- | -------------
[![Build Status](https://travis-ci.org/Wohlstand/OPL3BankEditor.svg?branch=master)](https://travis-ci.org/Wohlstand/OPN2BankEditor) | [![Build status](https://ci.appveyor.com/api/projects/status/mtl3v7gemh10p30g?svg=true)](https://ci.appveyor.com/project/Wohlstand/opn2bankeditor) | [![Build Status](https://travis-ci.org/Wohlstand/OPN2BankEditor.svg?branch=master)](https://travis-ci.org/Wohlstand/OPN2BankEditor)

**BETA. Please report me any bugs and imperfections you have found**

## Download
* **Stable builds:** https://github.com/Wohlstand/OPN2BankEditor/releases
* **Fresh dev builds:**
  * [Download for Windows x86_64](https://wohlsoft.ru/docs/_laboratory/_Builds/win32/opn2-bank-editor/opn2-bank-editor-dev-win64.zip) (built by [AppVeyor](https://ci.appveyor.com/project/Wohlstand/opn2bankeditor))
  * [Download for Windows x86](https://wohlsoft.ru/docs/_laboratory/_Builds/win32/opn2-bank-editor/opn2-bank-editor-dev-win32.zip) (built by [AppVeyor](https://ci.appveyor.com/project/Wohlstand/opn2bankeditor))
  * [Download for macOS x64 (DMG)](https://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opn2-bank-editor/opn2_bank_editor-macos.dmg) (built by [Travis-CI](https://travis-ci.org/Wohlstand/OPN2BankEditor))
  * [Download for macOS x64 (ZIP)](https://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opn2-bank-editor/opn2_bank_editor-macos.zip) (built by [Travis-CI](https://travis-ci.org/Wohlstand/OPN2BankEditor))
  * CIs for other operating systems are coming soon... (However, it's easy to build it by yourself :wink:)

# How to build
**Prerequisites**

This editor requires following dependences on Linux-based systems.
Debian and it's derivatives:
```
build-essentials
libasound2-dev
zlib1g-dev
cmake
qt5-default
qttools5-dev
libpulse-dev
libqwt-qt5-dev //OPTIONAL
libjack-dev //OPTIONAL
```
Arch and it's derivatives:
```
desktop-file-utils
hicolor-icon-theme
jack
libpulse
qwt
cmake
qt5-base
qt5-tools
```
Before you start the build, make sure you have also cloned submodules!

```
git submodule init
git submodule update
```
**Building with CMake**

Navigate to the project directory in a terminal and follow this build procedure.

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

This will result in a software installation located in /usr/local, with program shortcuts, icons and MIME type associations.

**Building with QMake**

The easiest way is to open FMBankEdit.pro in the QtCreator software and run a compilation. Alternatively, you can also build manually on the command line.

Navigate to the project directory in a terminal and follow this build procedure.

```
qmake CONFIG+=release CONFIG-=debug FMBankEdit.pro
make
```
As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.

## Languages

As of version 1.3.1, this tool supports following languages:

* English
* Français (French)
* Русский (Russian)
* Polski (Polish)

# Folders
* ***Bank_Examples*** - example bank files which you can edit and preview them
* ***Specifications*** - documentation of file formats used by this tool
* ***cmake*** - CMake-related scripts
* ***src*** - source code of this tool
* ***_Misc*** - Various stuff (test scripts, dummy banks, documents, etc.) which was been used in development of this tool
