#include "protocol.h"

// 把十六进制字符转换成对应数值。
// 主要用于解析经过转义后的 %XX 形式内容。
static int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return -1;
}

// 对单个字段做转义，避免字段内容中的 |、% 和换行符
// 破坏自定义协议的分隔格式。
std::string escapeField(const std::string& value)
{
    static const char* digits = "0123456789ABCDEF";
    std::string result;

    for (unsigned char ch : value)
    {
        if (ch == '|' || ch == '%' || ch == '\n' || ch == '\r')
        {
            result.push_back('%');
            result.push_back(digits[(ch >> 4) & 0x0F]);
            result.push_back(digits[ch & 0x0F]);
        }
        else
        {
            result.push_back((char)ch);
        }
    }

    return result;
}

// 把 %XX 形式的转义内容还原回原始字符。
std::string unescapeField(const std::string& value)
{
    std::string result;

    for (size_t i = 0; i < value.size(); i++)
    {
        if (value[i] == '%' && i + 2 < value.size())
        {
            int high = hexValue(value[i + 1]);
            int low = hexValue(value[i + 2]);
            if (high >= 0 && low >= 0)
            {
                result.push_back((char)((high << 4) | low));
                i += 2;
                continue;
            }
        }

        result.push_back(value[i]);
    }

    return result;
}

// 按 | 分隔一个完整协议包，并对每个字段做反转义。
std::vector<std::string> splitPacket(const std::string& packet)
{
    std::vector<std::string> fields;
    std::string current;

    for (char ch : packet)
    {
        if (ch == '|')
        {
            fields.push_back(unescapeField(current));
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    fields.push_back(unescapeField(current));
    return fields;
}

// 把多个字段拼成一个协议包，字段之间用 | 分隔，
// 并先对每个字段执行转义。
std::string makePacket(const std::vector<std::string>& fields)
{
    std::string packet;

    for (size_t i = 0; i < fields.size(); i++)
    {
        if (i > 0)
        {
            packet.push_back('|');
        }
        packet += escapeField(fields[i]);
    }

    return packet;
}
