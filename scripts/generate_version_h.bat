echo #ifndef VERSION_H  > ..\src\version.h
set "tempFile=%tmp%\bat~%RANDOM%.bat"
git -C .. describe --tags --always --dirty >> %tempFile%
set /P GitVersion=<%tempFile%
echo #define VERSION_H >> ..\src\version.h
echo #define JSRDBG_VERSION "%GitVersion%" >> ..\src\version.h
echo #endif >> ..\src\version.h
