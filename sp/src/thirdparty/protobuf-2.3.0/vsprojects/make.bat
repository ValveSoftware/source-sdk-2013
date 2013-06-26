@echo off
setlocal

@rem Move to the batch file directory so we can run it from anywhere
cd %~dp0

call "%VS100COMNTOOLS%vsvars32.bat"

for /d %%d in (debug release "debug|x64" "release|x64") do (
	echo Building %%d
	devenv protobuf_2010.sln /clean %%d
	devenv protobuf_2010.sln /build %%d /project libprotobuf
	devenv protobuf_2010.sln /build %%d /project libprotobuf-lite
	devenv protobuf_2010.sln /build %%d /project libprotoc
)

p4 edit ..\..\..\lib\win32\debug\VS2010\libproto*.lib
p4 edit ..\..\..\lib\win32\release\VS2010\libproto*.lib
p4 edit ..\..\..\lib\win64\debug\VS2010\libproto*.lib
p4 edit ..\..\..\lib\win64\release\VS2010\libproto*.lib

copy /y Debug\libproto*.lib ..\..\..\lib\win32\debug\VS2010
copy /y Release\libproto*.lib ..\..\..\lib\win32\Release\VS2010
copy /y x64\Debug\libproto*.lib ..\..\..\lib\win64\debug\VS2010
copy /y x64\Release\libproto*.lib ..\..\..\lib\win64\Release\VS2010

@echo Check in the changed libraries in src\lib if you are done.
