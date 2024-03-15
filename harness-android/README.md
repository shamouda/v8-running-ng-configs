### How to build the harness program for Android on a linux machine

1. Download the latest linux package of the Android NDK tool from https://developer.android.com/ndk
1. Unpack the package and export its location to NDK_HOME
1. Run this command to generate the .so library:

    ```
    $NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
    --target=aarch64-linux-android33 -shared -fPIC \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -O2 -static-libstdc++ harness.cpp -o libharness.so
    ```

    The above assumes you are using Android API version 33. 
    Modify the command as needed to match your requirements.
