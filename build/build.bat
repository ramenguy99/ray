@echo off

REM Hopefully find visual studio vcvarsall file to add the compiler to the path

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 11.0
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 13.0
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

SET VC_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
IF NOT DEFINED LIB (IF EXIST "%VC_PATH%" (call "%VC_PATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64))

IF NOT DEFINED LIB ( 
    echo Unable to find Visual Studio installation. Execute vcvarsall.bat amd64 in the shell before building
    EXIT /B 1
)

REM Actually build the program
SET IGNOREDWARNINGS= -wd4100 -wd4127 -wd4189 -wd4201 -wd4505 -wd4200 -wd4204

SET CompilerFlags= -O2 -nologo -MT -GR- -EHa- -Oi -WX -W4 %IGNOREDWARNINGS% -FC -Z7 -F 2097152 -D_CRT_SECURE_NO_WARNINGS
SET IncludePaths= 

SET LinkerFlags=  -incremental:no -opt:ref  -IGNORE:4099
SET LibPaths=

cl %CompilerFlags%  %IncludePaths% ../src/main.cpp /Feray /link %LibPaths% %LinkerFlags% 


