# STM32 ↔ 树莓派 串口通信 API 文档

## 1. 通信概述

树莓派作为上层决策端，通过串口（UART）向 STM32 发送控制指令。STM32 作为底层执行端，负责电机驱动、传感器采集、舵机控制，并将结果回复给树莓派。

```
树莓派 (上层决策)
  │
  │  UART 115200 bps, 8N1
  │
  ▼
STM32 (底层执行)
  ├── 4× 麦克纳姆轮电机 (L298N)
  ├── 超声波测距 (HC-SR04)
  ├── 红外循迹 (4路)
  ├── 舵机 (SG90, 带超声波探头)
  └── 电池电压 ADC
```

## 2. 串口参数

| 参数 | 值 |
|------|-----|
| 波特率 | 115200 |
| 数据位 | 8 |
| 校验位 | 无 |
| 停止位 | 1 |
| 数据流向 | 树莓派 TX → STM32 USART1 RX (PA10) |

> **硬件连接**: 树莓派 TX → STM32 PA10 (USART1_RX), 树莓派 RX → STM32 PA9 (USART1_TX), **共地必须接！**

## 3. 协议格式

### 3.1 发送帧（树莓派 → STM32）

```
$<CMD>,<参数1>,<参数2>,...\n
```

- 以 `$` 开头
- 逗号分隔参数
- 以换行符 `\n` (0x0A) 结尾
- **区分大小写**

### 3.2 回复帧（STM32 → 树莓派）

**成功:**
```
$OK,<数据1>,<数据2>,...\n
```

**失败:**
```
$ERR,<错误码>,<描述>\n
```

### 3.3 超时处理

树莓派发送命令后，**应在 500ms 内收到回复**。若超时未收到，可重发一次。若连续 3 次无回复，STM32 可能离线。

## 4. API 命令一览

### 4.1 心跳检测 `$PING`

| 项目 | 内容 |
|------|------|
| 命令 | `$PING\n` |
| 说明 | 检测 STM32 是否在线 |
| 参数 | 无 |
| 回复 | `$OK,PONG` |
| 使用场景 | 树莓派启动时握手、运行时保活 |

**示例:**
```
→ $PING
← $OK,PONG
```

---

### 4.2 运动控制 `$MOV`

| 项目 | 内容 |
|------|------|
| 命令 | `$MOV,<方向>,<速度>\n` |
| 说明 | 控制麦克纳姆轮全向运动 |

**方向参数:**

| 值 | 动作 | 说明 |
|----|------|------|
| 0 | 停车 | 立即停止所有电机 |
| 1 | 前进 | 四轮正转 |
| 2 | 后退 | 四轮反转 |
| 3 | 横移右 | 麦克纳姆右平移 |
| 4 | 横移左 | 麦克纳姆左平移 |
| 5 | 原地右转 | 顺时针旋转 |
| 6 | 原地左转 | 逆时针旋转 |

**速度参数:**

| 范围 | 说明 |
|------|------|
| 0 | 停车（方向=0 时使用） |
| 600~1999 | 有效速度范围，值越大越快 |
| 600 | 最低启动速度（克服 JGB37-520 静摩擦） |
| 1500 | 建议常用速度（~75% 占空比） |
| 1999 | 最大速度（满速） |

**回复:** `$OK`

**示例:**
```
→ $MOV,1,1500     # 前进，速度 1500
← $OK

→ $MOV,3,1000     # 横移右，速度 1000
← $OK

→ $MOV,0,0        # 停车
← $OK

→ $MOV,5,800      # 原地右转，速度 800
← $OK
```

**麦克纳姆轮运动示意（俯视）:**
```
        前
  FL(\)    FR(/)
    ↘      ↙
     ↘    ↙
  RL(/)    RR(\)
    ↙      ↘
        后

前/后: 四轮同向
横移:  对角线轮同向
旋转:  左右轮反向
```

---

### 4.3 读超声波距离 `$DIS`

| 项目 | 内容 |
|------|------|
| 命令 | `$DIS\n` |
| 说明 | 读取超声波传感器距离 |
| 回复 | `$OK,<距离cm>` |

**回复字段:**

