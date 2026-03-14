# RuOSPFd

20级信息类-6系 大三下计算机网络实验 提高层次实验——OSPF协议实现（基于Linux内核）


> 笑死，原本想写成守护进程的所以取名后有一个d；但最终并没有，不过鉴于尾缀d有种平衡的美感，就不改删了

### 目录结构

- `doc`：文档，包含一些实现时的注意事项
- `src`：实现代码

```
main.cpp				# 主函数，开关程序以及相应线程，初始化
setting.cpp/.h			# OSPF配置 + 全局通用变量/常量
common.h				# 通用头文件汇总，供.cpp直接引用，根据规约不在.h中引用
----------------
ospf_packet.cpp/.h		# 定义OSPF报文和LSA数据结构
interface.cpp/.h		# 定义OSPF协议中接口数据结构
neighbor.cpp/.h			# 定义OSPF协议中邻居数据结构
lsdb.cpp/.h				# 定义OSPF协议中连接状态数据库结构
----------------
package_manage.cpp/.h	# 负责处理报文收发逻辑
lsa_manage.cpp/.h		# 负责处理LSA的生成
routing.cpp/.h			# 负责路由计算，内核路由表更新与重置（提供route_manager）
----------------
retransmitter.cpp/.h	# 重传工具
```

- `try`：一些功能技术小试验

### 总体设计

<img src="https://i.postimg.cc/k44LcsRs/image.png" style="width:60%;" />

### 运行方式

根目录 `make` 编译，然后`sudo ./my_ospf` 执行 （啊目前还没写检测环境+动态配置，所以不手动调整`setting.cpp/.h` 的话只能在我自己的虚拟机上跑doge）；输入 `exit` 退出程序

**精简控制台输出：**去除 `setting.h` 的 `#define DEBUG`

调试环境中g++用的标准是C++14，但估计顶多用到C++11的功能为止

**仿真验证环境搭建：**华为ensp接入VMWare。在ensp搭建网络拓扑结构，然后连入VMWare的NAT虚拟网络适配器（`VMware Network Adapter VMnet8`）；同时配置虚拟机网络连接为NAT模式。（参考链接：[Ensp接入VMWare](https://blog.51cto.com/u_15107455/5173737)，其中端口映射的类型若为E，然后使用ensp路由器GE接口连接，似乎也没问题；不过感觉设置为一样最好）

<img src="https://i.postimg.cc/nhkZ8M5d/ensp.png" style="width:60%;" />


### 在VMware + eNSP + Ubuntu环境下的建议配置

1. **VMware网络建议使用同一网段**：Win7(eNSP) 与 Ubuntu(运行本项目) 都挂到 `VMnet8(NAT)`，并确认网段一致（例如你当前的 `192.168.234.131/24` 与 `192.168.234.133/24`）。
2. **在eNSP中接入VMnet8**：通过 Cloud/桥接方式把路由器 GE 口接入 `VMware Network Adapter VMnet8`，并给路由器接口配置同网段地址。
3. **关闭或放通Win7防火墙**：OSPF协议号是 IP Proto 89，不走TCP/UDP端口，建议先临时关闭防火墙排查。
4. **Ubuntu启用转发（按需）**：若Ubuntu需要承担转发角色，执行 `sudo sysctl -w net.ipv4.ip_forward=1`。

本项目默认写死了网卡名/IP（原为 `ens33` + `192.168.75.128`），现在支持通过环境变量覆盖：

```bash
export OSPF_NIC=ens33
export OSPF_INTERFACE_IP=192.168.234.133
export OSPF_ROUTER_ID=192.168.234.133   # 可选，不设置时默认等于接口IP
sudo -E ./my_ospf
```

如果Ubuntu网卡名不是 `ens33`（常见如 `ens160`），必须同步修改 `OSPF_NIC`。

### 常见问题：`dis ospf peer` 显示邻居卡在 `Init`

如果在 eNSP 路由器上能看到 Ubuntu 的 Router ID，但状态一直是 `Init`，通常表示 **单向 Hello**：

- eNSP 收到了 Ubuntu 发来的 Hello；
- 但 Ubuntu 的 Hello 邻居列表中没有（或还没有）包含 eNSP 的 Router ID；
- 因而 eNSP 无法进入 `2-Way/Full`。

结合本项目实现，Hello 报文的邻居列表来自 `interface->neighbor_list`。只有当 Ubuntu 先收到对端 Hello 后，才会把对端 Router ID 回填到后续 Hello 中。

排查建议：

1. 先确认双向收包（尤其是 Ubuntu 是否能收到 eNSP 发往 `224.0.0.5` 的 OSPF 报文）；
2. 重点检查 Win7 防火墙、eNSP Cloud 绑定网卡是否正确、VMnet8 是否允许该方向组播/协议 89；
3. 确认双方 OSPF 参数一致：Area、Hello/Dead、掩码、认证。

可在 Ubuntu 执行：

```bash
sudo tcpdump -ni ens33 'ip proto 89 or host 224.0.0.5'
```

若只看到 Ubuntu 发包而看不到 eNSP 回包，问题基本不在本程序状态机，而在虚拟网络互通链路（Cloud/防火墙/网卡绑定）。

### 参考资料
- RFC 2328：有种规范的美感
- 《OSPF完全实现》及其源码：比较复杂完整，没有太多精力借鉴。[源码分享](https://pan.baidu.com/s/1tMO2Cf92Iy1mc2eP56qvlQ)，提取码：dz89 
- [pcyin/OSPF_Router](https://github.com/pcyin/OSPF_Router)：感恩，对照着写的（但读了遍有好多bug，感觉跑不起来的样子）
