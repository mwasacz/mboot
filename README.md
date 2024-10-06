# MBoot

An open source drop-in replacement for the legacy ESP8266 bootloader.

## Description

The ESP8266 NONOS SDK, as well as RTOS SDK v1.x/v2.x, rely on the legacy bootloader provided as a binary blob.
Unfortunately, it has a bug that prevents OTA updates when the 8MB or 16MB flash map is used. This repository provides
an alternative open source bootloader that fixes this issue and can be used as a drop-in replacement for `boot_v1.7.bin`
bootloader, without the need to change any user code.

The code is based on [rBoot](https://github.com/raburton/rboot) by Richard A Burton, with modifications to make it
compatible with the standard OTA API used in NONOS SDK and RTOS SDK v1.x/v2.x. All extra rBoot features have been
removed (since they wouldn't be compatible with the API anyway). Refer to the original repository for information about
rBoot. Also check out the following resources which were useful:

- [Decompiling the ESP8266 boot loader v1.3(b3)](
https://richard.burtons.org/2015/05/17/decompiling-the-esp8266-boot-loader-v1-3b3/)
- [ESP8266 16MB Flash Handling](
https://piers.rocks/esp8266/16mb/flash/eeprom/2016/10/14/esp8266-16mbyte-flash_handling.html)

The only change in behavior compared to `boot_v1.7.bin` is a different message printed during boot, and a different UART
baud rate (115200 rather than 74880). However, the baud rate is can be changed by editing [rboot.h](rboot.h). If for
some reason you require even greater compatibility with the original bootloader, check out
[Fixboot](https://github.com/mwasacz/fixboot).

**Note:** NONOS SDK and RTOS SDK v1.x/v2.x are deprecated. You should use RTOS SDK v3.x for new projects.

## Compilation

The compiled binary is available on the [Releases](https://github.com/mwasacz/mboot/releases) page. If you want to build
it yourself, you'll need the ESP8266 toolchain (version 4.8.5 is recommended). On Windows, a compatibility layer such as
MSYS2 is required. For installation instructions, refer to
[RTOS SDK docs](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#setup-toolchain).
Make sure `xtensa-lx106-elf/bin` is added to the `PATH` environment variable.

Additionally, you will need [Esptool2](https://github.com/raburton/esptool2). Place the Esptool2 and MBoot directories
in the same parent directory. You can use `make` to compile Esptool2 (on Windows use the MINGW64 shell or compile it
with Visual Studio).

Once you have the toolchain and Esptool2 installed, run `make` to build MBoot (on Windows use the MSYS2 shell).

## Usage

When flashing the ESP8266, simply use `mboot.bin` instead of `boot_v1.7.bin`. The regular NONOS SDK or RTOS SDK
v1.x/v2.x OTA update process should work without issues.

## License

This project is released under the [MIT license](LICENSE). It is based on [rBoot](https://github.com/raburton/rboot) by
Richard A Burton, which is released under the [MIT license](https://github.com/raburton/rboot/blob/master/license.txt).