| 字段 | 说明 |
|------|------|
| 距离cm | 整数，单位 cm，范围 2~400 |

**示例:**
```
→ $DIS
← $OK,25

→ $DIS
← $OK,12
```

---

### 4.4 读电池电压 `$BAT`

| 项目 | 内容 |
|------|------|
| 命令 | `$BAT\n` |
| 说明 | 读取电池电压（经分压采样） |
| 回复 | `$OK,<电压V>` |

**回复字段:**

| 字段 | 说明 |
|------|------|
| 电压V | 浮点数，单位 V，精度约 0.05V |

**示例:**
```
→ $BAT
← $OK,11.80

→ $BAT
← $OK,11.25
```

---

### 4.5 舵机控制 `$SRV`

| 项目 | 内容 |
|------|------|
| 命令 | `$SRV,<角度>\n` |
| 说明 | 控制舵机角度（带动超声波探头转向） |

**角度参数:**

| 值 | 位置 | 说明 |
|----|------|------|
| 50 | 左转 | 探头朝左 |
| 80 | 正前方 | 默认居中位置 |
| 110 | 右转 | 探头朝右 |

**回复:** `$OK`

**示例:**
```
→ $SRV,80       # 探头回正
← $OK

→ $SRV,50       # 探头左转
← $OK
```

---

### 4.6 读循迹传感器 `$TRK`

| 项目 | 内容 |
|------|------|
| 命令 | `$TRK\n` |
| 说明 | 读取 4 路红外循迹传感器状态 |
| 回复 | `$OK,<HW1>,<HW2>,<HW3>,<HW4>` |

**回复字段:**

| 值 | 说明 |
|----|------|
| 0 | 未检测到黑线（白色地面） |
| 1 | 检测到黑线（压在线上） |

**传感器物理排列（从左到右）:**
```
  HW4(PA15)  HW3(PB3)  HW2(PB4)  HW1(PB5)
    [4]        [3]       [2]       [1]
      ←────── 车头方向 ──────→
```

**典型循迹场景:**

| HW4 | HW3 | HW2 | HW1 | 状态 | 建议动作 |
|-----|-----|-----|-----|------|----------|
| 0 | 0 | 0 | 0 | 在白地上，线丢失 | 直行或停止 |
| 0 | 0 | 0 | 1 | 偏右很多 | 大幅右转 |
| 0 | 0 | 1 | 0 | 轻微偏右 | 小幅右转 |
| 0 | 1 | 0 | 0 | 轻微偏左 | 小幅左转 |
| 1 | 0 | 0 | 0 | 偏左很多 | 大幅左转 |
| 0 | 1 | 1 | 0 | 在线正中 | 直行 |
| 1 | 1 | 1 | 1 | 全在黑线上 | 直行 |

**示例:**
```
→ $TRK
← $OK,0,0,1,0

→ $TRK
← $OK,1,0,0,0
```

---

### 4.7 读全量传感器 `$SEN`

| 项目 | 内容 |
|------|------|
| 命令 | `$SEN\n` |
| 说明 | 一次性读取所有传感器数据 |
| 回复 | `$OK,<距离>,<电压>,<HW1>,<HW2>,<HW3>,<HW4>` |

**回复字段:**

| 字段 | 说明 |
|------|------|
| 距离 | 超声波距离 (cm) |
| 电压 | 电池电压 (V) |
| HW1~4 | 循迹传感器 (0/1) |

**示例:**
```
→ $SEN
← $OK,25,11.80,0,0,1,0
```

---

## 5. 错误码

| 错误码 | 说明 |
|--------|------|
| `E01` | 未知命令（命令格式错误） |
| `E02` | 参数错误（参数超出范围） |
| `E03` | 参数不足（缺少必要参数） |

**示例:**
```
→ $MOV,8,1000     # 方向 8 不存在
← $ERR,E02,DIR

→ $MOV,1          # 缺少速度参数
← $ERR,E03,ARG
```

---

## 6. 树莓派端 Python 参考代码

