#include "protocol.h"

static int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

static const char* digits = "0123456789ABCDEF";

void escapeField(const char* src, char* out)
{
    while (*src)
    {
        unsigned char ch = (unsigned char)*src;
        if (ch == '|' || ch == '%' || ch == '\n' || ch == '\r')
        {
            *out++ = '%';
            *out++ = digits[(ch >> 4) & 0x0F];
            *out++ = digits[ch & 0x0F];
        }
        else
        {
            *out++ = (char)ch;
        }
        src++;
    }
    *out = '\0';
}

void unescapeField(const char* src, char* out)
{
    while (*src)
    {
        if (*src == '%' && *(src + 1) && *(src + 2))
        {
            int high = hexValue(*(src + 1));
            int low = hexValue(*(src + 2));
            if (high >= 0 && low >= 0)
            {
                *out++ = (char)((high << 4) | low);
                src += 3;
                continue;
            }
        }
        *out++ = *src++;
    }
    *out = '\0';
}

int splitPacket(const char* packet, char fields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN])
{
    int count = 0;

    if (!packet || !*packet)
        return 0;

    while (*packet && count < PROTOCOL_MAX_FIELDS)
    {
        char* out = fields[count];
        int outPos = 0;

        while (*packet && *packet != '|' && outPos < PROTOCOL_MAX_FIELD_LEN - 1)
        {
            if (*packet == '%' && *(packet + 1) && *(packet + 2))
            {
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
        out[outPos] = '\0';

        count++;
        if (*packet == '|') packet++;
    }

    return count;
}

void makePacket(const char** fields, int count, char* out, int outSize)
{
    int pos = 0;

    for (int i = 0; i < count && pos < outSize - 1; i++)
    {
        if (i > 0)
        {
            out[pos++] = '|';
            if (pos >= outSize - 1) break;
        }

        const char* src = fields[i];
        while (*src && pos < outSize - 1)
        {
            unsigned char ch = (unsigned char)*src;
            if (ch == '|' || ch == '%' || ch == '\n' || ch == '\r')
            {
                if (pos + 3 >= outSize - 1) break;
                out[pos++] = '%';
                out[pos++] = digits[(ch >> 4) & 0x0F];
                out[pos++] = digits[ch & 0x0F];
            }
            else
            {
                out[pos++] = (char)ch;
            }
            src++;
        }
    }
    out[pos] = '\0';
}
