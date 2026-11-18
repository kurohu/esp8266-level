# ESP8266 电子水平仪 · 开源资料包

原创设计的 ESP8266 电子水平仪全套资料：外壳（参数化建模 + STL/STEP）、固件源码与可直刷 BIN、接线与组装、使用与校准调试。主控 NodeMCU 1.0（ESP-12E，4 MB Flash），显示 GMT020-02-8P／ST7789 2.0 英寸 SPI 屏，姿态传感器 GY-521／MPU6050。

3D 打印模型页：MakerWorld 模型链接发布后补充（TODO）

## 仓库结构

```
firmware/   Arduino 源码（ST7789_Test.ino）与编译好的固件 BIN
docs/       硬件接线与组装、使用与调试（含六面标定）
casing/     外壳：CadQuery 参数化源码、README、build.py、output/ 内 STL+STEP
images/     接线图（渲染版）
```

## 导航

| 需要什么 | 去哪里看 |
|-|-|
| 零件、接线与供电 | [docs/硬件接线与组装.md](docs/硬件接线与组装.md) |
| 操作、校准、原理与验收 | [docs/使用与调试.md](docs/使用与调试.md) |
| Arduino 源码 | [firmware/ST7789_Test/ST7789_Test.ino](firmware/ST7789_Test/ST7789_Test.ino) |
| 可直接刷写的固件 | [firmware/ST7789_Test.ino.bin](firmware/ST7789_Test.ino.bin) |
| 外壳打印文件 | [casing/output/](casing/output/)（STL 打印用，STEP 可改尺寸） |
| 外壳尺寸修改入口 | [casing/cad/model.py](casing/cad/model.py)（参数化源码） |
| 完整性校验 | [SHA256SUMS.txt](SHA256SUMS.txt) |

## 从零开始

1. 按[接线表](docs/硬件接线与组装.md)连接 NodeMCU、屏幕和 MPU6050。先用开发板的 USB 单独供电，检查屏幕与传感器，再接按键、蜂鸣器和电池线路。
2. 从下面两种烧录方式中任选一种。烧录、启动和复位期间松开外接 UP／DOWN。
3. 用 [casing/output/](casing/output/) 的 STL 打印外壳（打印配置见 MakerWorld 模型页的 3MF），装配后在已知水平的基准面上按[使用与调试](docs/使用与调试.md)完成校准，再验收 HOLD、PIN、提示音和电量。

### 直接刷入 BIN

安装 Python 3 和 esptool 5。用 USB 数据线连接 NodeMCU，关闭串口监视器，从仓库根目录运行。以 COM3 为例，按实际端口替换。

```powershell
python -m pip install esptool==5.2.0
python -m esptool --chip esp8266 --port COM3 --baud 460800 write-flash 0x00000 .\firmware\ST7789_Test.ino.bin
```

固件为 4 MB Flash 配置，DIO，40 MHz。看到写入数据哈希校验通过后即可启动。如果高速烧录失败，可试 115200。自动下载失败时，用开发板自带 FLASH／BOOT 键按住 GPIO0，再按一下 RST，进入串口下载模式。

### 从源码编译

Arduino IDE 的附加开发板管理器网址填写 `https://arduino.esp8266.com/stable/package_esp8266com_index.json`。安装 **esp8266 by ESP8266 Community 3.1.2**，打开 `firmware/ST7789_Test/ST7789_Test.ino`，选择 **NodeMCU 1.0 (ESP-12E Module)** 和实际串口。源码只使用 ESP8266 Arduino Core 自带的 Arduino、SPI、Wire、EEPROM。

已有 Arduino CLI 时，也可以在仓库根目录运行：

```powershell
arduino-cli core update-index --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core install esp8266:esp8266@3.1.2 --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .\firmware\ST7789_Test
```

- [ESP8266 Arduino Core 安装说明](https://arduino-esp8266.readthedocs.io/en/3.1.2/installing.html)
- [乐鑫 ESP8266 启动与下载模式](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/boot-mode-selection.html)

## 许可证

本仓库全部内容（源码、文档、模型）采用 [CC BY-NC-SA 4.0](LICENSE)（署名-非商业性使用-相同方式共享）：可自由转发和改作，衍生品必须以相同许可共享，且不得用于商业目的。与 MakerWorld 模型页的许可一致。
