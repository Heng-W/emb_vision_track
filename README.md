# emb_vision_track

## 基于ARM Cortex A9 (S5P4418) 的嵌入式视觉移动追踪系统，含安卓APP

### 系统功能
（1）支持多用户连接的C/S模型TCP服务器
（2）摄像头画面实时传输到所有客户端
（3）任意选定目标的视觉跟踪（单尺度和多尺度）
（4）摄像头云台的手自动控制
（5）机器人小车的手自动控制
（6）APP数据监测及系统管理

### 简要说明
安卓APP源代码在AppProject目录下，可在Android Studio或AIDE环境下编译
嵌入式系统开发使用C/C++，源代码在assets目录下，包含内核层（硬件驱动编译成内核模块），应用层（网络通讯、视觉处理、设备控制）


### 系统源码编译
（1）交叉编译平台
下载目标平台为arm v7-a的交叉编译工具链，配置环境变量，输入arm-linux-gcc -v验证。
进入assets目录：cd assets/
执行Makefile：make
生成的可执行文件为assets/build/bin/out

下载linux源码树，进行编译
打开工程assets/drivers目录下的Makefile，修改KERNEL_DIR为内核源码目录
编译内核模块，在assets目录下执行：make modules
生成的内核模块ko文件在assets/build/modules目录下

（2）嵌入式开发板上直接编译
执行的Makefile加上参数：make CROSS_COMPILE=
执行程序：make exec
