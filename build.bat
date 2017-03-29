@echo off
set rel=TerMosin32
set pref1=m8
set mfale8=%~d0%~p0Makefile-m8

make clean
cls

rem make -f %mfale8%
make

del %rel%-%pref1%.hex
copy %rel%.hex %rel%-%pref1%.hex

make clean

rem @pause
@PING -n 1 -w 5000 192.168.253.253 > nul