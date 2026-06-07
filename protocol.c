/*
 * ============================================================
 *   protocol.c —— 自定义应用层协议的具体实现
 *   实现了 URL风格的转义/反转义机制，确保包含特殊字符
 *   （| % \n \r）的数据能在"一行一个包"的TCP流中安全传输。
 * ============================================================
 */
#include "protocol.h"

/*
 * hexValue —— 将十六进制字符转换为数值
 * 例如 'A'/'a' -> 10, '3' -> 3
 * 用于解析 %XX 形式的转义序列
 */
static int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;  /* 非十六进制字符 */
}

/* 十六进制数字字符表，用于生成 %XX 转义序列 */
static const char* digits = "0123456789ABCDEF";

/*
 * escapeField —— 转义字段中的特殊字符
 * 协议中保留 | 作为字段分隔符，% 作为转义标记，
 * \n 和 \r 不能被包含在行内（因为TCP是逐行传输的）。
 * 将这4个字符转换为 %XX 格式（如 \n -> %0A）。
 */
void escapeField(const char* src, char* out)
{
    while (*src)
    {
        unsigned char ch = (unsigned char)*src;
        if (ch == '|' || ch == '%' || ch == '\n' || ch == '\r')
        {
            /* 将特殊字符编码为 %XX 形式 */
            *out++ = '%';
            *out++ = digits[(ch >> 4) & 0x0F];  /* 高4位 → 十六进制字符 */
            *out++ = digits[ch & 0x0F];          /* 低4位 → 十六进制字符 */
        }
        else
        {
            /* 普通字符直接复制 */
            *out++ = (char)ch;
        }
        src++;
    }
    *out = '\0';  /* 字符串结束符 */
}

/*
 * unescapeField —— 反转义，恢复原始字符
 * 将 %XX 格式的转义序列还原为原始字符，非转义字符保持原样。
 */
void unescapeField(const char* src, char* out)
{
    while (*src)
    {
        if (*src == '%' && *(src + 1) && *(src + 2))
        {
            /* 遇到 % 且后面有2个字符，尝试解析为十六进制转义 */
            int high = hexValue(*(src + 1));
            int low = hexValue(*(src + 2));
            if (high >= 0 && low >= 0)
            {
                /* 将两个十六进制数组合成一个字节 */
                *out++ = (char)((high << 4) | low);
                src += 3;  /* 跳过 %XX 三个字符 */
                continue;
            }
        }
        /* 非转义字符直接复制 */
        *out++ = *src++;
    }
    *out = '\0';
}

/*
 * splitPacket —— 解析数据包，按 | 分隔符拆分字段
 *
 * 处理流程：
 * 1. 从数据包头部开始扫描
 * 2. 遇到 | 时结束当前字段，开始下一个字段
 * 3. 每个字段在提取时自动完成反转义（%XX → 原始字符）
 * 4. 直接写入目标字段缓冲区，避免使用中间缓冲区
 *
 * 返回值：拆分出的字段数；如果数据包为空则返回 0
 */
int splitPacket(const char* packet, char fields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN])
{
    int count = 0;  /* 已拆分的字段计数 */

    if (!packet || !*packet)
        return 0;  /* 空数据包 */

    while (*packet && count < PROTOCOL_MAX_FIELDS)
    {
        char* out = fields[count];
        int outPos = 0;

        /* 从当前位置复制字符到当前字段，直到遇到 | 或字符串结尾 */
        while (*packet && *packet != '|' && outPos < PROTOCOL_MAX_FIELD_LEN - 1)
        {
            if (*packet == '%' && *(packet + 1) && *(packet + 2))
            {
                /* 在拆分过程中直接反转义，避免中间缓冲区截断问题 */
                int high = hexValue(*(packet + 1));
                int low  = hexValue(*(packet + 2));
                if (high >= 0 && low >= 0)
                {
                    out[outPos++] = (char)((high << 4) | low);
                    packet += 3;
                    continue;
                }
            }
            out[outPos++] = *packet++;
        }
        out[outPos] = '\0';  /* 当前字段结束 */

        count++;
        if (*packet == '|') packet++;  /* 跳过字段分隔符 */
    }

    return count;
}

/*
 * makePacket —— 构建数据包，将多个字段用 | 拼接
 *
 * 处理流程：
 * 1. 遍历所有字段
 * 2. 在每个字段之间插入 | 作为分隔符
 * 3. 对每个字段中的特殊字符（| % \n \r）进行转义
 * 4. 直接转义写入输出缓冲区，不使用中间缓冲区
 *    这样避免了固定大小中间缓冲区可能导致的溢出问题
 *
 * 核心设计思路：
 * - 字段内容中的特殊字符被转义后，| 和 \n 就不会被
 *   误认为是协议分隔符，保证了数据传输的正确性
 * - 对于大字段（如航班列表表格），可以直接处理而不会溢出
 */
void makePacket(const char** fields, int count, char* out, int outSize)
{
    int pos = 0;  /* 输出缓冲区的当前写入位置 */

    for (int i = 0; i < count && pos < outSize - 1; i++)
    {
        /* 除第一个字段外，其他字段前插入 | 分隔符 */
        if (i > 0)
        {
            out[pos++] = '|';
            if (pos >= outSize - 1) break;
        }

        /* 逐字符处理当前字段，边转义边写入输出缓冲区 */
        const char* src = fields[i];
        while (*src && pos < outSize - 1)
        {
            unsigned char ch = (unsigned char)*src;
            if (ch == '|' || ch == '%' || ch == '\n' || ch == '\r')
            {
                /* 特殊字符需要转义为 %XX 格式，需要3字节空间 */
                if (pos + 3 >= outSize - 1) break;  /* 空间不足则截断 */
                out[pos++] = '%';
                out[pos++] = digits[(ch >> 4) & 0x0F];
                out[pos++] = digits[ch & 0x0F];
            }
            else
            {
                /* 普通字符直接写入 */
                out[pos++] = (char)ch;
            }
            src++;
        }
    }
    out[pos] = '\0';  /* 字符串结束符 */
}
