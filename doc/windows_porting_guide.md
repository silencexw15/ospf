# Windows 7 + VMware + eNSP 适配指南

> 目标：让当前基于 Linux 原生套接字/路由 ioctl 的 OSPF 实现，能够在 **Win7 虚拟机内编译运行**，并与 eNSP 拓扑互通验证。

## 1. 先说结论：需要改动的核心点

当前代码是 Linux 定制实现，主要依赖：

- `SO_BINDTODEVICE` 绑定网卡。
- `AF_INET + SOCK_RAW + IPPROTO_OSPF` 原始套接字收发。
- `ioctl(SIOCADDRT/SIOCDELRT)` + `linux/route.h` 写内核路由。
- `fork/setsid` 守护进程逻辑。
- `pthread` 线程接口。

对应文件分别集中在：

- `src/common.h`
- `src/packet_manage.cpp`
- `src/routing.h`
- `src/routing.cpp`
- `src/main.cpp`
- `src/setting.cpp`

因此**不能直接在 Win7 上无修改编译运行**。

---

## 2. 推荐迁移策略（最稳妥）

建议分 2 个阶段：

1. **先做“可编译”**：通过平台抽象层把 Linux API 隔离，Windows 先打通编译链路。
2. **再做“可收发/可改路由”**：在 Windows 端引入 Npcap + IP Helper API，替换 Linux 原始网络与路由接口。

这样能避免一次性大改导致无法定位问题。

---

## 3. 代码改造清单（按模块）

## 3.1 平台头文件与初始化

### 需要调整

在 `src/common.h` 里把 Linux 专属头文件改为条件编译：

- Linux 分支保留 `unistd.h`, `sys/ioctl.h`, `net/if.h`, `netinet/*`。
- Windows 分支使用：
  - `winsock2.h`
  - `ws2tcpip.h`
  - `iphlpapi.h`
  - `windows.h`

并增加一个平台初始化函数（例如 `platformNetInit()`）：

- Linux：空实现。
- Windows：调用 `WSAStartup(MAKEWORD(2,2), ...)`，退出时 `WSACleanup()`。

## 3.2 线程模型

当前是 `pthread_*`。Windows 可选两种方式：

- **推荐**：统一改成 `std::thread + std::mutex`（跨平台最省事）。
- 保留 pthread：需要 Win pthread 库，不推荐。

涉及文件：`src/main.cpp`, `src/setting.h`, `src/setting.cpp`, 各线程函数声明。

## 3.3 OSPF 报文收发

### Linux 现状

`src/packet_manage.cpp` 使用原始套接字 + `SO_BINDTODEVICE`。

### Windows 建议实现

Win7 环境建议改为 **Npcap/WinPcap（Packet.dll/wpcap.dll）** 来收发以太网帧：

- 发送：构造 Ethernet + IPv4 + OSPF 报文，`pcap_sendpacket()`。
- 接收：`pcap_loop()` 或 `pcap_next_ex()` 过滤协议号 89（OSPF）。
- 绑定接口：通过 Npcap 打开的具体适配器句柄替代 `SO_BINDTODEVICE`。

> 说明：Windows 原始套接字对某些场景限制较多，且接口绑定能力不如 Linux 明确；在教学/实验场景，Npcap 更可控。

## 3.4 路由表写入

### Linux 现状

`src/routing.h` 依赖 `<linux/route.h>`，`src/routing.cpp` 用 `ioctl(SIOCADDRT/SIOCDELRT)`。

### Windows 替代

改为 IP Helper API：

- 添加路由：`CreateIpForwardEntry` / `CreateIpForwardEntry2`
- 删除路由：`DeleteIpForwardEntry` / `DeleteIpForwardEntry2`
- 查询接口索引：`GetAdaptersInfo` 或 `GetAdaptersAddresses`

并将 Linux 的 `rtentry` 缓存结构替换成你自己的跨平台路由记录结构。

## 3.5 守护进程逻辑

`src/main.cpp` 中 `fork/setsid` 只能 Linux 使用。

建议：

- Windows 下禁用 `start_ospfd()` 分支（或改成普通前台程序）。
- 保留控制台交互（`exit` 退出）即可满足实验验证。

## 3.6 网卡/地址配置

当前 `src/setting.cpp` 写死：

- `nic_name = "ens33"`
- `router_id = 192.168.75.128`

Windows 下应改为：

- 从配置文件读取网卡名/IP（推荐新增 `config.ini`）。
- 或启动参数指定（例如 `--ifname`, `--router-id`）。

---

## 4. 构建系统改造建议

当前是 Makefile，Windows 不友好。建议新增 CMake：

- Linux 链接：`pthread`
- Windows 链接：`ws2_32`, `iphlpapi`, `wpcap`, `packet`

如果你坚持 MinGW + Makefile，也可做双分支变量，但维护成本会更高。

---

## 5. eNSP + VMware 配置（Win7 虚拟机场景）

## 5.1 VMware 网络

1. 打开 VMware Virtual Network Editor。
2. 保留 `VMnet8`（NAT）或新建自定义网段（例如 `VMnet2` Host-Only）。
3. 确认 Win7 虚拟机网卡连接到该 VMnet。
4. 在虚拟机网卡高级设置启用允许混杂模式（如果可选，设为 Accept）。

## 5.2 eNSP 连接

1. 在 eNSP 拓扑放置 Cloud。
2. Cloud 绑定到对应 VMware 虚拟网卡（例如 `VMware Network Adapter VMnet8`）。
3. Cloud 接到路由器 GE 接口。
4. 给 eNSP 路由器接口与 Win7 网卡配置同网段地址。

## 5.3 Win7 系统与抓包

1. 安装 Npcap（建议勾选 WinPcap API 兼容）。
2. 关闭 Win7 防火墙（实验环境建议临时关闭）。
3. 使用 Wireshark 在 Win7 网卡抓包，确认看到协议号 89（OSPF）。

---

## 6. 建议的验证顺序

1. **单机自检**：程序启动后能周期发送 Hello。
2. **双向连通**：eNSP 路由器能收到 Win7 发出的 Hello。
3. **邻居建立**：看到 DD/LSR/LSU/LSAck 交互。
4. **路由下发**：Windows 路由表出现 OSPF 学到的网段（`route print`）。
5. **业务验证**：跨网段 `ping` 通。

---

## 7. 关键风险与建议

- Win7 较旧，驱动与 Npcap 版本匹配是常见问题。
- eNSP + VMware + 虚拟网卡叠加时，最常见故障是“网卡绑错”。
- 若仅为验证 OSPF 算法正确性，建议把“报文收发”和“路由写入”封装成接口，先做离线回放测试再做在线联调。

如果你愿意，我可以下一步按这个指南直接给出一版“最小可编译”的跨平台代码骨架（含 `#ifdef _WIN32` 抽象层和 CMakeLists）。
