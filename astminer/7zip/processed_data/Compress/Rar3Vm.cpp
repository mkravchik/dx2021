#include "StdAfx.h"
#include <stdlib.h>
#include "../../../C/7zCrc.h"
#include "../../../C/Alloc.h"
#include "../../Common/Defs.h"
#include "Rar3Vm.h"
namespace NCompress {
namespace NRar3 {
UInt32 CMemBitDecoder::ReadBits(unsigned numBits) {
  UInt32 res = 0;
  for (;;) {
    unsigned b = _bitPos < _bitSize ? (unsigned)_data[_bitPos >> 3] : 0;
    unsigned avail = (unsigned)(8 - (_bitPos & 7));
    if (numBits <= avail) {
      _bitPos += numBits;
      return res | (b >> (avail - numBits)) & ((1 << numBits) - 1); }
    numBits -= avail;
    res |= (UInt32)(b & ((1 << avail) - 1)) << numBits;
    _bitPos += avail; } }
UInt32 CMemBitDecoder::ReadBit() { return ReadBits(1); }
UInt32 CMemBitDecoder::ReadEncodedUInt32() {
  unsigned v = (unsigned)ReadBits(2);
  UInt32 res = ReadBits(4 << v);
  if (v == 1 && res < 16)
    res = 0xFFFFFF00 | (res << 4) | ReadBits(4);
  return res; }
namespace NVm {
static const UInt32 kStackRegIndex = kNumRegs - 1;
CVm::CVm(): Mem(NULL) {}
bool CVm::Create() {
  if (!Mem)
    Mem = (Byte *)::MyAlloc(kSpaceSize + 4);
  return (Mem != NULL); }
CVm::~CVm() {
  ::MyFree(Mem); }

bool CVm::Execute(CProgram *prg, const CProgramInitState *initState,
    CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData) {
  memcpy(R, initState->InitR, sizeof(initState->InitR));
  R[kStackRegIndex] = kSpaceSize;
  R[kNumRegs] = 0;
  Flags = 0;
  UInt32 globalSize = MyMin((UInt32)initState->GlobalData.Size(), kGlobalSize);
  if (globalSize != 0)
    memcpy(Mem + kGlobalOffset, &initState->GlobalData[0], globalSize);
  UInt32 staticSize = MyMin((UInt32)prg->StaticData.Size(), kGlobalSize - globalSize);
  if (staticSize != 0)
    memcpy(Mem + kGlobalOffset + globalSize, &prg->StaticData[0], staticSize);
  bool res = true;





  {







    res = false;

  }
  UInt32 newBlockPos = GetFixedGlobalValue32(NGlobalOffset::kBlockPos) & kSpaceMask;
  UInt32 newBlockSize = GetFixedGlobalValue32(NGlobalOffset::kBlockSize) & kSpaceMask;
  if (newBlockPos + newBlockSize >= kSpaceSize)
    newBlockPos = newBlockSize = 0;
  outBlockRef.Offset = newBlockPos;
  outBlockRef.Size = newBlockSize;
  outGlobalData.Clear();
  UInt32 dataSize = GetFixedGlobalValue32(NGlobalOffset::kGlobalMemOutSize);
  dataSize = MyMin(dataSize, kGlobalSize - kFixedGlobalSize);
  if (dataSize != 0) {
    dataSize += kFixedGlobalSize;
    outGlobalData.ClearAndSetSize(dataSize);
    memcpy(&outGlobalData[0], Mem + kGlobalOffset, dataSize); }
  return res; }
bool CProgram::PrepareProgram(const Byte *code, UInt32 codeSize) {
  IsSupported = false;






  bool isOK = false;
  Byte xorSum = 0;
  for (UInt32 i = 0; i < codeSize; i++)
    xorSum ^= code[i];
  if (xorSum == 0 && codeSize != 0) {
    IsSupported = true;
    isOK = true;
    IsSupported = false;

  }




  return isOK; }
void CVm::SetMemory(UInt32 pos, const Byte *data, UInt32 dataSize) {
  if (pos < kSpaceSize && data != Mem + pos)
    memmove(Mem + pos, data, MyMin(dataSize, kSpaceSize - pos)); }
}}}
