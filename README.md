dama
====

Project to provide a silencer equipment to solve DaMa dancing issue in China.
# Codey代码简介

codey的代码，包括 esp-idf以及 micropython 两份代码。


# 编译方式

1. 在windows环境下，搭建 [msys2开发环境](https://esp-idf.readthedocs.io/en/latest/get-started/windows-setup.html)，可以参考这个说明 [点击链接](https://github.com/YanMinge/esp_project)
2. 使用git下载本工程代码
3. msys2 进入 `Codey\micropython-esp32\mpy-cross` 目录， 输入命令 `make`
4. 在`Codey\micropython-esp32\ports\esp32` 目录中，创建一个 Makefile文件，内容如下:
```
#esp idf的目录
ESPIDF = /g/Codey/esp-idf
#esp32设备的端口号
PORT = COM3
FLASH_MODE = dio
#烧写的波特率
BAUD = 256000
#编译文件所存储目录
BUILD = build
include GNUMakefile
```
5. mysy2 进入 `Codey\micropython-esp32\ports\esp32` 目录，输入 `make deploy -j4`

# esp32的Backtrace调试方法

- 进入工程编译后的 build目录，通过 objdump 生成 静态分析文件。
xtensa-esp32-elf-objdump -S ota.elf > debug.txt // -S 而不是 -s

- vim debug.txt 输入 Backtrace 的各个地址【从后向前 即 从上层到底层
