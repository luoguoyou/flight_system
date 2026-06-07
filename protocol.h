#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOCOL_MAX_FIELDS 32
#define PROTOCOL_MAX_FIELD_LEN 4096

// Escape special chars (|, %, \n, \r) in src into out buffer
void escapeField(const char* src, char* out);

// Unescape %XX sequences back to original chars
void unescapeField(const char* src, char* out);

// Split a packet by | delimiter, returns field count (up to PROTOCOL_MAX_FIELDS)
int splitPacket(const char* packet, char fields[PROTOCOL_MAX_FIELDS][PROTOCOL_MAX_FIELD_LEN]);

// Join multiple fields with | separator into one packet string
void makePacket(const char** fields, int count, char* out, int outSize);
