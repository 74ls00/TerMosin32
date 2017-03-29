@echo off
@set rel=TerMosin32

@make clean
@cls

@make

@del %rel%-rel.hex
@copy %rel%.hex %rel%-rel.hex

@make clean

rem @pause
@PING -n 1 -w 5000 192.168.253.253 > nul