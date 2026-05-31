# Ticket System 2026

SJTU 数据结构课程大作业：一个通过标准输入/输出交互的火车票管理系统。项目主体使用 C++17 实现，数据持久化在本地二进制文件中，支持用户管理、车次管理、余票查询、购票、候补和退票。

## 项目结构

```text
.
├── CMakeLists.txt              # CMake 构建入口，生成可执行文件 code
├── src/
│   ├── main.cpp                # 程序入口
│   ├── ticket_system.cpp       # 命令分发、参数解析后的业务编排和输出格式化
│   ├── parser.cpp              # 命令行解析
│   ├── user.cpp                # 用户服务实现
│   ├── train.cpp               # 车次、余票、直达和换乘查询实现
│   └── order.cpp               # 购票、订单、退票和候补推进实现
├── include/
│   ├── ticket_system.h
│   ├── model/data_types.h      # 定长记录结构和业务视图结构
│   ├── service/                # UserService / TrainService / OrderService
│   ├── storage/                # B+ 树和记录文件
│   └── util/                   # 时间、字符串缓冲区等工具函数
├── doc/                        # 课程题面和说明文档
├── testcases/                  # 本地测试数据
└── run-test                    # 按 testcases/config.json 执行测试分组
```

## 构建与运行

项目要求 CMake 3.16+ 和支持 C++17 的编译器。

```bash
cmake -S . -B build
cmake --build build
```

可执行文件名为 `code`。如果使用本仓库的 `run-test` 脚本，需要把可执行文件放在仓库根目录：

```bash
cp build/code ./code
./run-test <group-name>
```

直接运行：

```bash
./build/code < testcases/1.in
```

程序从标准输入逐行读取命令，输出格式为首行带原时间戳：

```text
[1] add_user -c root -u admin -p pass -n 管理员 -m admin@mail.com -g 10
[1] 0
```

## 交互命令

所有命令格式为：

```text
[timestamp] command -key value -key value ...
```

参数顺序不限，输入保证合法。多值参数使用 `|` 分隔。

### 用户命令

| 命令 | 参数 | 成功输出 | 失败输出 | 说明 |
| --- | --- | --- | --- | --- |
| `add_user` | `-c -u -p -n -m -g` | `0` | `-1` | 创建用户。第一个用户自动获得权限 `10`；之后要求当前用户已登录且权限高于新用户。 |
| `login` | `-u -p` | `0` | `-1` | 用户登录；重复登录失败。 |
| `logout` | `-u` | `0` | `-1` | 用户登出。 |
| `query_profile` | `-c -u` | `<username> <name> <mailAddr> <privilege>` | `-1` | 查询自己或权限更低用户的信息。 |
| `modify_profile` | `-c -u (-p) (-n) (-m) (-g)` | 同 `query_profile` | `-1` | 修改用户信息；新权限必须低于操作者权限。 |

### 车次命令

| 命令 | 参数 | 成功输出 | 失败输出 | 说明 |
| --- | --- | --- | --- | --- |
| `add_train` | `-i -n -m -s -p -x -t -o -d -y` | `0` | `-1` | 添加未发布车次。站名、价格、行驶时间、停站时间、售卖日期区间用 `|` 分隔。 |
| `delete_train` | `-i` | `0` | `-1` | 删除未发布车次。已发布车次不可删除。 |
| `release_train` | `-i` | `0` | `-1` | 发布车次，并把每个经停站加入车站索引。发布后可查询和购票。 |
| `query_train` | `-i -d` | 车次和逐站信息 | `-1` | 查询某始发日期下车次运行信息。未售出票段显示初始座位数。 |
| `query_ticket` | `-s -t -d (-p time)` | 首行数量，后续为车票列表 | 无匹配时输出 `0` | 查询直达车票，按时间或价格排序。 |
| `query_transfer` | `-s -t -d (-p time)` | 两行换乘方案 | `0` | 查询恰好换乘一次的最优方案。 |

`query_ticket` 和 `query_transfer` 的输出行格式：

```text
<trainID> <FROM> <LEAVING_TIME> -> <TO> <ARRIVING_TIME> <PRICE> <SEAT>
```

### 订单命令

| 命令 | 参数 | 成功输出 | 失败输出 | 说明 |
| --- | --- | --- | --- | --- |
| `buy_ticket` | `-u -i -d -n -f -t (-q false)` | 总价或 `queue` | `-1` | 购买已发布车次的车票。余票不足且 `-q true` 时生成候补订单。 |
| `query_order` | `-u` | 首行数量，后续为订单列表 | `-1` | 查询用户订单，按下单时间从新到旧输出。 |
| `refund_ticket` | `-u (-n 1)` | `0` | `-1` | 退订从新到旧第 `n` 个订单。成功退票后尝试推进同车次同运行日的候补订单。 |

订单输出格式：

```text
[success|pending|refunded] <trainID> <FROM> <LEAVING_TIME> -> <TO> <ARRIVING_TIME> <PRICE> <NUM>
```

### 系统命令

