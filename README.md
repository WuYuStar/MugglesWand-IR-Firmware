# MugglesWand-IR 魔杖固件

ESP32-S3 魔杖固件源码，属于 [MugglesWand-IR 项目](https://github.com/WuYuStar/MugglesWand-IR)，基于 [zoushui 的 MugglesWand-src](https://github.com/zou-shui/MugglesWand-src) 扩展。魔杖识别挥杖动作，提供照明、空中光绘、蓝牙鼠标和音量控制；网页还能设置手势与灯效、配置 Home Assistant，并通过 ESP-NOW 连接红外配件。

## 使用魔杖

开机后连接 Wi-Fi `Muggles' Wand`，在浏览器打开 `http://192.168.4.1`。「发送指令」可直接体验功能；「配置 → 手势与灯效」可修改动作绑定。使用红外配件时，在「配置 → 红外配件」搜索并连接 ESP32-C3，学习信号后即可重放或绑定到手势。C3 固件和接线说明在[主项目](https://github.com/WuYuStar/MugglesWand-IR/tree/main/2.Firmware/IRLearner-C3)。

「配置 → 灯珠数量」可选择 35～45 颗，默认 39 颗。保存后立即作用于照明、灯效、开关机动画和空中光绘，断电后保留。

自定义手势可用[电脑端训练工具](https://github.com/WuYuStar/MugglesWand-IR/tree/main/3.Software/4.GestureTrainer)录制、训练和上传 `.wandmodel`；日常更换模型不需要重新编译本固件。

## 源码与构建

- `src/HAL/`：IMU、灯带、电量、电源和按键。
- `src/Model/`：手势推理与模型管理。
- `src/Service/`：网页、网络、蓝牙、升级和系统服务。
- `src/APP/`：照明、光绘、蓝牙 HID、ESP-NOW 和手势功能映射。

项目使用 PlatformIO 和 Arduino 框架。在 WSL 中安装 PlatformIO 后，于仓库根目录运行：

```sh
pio run -e esp32-s3-fh4r2
```

输出应用镜像位于 `.pio/build/esp32-s3-fh4r2/firmware.bin`。魔杖网页「设备 → 系统升级」可上传匹配的应用镜像。板型、内存和依赖版本以 [`platformio.ini`](platformio.ini) 为准；新设备首次完整烧录需要对应的引导程序和分区表。

原作者版权和 MIT 许可保留在 [LICENSE](LICENSE)。
