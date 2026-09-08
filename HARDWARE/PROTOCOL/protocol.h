#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include "sys.h"

/* 树莓派上位机通信协议模块 (USART1)
 * 文本行协议: 命令以 '\n' 或 '\r\n' 结尾
 * 格式: "CMD" 或 "CMD,arg1,arg2,..."
 */

void Protocol_Poll(void);  /* 主循环中轮询, 检查并处理一行新命令 */

#endif
