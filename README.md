Replacing IP addresses based on redirection with a hook using a c++ plugin for the library libblackrussia-client.so.
Author: DragosHack
Forked from: kuzia15
![image](https://github.com/user-attachments/assets/f13a026c-a411-4e90-a489-3ca39f5dde09)

Installing:
git clone https://github.com/soul-proger/blackrussia-new-changeip.git

Preparing:
Change your ip address and port in game/hooks.cpp

Building:
1. Create folder "jni"
2. Move all files from archive to folder "jni"
3. Open terminal and send command "{path/to/ndk21/}ndk-build -C {path/to/jni/}"

You can download the original black russia client on the black russia.online website. Then you need to delete libraries for 64-bit architecture in apk, and add your library for 32-bit, you also need to add it to smali (in the search, write LoadLibrary in the smali folder, there will be only one file, copy the function usage and repeat it with your plugin.
