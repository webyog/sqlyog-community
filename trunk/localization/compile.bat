@echo off
echo Compiling strings
"Tools\appl10n\bin\release\xmltodb.exe" -c Strings\Config.xml -x Strings\String -f *.xml -d bin\L10n.db



