#Copyright (c) 2019-2020 <>< Charles Lohr - Under the MIT/x11 or NewBSD License you choose.
# NO WARRANTY! NO GUARANTEE OF SUPPORT! USE AT YOUR OWN RISK
export TZ=UTC

all : makecapk.apk

.PHONY : push run

APPNAME?=Streak
LABEL?=$(APPNAME)
APKFILE ?= $(APPNAME).apk
PACKAGENAME?=org.rayg.streaktracker
RAWDRAWANDROID?=../rawdrawandroid
RAWDRAWANDROIDSRCS=$(RAWDRAWANDROID)/android_native_app_glue.c
SRC?=../main.c
BUILD_DIR?=..

# Pin, 33
ANDROIDVERSION=33
ANDROIDTARGET?=$(ANDROIDVERSION)

# for reproducible, ensures the binary hashes match regardless of where the project is stored.
CFLAGS?=-ffunction-sections -Os -fdata-sections -Wall -fvisibility=hidden -ffile-prefix-map=$(BUILD_DIR)=. -ffile-prefix-map=$(NDK)=/ndk
LDFLAGS?=-Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/output.map -s
ANDROID_FULLSCREEN?=y
ADB?=adb
UNAME := $(shell uname)

# For really tight compiles....
CFLAGS += -fvisibility=hidden
LDFLAGS += -s

# For C++
# LDFLAGS += -static-libstdc++
# $(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/aarch64-linux-android/libc.a

ANDROID_FULLSCREEN?=y
ADB?=adb
UNAME := $(shell uname)


ANDROIDSRCS:= $(SRC) $(RAWDRAWANDROIDSRCS)

#if you have a custom Android Home location you can add it to this list.
#This makefile will select the first present folder.


ifeq ($(UNAME), Linux)
OS_NAME = linux-x86_64
endif
ifeq ($(UNAME), Darwin)
OS_NAME = darwin-x86_64
endif
ifeq ($(OS), Windows_NT)
OS_NAME = windows-x86_64
endif

# Search list for where to try to find the SDK
SDK_LOCATIONS += $(ANDROID_HOME) $(ANDROID_SDK_ROOT) ~/Android/Sdk $(HOME)/Library/Android/sdk

