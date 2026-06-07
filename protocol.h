/*
 * ============================================================
 *   protocol.h —— 自定义应用层协议定义
 *   功能：定义客户端与服务器之间的通信协议格式。
 *   协议格式：字段之间用"|"分隔，字段内的特殊字符
 *            (| % \n \r) 会被转义为 %XX 形式。
 *   每个数据包以换行符 \n 结尾，构成"一行一个包"的协议。
 * ============================================================
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 协议常量定义 */
#define PROTOCOL_MAX_FIELDS     32      /* 一个数据包最多包含的字段数 */
#define PROTOCOL_MAX_FIELD_LEN  4096    /* 每个字段的最大长度（字节） */

/*
 * escapeField —— 转义字段中的特殊字符
 * 将 src 中的 | % \n \r 转换为 %XX 格式（URL编码风格），
 * 确保这些分隔符不会破坏数据包的结构。
 */
void escapeField(const char* src, char* out);

/*
 * unescapeField —— 反转义，恢复原始字符
 * 将 %XX 序列还原为对应的原始字符，是 escapeField 的逆操作。
 */
void unescapeField(const char* src, char* out);

/*
 * splitPacket —— 解析数据包，按 | 分隔符拆分字段
 * 将收到的完整数据包字符串按 | 拆分成多个字段，
 * 同时自动对每个字段执行反转义。
 * 返回值：实际拆分出的字段数量
 */
int splitPacket(const char* packet, char fields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN]);

/*
 * makePacket —— 构建数据包，将多个字段用 | 连接
 * 将多个字段用 | 拼接成一个完整的数据包字符串，
 * 同时对每个字段中的特殊字符进行转义。
 */
void makePacket(const char** fields, int count, char* out, int outSize);