```python
import serial
import time

class STM32Controller:
    def __init__(self, port='/dev/ttyAMA0', baudrate=115200):
        """初始化串口连接"""
        self.ser = serial.Serial(port, baudrate, timeout=0.5)
        time.sleep(0.5)  # 等待 STM32 复位稳定

    def _send(self, cmd):
        """发送命令并接收回复"""
        self.ser.write((cmd + '\n').encode())
        response = self.ser.readline().decode().strip()
        if response.startswith('$OK'):
            parts = response.split(',')
            return True, parts[1:] if len(parts) > 1 else []
        elif response.startswith('$ERR'):
            parts = response.split(',')
            return False, parts[1] if len(parts) > 1 else 'UNKNOWN'
        return False, 'NO_RESPONSE'

    def ping(self):
        """心跳检测"""
        ok, data = self._send('$PING')
        return ok and data and data[0] == 'PONG'

    def move(self, direction, speed=1500):
        """
        运动控制
        direction: 0=停 1=前 2=后 3=右 4=左 5=右转 6=左转
        speed: 600~1999
        """
        ok, _ = self._send(f'$MOV,{direction},{speed}')
        return ok

    def forward(self, speed=1500):
        return self.move(1, speed)

    def backward(self, speed=1500):
        return self.move(2, speed)

    def strafe_right(self, speed=1500):
        return self.move(3, speed)

    def strafe_left(self, speed=1500):
        return self.move(4, speed)

    def rotate_cw(self, speed=1000):
        return self.move(5, speed)

    def rotate_ccw(self, speed=1000):
        return self.move(6, speed)

    def stop(self):
        return self.move(0, 0)

    def get_distance(self):
        """读取超声波距离 (cm)"""
        ok, data = self._send('$DIS')
        if ok and data:
            return int(data[0])
        return -1

    def get_battery(self):
        """读取电池电压 (V)"""
        ok, data = self._send('$BAT')
        if ok and data:
            return float(data[0])
        return -1.0

    def set_servo(self, angle):
        """设置舵机角度 (50~110)"""
        ok, _ = self._send(f'$SRV,{angle}')
        return ok

    def get_tracking(self):
        """读取循迹传感器 [HW1, HW2, HW3, HW4]"""
        ok, data = self._send('$TRK')
        if ok and len(data) == 4:
            return [int(x) for x in data]
        return [0, 0, 0, 0]

    def get_all_sensors(self):
        """一次性读取所有传感器"""
        ok, data = self._send('$SEN')
        if ok and len(data) == 6:
            return {
                'distance': int(data[0]),
                'battery': float(data[1]),
                'tracking': [int(x) for x in data[2:6]]
            }
        return None

    def close(self):
        self.ser.close()


# === 使用示例 ===

if __name__ == '__main__':
    car = STM32Controller('/dev/ttyAMA0', 115200)

    # 检测连接
    if not car.ping():
        print("STM32 不在线!")
        exit()

    print("连接成功!")

    # 基础运动测试
    car.forward(1200)       # 前进
    time.sleep(1)
    car.stop()

    # 跟随示例
    while True:
        dist = car.get_distance()
        if dist < 0:
            continue
        if dist > 25:
            car.forward(1500)
        elif dist < 15:
            car.backward(1000)
        else:
            car.stop()
        time.sleep(0.1)

    car.close()
```

---

## 7. 接线参考

```
树莓派              STM32
─────────         ─────────
GPIO14 (TX)  ──→  PA10 (USART1_RX)
GPIO15 (RX)  ←──  PA9  (USART1_TX)
GND          ──   GND    ← 必须共地!

如果树莓派 GPIO 是3.3V, 可直连 STM32 (也是3.3V)
如果树莓派 GPIO 是5V, TX 接 STM32 3.3V 需要电平转换(或串联1k电阻保护)
```

## 8. 注意事项

1. **共地必须连接**，否则串口通信不稳定
2. STM32 上电后约 **500ms** 初始化时间，树莓派启动后应等待再发命令
3. 速度参数范围 **600~1999**，低于 600 电机可能不转（克服静摩擦）
4. 舵机角度范围 **50~110**，超出范围会被 STM32 拒绝
5. 电池电压读数需校准：当前分压比系数写死在代码中，更换分压电阻后需修改
