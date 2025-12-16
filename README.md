# Streak

A super minimal daily streak tracking app built in C, for Android. 

189 kb apk, built to track workout streaks but could be used for anything

## How to Compile

1. Ensure you have the Android NDK and SDK installed. Then change line 22 `ANDROIDVERSION?=` in build/Makefile to your SDK version. Default is 34.

2. Clone this repo

`git clone --recurse-submodules https://github.com/RaynardGerraldo/Streak`

3. Navigate to build folder

`cd Streak; cd build/`

4. Connect your android phone to your machine with adb activated

`adb start-server`

`adb devices`

4. Build and Run:

`make run`

## Credits

https://github.com/cnlohr/rawdrawandroid -- awesome project

## License

MIT
