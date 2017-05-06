# OPN2BankEditor
A small cross-platform editor of the OPN2 FM chip (which is widely used in Sega Genesis (aka Mega Drive) game console)

**BETA. Please report me any bugs and imperfections you have found**

# How to build
You need a Qt 5 to build this project.

Run next commands from project directory:
```
qmake CONFIG+=release CONFIG-=debug FMBankEdit.pro
make
```

As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.

# Folders
* ***Bank_Examples*** - example bank files which you can edit and preview them
* ***src*** - source code of this tool
* ***_Misc*** - Various stuff (test scripts, dummy banks, documents, etc.) which was been used in development of this tool