| 命令 | 输出 | 说明 |
| --- | --- | --- |
| `clean` | `0` | 清空所有持久化数据并重新初始化索引。 |
| `exit` | `bye` | 退出程序。 |

## 实现逻辑

### 总控流程

`main.cpp` 创建 `sjtu::TicketSystem` 后进入 `run()`。`CommandParser` 解析每行输入，`TicketSystem::excute` 按命令名分发到对应 handler。handler 只负责参数转换和输出格式化，实际业务由三个 service 完成：

- `UserService`：用户文件、用户索引和运行期登录状态。
- `TrainService`：车次文件、座位文件、车次索引、车站索引、座位索引，以及直达/换乘查询。
- `OrderService`：订单文件、用户订单索引、候补订单索引，以及购票/退票流程。

### 持久化存储

底层存储由两部分组成：

- `RecordFile<Record>`：定长记录文件，文件头维护总槽位数、有效记录数和空闲链表。删除记录会回收到 free list，后续插入可复用槽位。
- `BPT<Data>`：磁盘 B+ 树，`Data` 由字符串哈希 key 和 int value 组成。节点大小为 128，叶子节点通过 `prev/next` 串联，内部有 256 个节点的 LRU 写回缓存。

系统运行目录下会生成若干数据文件，例如：

```text
ts_users.dat
ts_trains.dat
ts_seats.dat
ts_order_dat
init_ts_user_index / data_ts_user_index
init_ts_train_index / data_ts_train_index
init_ts_station_index / data_ts_station_index
init_ts_seat_index / data_ts_seat_index
init_ts_order_index / data_ts_order_index
init_ts_pending_index / data_ts_pending_index
```

### 用户模块

用户记录使用 `UserProfile` 定长保存。`user_index_` 建立 `username -> user_file offset` 的映射。

核心规则：

- 第一个用户不检查 `-c`，直接创建为权限 `10`。
- 后续 `add_user` 要求操作者已登录，且操作者权限严格高于新用户权限。
- `query_profile` 和 `modify_profile` 允许操作自己；操作其他用户时要求操作者权限严格高于目标用户权限。
- 登录状态只保存在内存中的 `logged_in_`，程序重启后全部下线。

### 车次与余票模块

车次记录使用 `TrainRecord` 保存站点、票价、运行时间、售卖日期和发布状态。

关键预处理：

- `prefix_prices[i]` 保存从始发站到第 `i` 站的累计票价，区间票价可 O(1) 得到。
- `arrival_offsets` / `departure_offsets` 保存相对始发时间的分钟偏移，跨天时用绝对分钟转换输出日期时间。
- 发布车次时，将每个站点写入 `station_index_`，value 通过 `train_offset` 和 `station_index` 打包，供直达和换乘查询使用。

余票使用 `SeatRecord` 按“车次 offset + 运行日”懒创建。一条记录保存每个区间的剩余座位数；查询一段旅程余票时取覆盖区间的最小值。

### 查询逻辑

`query_ticket`：

1. 在车站索引中分别找出出发站和到达站相关车次。
2. 枚举候选数量更少的一侧。
3. 读取车次后定位另一站，要求出发站序号小于到达站序号。
4. 根据用户给出的出发站日期反推出车次始发运行日。
5. 读取或默认计算余票，生成结果后按 `time` 或 `cost` 排序。

`query_transfer`：

1. 先从所有能到达终点站的车次中枚举可能的第二程，并按换乘站分组。
2. 再枚举从起点出发的第一程车次和其中间站。
3. 对每个可连接的第二程，计算第二程最早可搭乘的运行日，要求第二程出发时间不早于第一程到达时间。
4. 计算两段票价、总时间和两段余票，按 `time` 或 `cost` 策略保留最优方案。
5. 查询过程中使用小型 LRU 缓存减少重复读取车次记录。

### 订单与候补模块

`buy_ticket` 流程：

1. 检查用户已登录、车次存在且已发布、站点顺序合法、购票数量不超过总座位数。
2. 根据出发站日期解析实际运行日，并加载或创建对应 `SeatRecord`。
3. 如果区间最小余票足够，扣减每个覆盖区间余票，创建 `success` 订单并返回总价。
4. 如果余票不足且允许候补，创建 `pending` 订单，并写入候补索引。
5. 如果不允许候补或校验失败，返回 `-1`。

`refund_ticket` 流程：

- 退候补订单：直接标记为 `refunded`，并从候补索引删除。
- 退成功订单：归还区间余票，标记为 `refunded`，然后调用 `try_promote_pending_orders`。
- 候补推进按同车次、同运行日扫描候补订单；只要当前余票能完整满足某订单，就扣票并把该订单改为 `success`。

## 测试

仓库提供 `run-test`，它会把根目录的 `code` 复制到临时目录运行，避免测试之间的数据文件互相污染。

```bash
cmake -S . -B build
cmake --build build
cp build/code ./code
./run-test <group-name>
```

可查看可用测试分组：

```bash
jq -r '.Groups[].GroupName' testcases/config.json
```

手动测试时建议先执行：

```text
[1] clean
```

以清除当前运行目录下的历史数据文件。
