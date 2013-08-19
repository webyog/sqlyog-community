@echo off
SET /a comment=1
:: SET /a comment=0
IF %comment%==1 GOTO ENDCOMMENT
IF NOT EXIST ..\SQLyogCommunity GOTO ENTERPRISE
echo Cleaning up..
IF EXIST Strings\String.xml del Strings\String*.xml
IF EXIST string.txt del string.txt
IF EXIST makexmldb del makexmldb
echo Extracting strings from Community source
"tools\pcregrep.exe" -rhoM  --file=tools\pattern.txt --include=.cpp --exclude_dir=.svn ..\SQLyogCommunity > string.txt
"tools\pcregrep.exe" -rhoM  --file=tools\pattern.txt --include=.h --exclude_dir=.svn ..\SQLyogCommunity >> string.txt
:ENTERPRISE
IF NOT EXIST ..\SQLyogEnterprise GOTO EXECUTE
echo Extracting strings from Enterprise source
"tools\pcregrep.exe" -rhoM  --file=tools\pattern.txt --include=.cpp --exclude_dir=.svn ..\SQLyogEnterprise >> string.txt
"tools\pcregrep.exe" -rhoM  --file=tools\pattern.txt --include=.h --exclude_dir=.svn ..\SQLyogEnterprise >> string.txt
:EXECUTE
IF NOT EXIST string.txt GOTO FINISH
echo Generating Strings\String.xml
"tools\makexml.exe" -i string.txt -i ..\SQLyogEnterprise\include\SQLyogEnt.rc -i ..\SQLyogCommunity\include\SQLyog.rc -o Strings\String.xml -t makexmldb -c 1000
:FINISH
IF EXIST makexmldb del makexmldb
IF EXIST string.txt del string.txt
GOTO END
:ENDCOMMENT
echo uncomment line 3 (::SET /a comment=0) to execute the batch script
:END