#Just a little Makefile witchcraft to find the first SDK_LOCATION that exists
#Then find an ndk folder and build tools folder in there.
ANDROIDSDK?=$(firstword $(foreach dir, $(SDK_LOCATIONS), $(basename $(dir) ) ) )
# get all available ndk candidates
NDK_CANDIDATES := $(ANDROID_NDK) \
	           $(ANDROID_NDK_HOME) \
	           $(wildcard $(ANDROIDSDK)/ndk/27.*) \
	           $(wildcard $(ANDROIDSDK)/ndk-bundle) \
	           $(wildcard $(ANDROIDSDK)/ndk-bundle) \
	           $(wildcard $(ANDROIDSDK)/ndk/*)
NDK := $(firstword $(NDK_CANDIDATES))
BUILD_TOOLS?=$(lastword $(wildcard $(ANDROIDSDK)/build-tools/*) )

# fall back to default Android SDL installation location if valid NDK was not found
ifeq ($(NDK),)
ANDROIDSDK := ~/Android/Sdk
endif

# Verify if directories are detected
ifeq ($(ANDROIDSDK),)
$(error ANDROIDSDK directory not found)
endif
ifeq ($(NDK),)
$(error NDK directory not found)
endif
ifeq ($(BUILD_TOOLS),)
$(error BUILD_TOOLS directory not found)
endif

# Pin NDK, 27
NDK_VERSION_FILE := $(NDK)/source.properties
ifneq ("$(wildcard $(NDK_VERSION_FILE))","")
	NDK_VERSION_MAJOR := $(shell grep "Pkg.Revision" $(NDK_VERSION_FILE) | cut -d = -f 2 | cut -d . -f 1 | tr -d ' ')
	ifneq ($(NDK_VERSION_MAJOR), 27)
	    $(error "Incorrect NDK Version. Please use NDK r27 (Found version $(NDK_VERSION_MAJOR))")
	endif
endif

ifneq ("$(wildcard ./android-$(ANDROIDVERSION).jar)","")
ANDROID_JAR:=android-$(ANDROIDVERSION).jar
else
ANDROID_JAR:=$(ANDROIDSDK)/platforms/android-$(ANDROIDVERSION)/android.jar
endif

testsdk :
	@echo "SDK:\t\t" $(ANDROIDSDK)
	@echo "NDK:\t\t" $(NDK)
	@echo "Build Tools:\t" $(BUILD_TOOLS)

CFLAGS+=-Os -DANDROID -DAPPNAME=\"$(APPNAME)\"
ifeq (ANDROID_FULLSCREEN,y)
CFLAGS +=-DANDROID_FULLSCREEN
endif
CFLAGS+= -I$(RAWDRAWANDROID)/rawdraw -I$(NDK)/sysroot/usr/include -I$(NDK)/sysroot/usr/include/android -I$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/include -I$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/include/android -fPIC -I$(RAWDRAWANDROID) -DANDROIDVERSION=$(ANDROIDVERSION)
LDFLAGS += -lm -lGLESv3 -lEGL -landroid -llog -lOpenSLES
LDFLAGS += -shared -uANativeActivity_onCreate

CC_ARM64:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/aarch64-linux-android$(ANDROIDVERSION)-clang
CC_ARM32:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/armv7a-linux-androideabi$(ANDROIDVERSION)-clang
CC_x86:=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/i686-linux-android$(ANDROIDVERSION)-clang
CC_x86_64=$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/bin/x86_64-linux-android$(ANDROIDVERSION)-clang
AAPT:=$(BUILD_TOOLS)/aapt

# Which binaries to build? Just comment/uncomment these lines:
DEFAULT_TARGETS += $(BUILD_DIR)/makecapk/lib/arm64-v8a/lib$(APPNAME).so
DEFAULT_TARGETS += $(BUILD_DIR)/makecapk/lib/armeabi-v7a/lib$(APPNAME).so
DEFAULT_TARGETS += $(BUILD_DIR)/makecapk/lib/x86/lib$(APPNAME).so
DEFAULT_TARGETS += $(BUILD_DIR)/makecapk/lib/x86_64/lib$(APPNAME).so

TARGETS ?= $(DEFAULT_TARGETS)

CFLAGS_ARM64:=-m64
CFLAGS_ARM32:=-mfloat-abi=softfp -m32
CFLAGS_x86:=-march=i686 -mssse3 -mfpmath=sse -m32
CFLAGS_x86_64:=-march=x86-64 -msse4.2 -mpopcnt -m64
STOREPASS?=password
DNAME:="CN=example.com, OU=ID, O=Example, L=Doe, S=John, C=GB"
KEYSTOREFILE:=debug.keystore
ALIASNAME?=standkey

keystore : $(KEYSTOREFILE)

$(KEYSTOREFILE) :
	keytool -genkey -v -keystore $(KEYSTOREFILE) -alias $(ALIASNAME) -keyalg RSA -keysize 2048 -validity 10000 -storepass $(STOREPASS) -keypass $(STOREPASS) -dname $(DNAME)

folders:
	mkdir -p $(BUILD_DIR)/makecapk/lib/arm64-v8a
	mkdir -p $(BUILD_DIR)/makecapk/lib/armeabi-v7a
	mkdir -p $(BUILD_DIR)/makecapk/lib/x86
	mkdir -p $(BUILD_DIR)/makecapk/lib/x86_64

$(BUILD_DIR)/makecapk/lib/arm64-v8a/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p $(BUILD_DIR)/makecapk/lib/arm64-v8a
	$(CC_ARM64) $(CFLAGS) $(CFLAGS_ARM64) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/aarch64-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

$(BUILD_DIR)/makecapk/lib/armeabi-v7a/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p $(BUILD_DIR)/makecapk/lib/armeabi-v7a
	$(CC_ARM32) $(CFLAGS) $(CFLAGS_ARM32) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/arm-linux-androideabi/$(ANDROIDVERSION) $(LDFLAGS)

$(BUILD_DIR)/makecapk/lib/x86/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p $(BUILD_DIR)/makecapk/lib/x86
	$(CC_x86) $(CFLAGS) $(CFLAGS_x86) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/i686-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

$(BUILD_DIR)/makecapk/lib/x86_64/lib$(APPNAME).so : $(ANDROIDSRCS)
	mkdir -p $(BUILD_DIR)/makecapk/lib/x86_64
	$(CC_x86) $(CFLAGS) $(CFLAGS_x86_64) -o $@ $^ -L$(NDK)/toolchains/llvm/prebuilt/$(OS_NAME)/sysroot/usr/lib/x86_64-linux-android/$(ANDROIDVERSION) $(LDFLAGS)

#We're really cutting corners.  You should probably use resource files.. Replace android:label="@string/app_name" and add a resource file.
#Then do this -S Sources/res on the aapt line.
#For icon support, add -S makecapk/res to the aapt line.  also,  android:icon="@mipmap/icon" to your application line in the manifest.
#If you want to strip out about 800 bytes of data you can remove the icon and strings.

#Notes for the past:  These lines used to work, but don't seem to anymore.  Switched to newer jarsigner.
#(zipalign -c -v 8 makecapk.apk)||true #This seems to not work well.
#jarsigner -verify -verbose -certs makecapk.apk

# help/doc:
# All .apk's that are produced:
#
# temp.apk
# 	Contains android jar file, res, and assets
# makecapk.apk
#	temp.apk + resources.arsc(compressed res) + Manifest, all compressed
# $(APKFILE)
# 	zipalign'ed and signed makecapk.apk

makecapk.apk : $(TARGETS) $(EXTRA_ASSETS_TRIGGER) $(BUILD_DIR)/AndroidManifest.xml $(KEYSTOREFILE)
	rm -rf $(BUILD_DIR)/temp.apk
	$(AAPT) package -f -F $(BUILD_DIR)/temp.apk -I $(ANDROID_JAR) -M $(BUILD_DIR)/AndroidManifest.xml -S res --auto-add-overlay -v --target-sdk-version $(ANDROIDTARGET)
	unzip -o $(BUILD_DIR)/temp.apk -d $(BUILD_DIR)/makecapk
	rm -rf $(BUILD_DIR)/makecapk.apk

	# fix timestamp to latest commit
	find $(BUILD_DIR)/makecapk -exec touch -d @$(shell git log -1 --format=%ct) {} +

	cd $(BUILD_DIR)/makecapk && find . -type f | LC_ALL=C sort | zip -D4Xo@ ../makecapk.apk
	# We use -4 here for the compression ratio, as it's a good balance of speed and size. -9 will make a slightly smaller executable but takes longer to build
	cd $(BUILD_DIR)/makecapk && zip -D4r ../makecapk.apk . && zip -D0r ../makecapk.apk ./resources.arsc ./AndroidManifest.xml
	# jarsigner is only necessary when targetting Android < 7.0
	#jarsigner -sigalg SHA1withRSA -digestalg SHA1 -verbose -keystore $(KEYSTOREFILE) -storepass $(STOREPASS) $(BUILD_DIR)/makecapk.apk $(ALIASNAME)
	rm -rf $(BUILD_DIR)/$(APKFILE)
	$(BUILD_TOOLS)/zipalign -v 4 $(BUILD_DIR)/makecapk.apk $(BUILD_DIR)/$(APKFILE)

	# F-Droid runs with KEYSTOREFILE empty, also removes the sed line from yml
	if [ -n "$(KEYSTOREFILE)" ] && [ -f "$(KEYSTOREFILE)" ]; then \
		$(BUILD_TOOLS)/apksigner sign --key-pass pass:$(STOREPASS) --ks-pass pass:$(STOREPASS) --ks $(KEYSTOREFILE) $(BUILD_DIR)/$(APKFILE); \
	fi

	rm -rf $(BUILD_DIR)/temp.apk
	rm -rf $(BUILD_DIR)/makecapk.apk
	@ls -l $(BUILD_DIR)/$(APKFILE)

manifest: $(BUILD_DIR)/AndroidManifest.xml

$(BUILD_DIR)/AndroidManifest.xml :
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/AndroidManifest.xml
	PACKAGENAME=$(PACKAGENAME) \
		ANDROIDVERSION=22 \
		ANDROIDTARGET=$(ANDROIDTARGET) \
		APPNAME=$(APPNAME) \
		LABEL=$(LABEL) envsubst '$$ANDROIDTARGET $$ANDROIDVERSION $$APPNAME $$PACKAGENAME $$LABEL' \
		< AndroidManifest.xml.template > $(BUILD_DIR)/AndroidManifest.xml


uninstall :
	($(ADB) uninstall $(PACKAGENAME))||true

push : makecapk.apk
	@echo "Installing" $(PACKAGENAME)
	$(ADB) install -r $(BUILD_DIR)/$(APKFILE)

run : push
	$(eval ACTIVITYNAME:=$(shell $(AAPT) dump badging $(BUILD_DIR)/$(APKFILE) | grep "launchable-activity" | cut -f 2 -d"'"))
	$(ADB) shell am start -n $(PACKAGENAME)/$(ACTIVITYNAME)

clean :
	rm -rf $(BUILD_DIR)/AndroidManifest.xml $(BUILD_DIR)/temp.apk $(BUILD_DIR)/makecapk.apk $(BUILD_DIR)/makecapk $(BUILD_DIR)/$(APKFILE) $(BUILD_DIR)/$(APKFILE).idsig $(BUILD_DIR)/output.map
