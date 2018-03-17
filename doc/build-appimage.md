## Scash QT AppImage

### What is an AppImage?

Quoting [their wiki](https://github.com/AppImage/AppImageKit/wiki#-what-is-an-appimage):

> An AppImage is a downloadable file for Linux that contains an application and everything the application needs to run (e.g., libraries, icons, fonts, translations, etc.) that cannot be reasonably expected to be part of each target system.

### Packaging the Scash QT Wallet

From downloading the github source, move the `scash-main` folder to the $HOME directory. If downloaded from [releases](https://github.com/scashml/scashcpp/releases), rename your uncompressed source code folder `scashcpp-XXXXX` to `scash-main`  and move to $HOME.


Compile the Scash QT wallet by following the [soure code build instructions](https://github.com/scashml/scashcpp/blob/main/doc/readme-qt.rst), then follow the next steps to obtain the AppImage. This method has been tested in Ubuntu 17.10 and should work for Debian-based systems.

Create a folder `dist` to contain the packaging results:

```
mkdir -p scashcpp-main/dist/usr/bin
```
Copy the `scash-qt` executable to the `usr/bin` folder:

```
cd scashcpp-main/dist
cp ../src/qt/scashcpp-main usr/bin/
```

Create a `.desktop` file for the package:

```
nano scash-qt.desktop
```

Enter the following configuration paramters:

```
[Desktop Entry]
Version=1.0
Type=Application
Name=Scash Wallet
Exec=AppRun %F
Icon=Scash
Categories=Network;
```

Download an icon for the binary package and rename:

```
wget https://i.imgur.com/FRRCFXk.png
mv mv FRRCFXk.png Scash.png
```

Download the `linuxdeployqt` utility in order to create the AppImage:

```
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
```

Make the `linuxdeployqt` utility executable:

```
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
```

Install `qt4-qmake` and make default:

```
sudo apt install qt4-default qt4-qmake
```

Create the Scash-QT AppImage:

```
./linuxdeployqt-continuous-x86_64.AppImage usr/bin/scash-qt -appimage -bundle-non-qt-libs -verbose=2
```

Make the newly created `Scash_Wallet-x86_64.AppImage` executable:

```
chmod a+x Scash_Wallet-x86_64.AppImage
```

Launching Scash-QT AppImage

```
$ ./Scash_Wallet-x86_64.AppImage
```
