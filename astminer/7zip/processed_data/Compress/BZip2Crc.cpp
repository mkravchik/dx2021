#include "StdAfx.h"
#include "BZip2Crc.h"

UInt32 CBZip2Crc::Table[256];
static const UInt32 kBZip2CrcPoly = 0x04c11db7;
void CBZip2Crc::InitTable() {
  for (UInt32 i = 0; i < 256; i++) {
    UInt32 r = (i << 24);
    for (unsigned j = 0; j < 8; j++)
      r = (r << 1) ^ (kBZip2CrcPoly & ((UInt32)0 - (r >> 31)));
    Table[i] = r; } }
class CBZip2CrcTableInit {
public:
  CBZip2CrcTableInit() { CBZip2Crc::InitTable(); }
} g_BZip2CrcTableInit;
