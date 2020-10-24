@echo off
call _paths.bat

PATH=%QtDir%;%MinGW%;%GitDir%;%PATH%
SET SEVENZIP=C:\Program Files\7-Zip

IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles(x86)%\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles%\7-Zip

lrelease FMBankEdit.pro

qmake FMBankEdit.pro CONFIG+=release CONFIG-=debug
IF ERRORLEVEL 1 goto error

mingw32-make
IF ERRORLEVEL 1 goto error

md opn2-bank-editor
cd bin-release
windeployqt opn2_bank_editor.exe
copy "..\src\translations\*.qm" translations
IF ERRORLEVEL 1 goto error
cd ..

"%SEVENZIP%\7z" a -tzip "opn2-bank-editor\opn2-bank-editor-dev-%WINXX_ARCH%.zip" .\license.txt .\changelog.txt .\Specifications\WOPN-and-OPNI-Specification.txt .\Specifications\YM2612.pdf .\bin-release\* Bank_Examples

goto quit
:error
echo ==============BUILD ERRORED!===============
:quit

