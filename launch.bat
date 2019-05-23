echo off
title Time of Doubles Launcher
echo "Time of Doubles Launcher"


cd ..\alicenode\
start alice.exe &
cd ..\node-tod\
echo "Launching Alice kinect server on process ID $!" &

start audio.maxpat &
echo "Launching Max/MSP & Sonification on process ID $!" &


rem timeout 10

rem supervisor tod.js
echo Exit Code is %errorlevel%

echo "Launching Time of Doubles simulation on process ID $!" &
supervisor tod.js 

pause

rem needs two to close console as well
taskkill /T /im Max.exe
taskkill /T /im Max.exe

taskkill /im alice.exe

taskkill /T /im cmd.exe