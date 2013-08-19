@echo off
echo Compiling strings
"Tools\xmltodb.exe" -c Strings\Config.xml -x Strings\String -f *.xml -d bin\L10n.db



