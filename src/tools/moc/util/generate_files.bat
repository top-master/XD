@echo off

copy /y licenseheader.txt ppkeywords.cpp
copy /y licenseheader.txt keywords.cpp

call generate_keywords for-preprocessor >> ppkeywords.cpp
call generate_keywords >> keywords.cpp

echo Done.

pause
