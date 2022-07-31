# emb_vision_track

## 基于ARM-Linux的嵌入式视觉移动追踪系统
## 搭载舵机云台的机器人小车通过视觉追踪设定目标，并通过 Android APP 回传画面和无线控制

### 系统描述
1. 在ARM Cortex A9 (S5P4418) 开发板上利用多线程分别实现网络 IO、图像采集、图像处理、硬件控制功能
2. 在 Linux 环境下搭建 Reactor 模型 TCP 服务器，支持多用户的连接访问
3. 利用 V4L2 接口采集相机图像，使用 OpenCV 实现的 KCF 算法进行任意设定目标的视觉跟踪（单尺度和多尺度追踪两种模式）
4. 编写 Linux 内核模块实现舵机和电机驱动，分别控制摄像头云台和机器人小车，并在用户层实现手动控制和自动控制逻辑
5. 开发 Android APP 进行 TCP 通信，以控制系统运行、接收相机画面、监测运行数据

### 说明
嵌入式系统中，内核层代码将硬件驱动编译成内核模块，使用了两个PWM控制舵机云台，两个PWM及两个GPIO控制机器人小车，可以参考移植到其它平台；应用层利用C++11实现网络通讯、视觉处理、硬件控制功能，代码可运行在任意的32位ARM-Linux平台（包含树莓派3B+），未安装内核模块时会自动忽略底层的硬件控制指令，可以预览视觉跟踪功能。

Android APP在Android Studio下开发。

### 编译安装
安装支持32位ARM-Linux的交叉编译器

进入assets目录：`cd assets/`

编译代码：`make -j`

编译完成后在bin目录下生成可执行文件，将bin、lib、conf三个目录拷贝到开发板上，运行bin目录下的可执行文件即可

### 编译内核模块

下载Linux内核源码（本项目使用的是支持S5P4418的linux-3.4.y-nanopi2-lollipop-mr1，其它平台可根据引脚功能分布加以调整），编译内核

打开工程assets/drivers目录下的Makefile，修改KERNEL_DIR为内核源码目录

编译内核模块，在assets目录下执行：`make modules`

生成的内核模块ko文件在assets/build/modules目录下，通过insmod命令加载

