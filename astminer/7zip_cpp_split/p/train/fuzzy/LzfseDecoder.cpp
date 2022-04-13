#include "StdAfx.h"
#include <stdio.h>
#include "../../../C/CpuArch.h"
#include "LzfseDecoder.h"
namespace NCompress {
namespace NLzfse {
static const Byte kSignature_LZFSE_V1 = 0x31;
static const Byte kSignature_LZFSE_V2 = 0x32;
HRESULT CDecoder::GetUInt32(UInt32 &val) {
  Byte b[4];
  for (unsigned i = 0; i < 4; i++)
    if (!m_InStream.ReadByte(b[i]))
      return S_FALSE;
  val = GetUi32(b);
  return S_OK; }
HRESULT CDecoder::DecodeUncompressed(UInt32 unpackSize) {
  ;
  const unsigned kBufSize = 1 << 8;
  Byte buf[kBufSize];
  for (;;) {
    if (unpackSize == 0)
      return S_OK;
    UInt32 cur = unpackSize;
    if (cur > kBufSize)
      cur = kBufSize;
    UInt32 cur2 = (UInt32)m_InStream.ReadBytes(buf, cur);
    m_OutWindowStream.PutBytes(buf, cur2);
    if (cur != cur2)
      return S_FALSE; } }
HRESULT CDecoder::DecodeLzvn(UInt32 unpackSize) {
  UInt32 packSize;
  RINOK(GetUInt32(packSize));
  ;
  UInt32 D = 0;
  for (;;) {
    if (packSize == 0)
      return S_FALSE;
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    packSize--;
    UInt32 M;
    UInt32 L;
    if (b >= 0xE0) {






      M = b & 0xF;
      if (M == 0) {
        if (packSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        M = (UInt32)b1 + 16; }
      L = 0;
      if ((b & 0x10) == 0) {

        L = M;
        M = 0; } }

    else if ((b & 0xF0) == 0x70)
      return S_FALSE;
    else if ((b & 0xF0) == 0xD0)
      return S_FALSE;
    else {
      if ((b & 0xE0) == 0xA0) {

        if (packSize < 2)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        Byte b2;
        if (!m_InStream.ReadByte(b2))
          return S_FALSE;
        packSize--;
        L = (((UInt32)b >> 3) & 3);
        M = (((UInt32)b & 7) << 2) + (b1 & 3);
        D = ((UInt32)b1 >> 2) + ((UInt32)b2 << 6); }
      else {
        L = (UInt32)b >> 6;
        M = ((UInt32)b >> 3) & 7;
        if ((b & 0x7) == 6) {

          if (L == 0) {

            if (M == 0)
              break;
            if (M <= 2)
              continue;
            return S_FALSE;
          } }
        else {
          if (packSize == 0)
            return S_FALSE;
          Byte b1;
          if (!m_InStream.ReadByte(b1))
            return S_FALSE;
          packSize--;


          D = ((UInt32)b & 7);
          if (D == 7) {
            if (packSize == 0)
              return S_FALSE;
            Byte b2;
            if (!m_InStream.ReadByte(b2))
              return S_FALSE;
            packSize--;
            D = b2; }
          D = (D << 8) + b1; } }
      M += 3; } {
      for (unsigned i = 0; i < L; i++) {
        if (packSize == 0 || unpackSize == 0)
          return S_FALSE;
        Byte b1;
        if (!m_InStream.ReadByte(b1))
          return S_FALSE;
        packSize--;
        m_OutWindowStream.PutByte(b1);
        unpackSize--; } }
    if (M != 0) {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE; } }
  if (unpackSize != 0)
    return S_FALSE;

  if (packSize != 7)
    return S_FALSE;
  do {
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    packSize--;
    if (b != 0)
      return S_FALSE; }
  while (packSize != 0);
  return S_OK; }
typedef UInt32 CFseState;
static UInt32 SumFreqs(const UInt16 *freqs, unsigned num) {
  UInt32 sum = 0;
  for (unsigned i = 0; i < num; i++)
    sum += (UInt32)freqs[i];
  return sum; }
static MY_FORCE_INLINE unsigned CountZeroBits(UInt32 val, UInt32 mask) {
  for (unsigned i = 0;;) {
    if (val & mask)
      return i;
    i++;
    mask >>= 1; } }
static MY_FORCE_INLINE void InitLitTable(const UInt16 *freqs, UInt32 *table) {
  for (unsigned i = 0; i < 256; i++) {
    unsigned f = freqs[i];
    if (f == 0)
      continue;





    unsigned k = CountZeroBits(f, (1 << 10));
    unsigned j0 = (((unsigned)(1 << 10) * 2) >> k) - f;







    UInt32 e = ((UInt32)i << 8) + k;
    k += 16;
    UInt32 d = e + ((UInt32)f << k) - ((UInt32)(1 << 10) << 16);
    UInt32 step = (UInt32)1 << k;
    unsigned j = 0;
    do {
      *table++ = d;
      d += step; }
    while (++j < j0);
    e--;
    step >>= 1;
    for (j = j0; j < f; j++) {
      *table++ = e;
      e += step; } } }
typedef struct {
  Byte totalBits;
  Byte extraBits;
  UInt16 delta;
  UInt32 vbase;
} CExtraEntry;
static void InitExtraDecoderTable(unsigned numStates,
    unsigned numSymbols,
    const UInt16 *freqs,
    const Byte *vbits,
    CExtraEntry *table) {
  UInt32 vbase = 0;
  for (unsigned i = 0; i < numSymbols; i++) {
    unsigned f = freqs[i];
    unsigned extraBits = vbits[i];
    if (f != 0) {
      unsigned k = CountZeroBits(f, numStates);
      unsigned j0 = ((2 * numStates) >> k) - f;
      unsigned j = 0;
      do {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(((f + j) << k) - numStates);
        e->vbase = vbase; }
      while (++j < j0);
      f -= j0;
      k--;
      for (j = 0; j < f; j++) {
        CExtraEntry *e = table++;
        e->totalBits = (Byte)(k + extraBits);
        e->extraBits = (Byte)extraBits;
        e->delta = (UInt16)(j << k);
        e->vbase = vbase; } }
    vbase += ((UInt32)1 << extraBits); } }
static const Byte k_L_extra[20] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 5, 8
};
static const Byte k_M_extra[20] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 5, 8, 11
};
static const Byte k_D_extra[64] = {
   0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
   4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
   8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11,
  12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15
};

typedef struct {
  UInt32 accum;
  unsigned numBits;
} CBitStream;
static MY_FORCE_INLINE int FseInStream_Init(CBitStream *s,
    int n,
    const Byte **pbuf) {
  *pbuf -= 4;
  s->accum = GetUi32(*pbuf);
  if (n) {
    s->numBits = n + 32;
    if ((s->accum >> s->numBits) != 0)
      return -1;
  }
  else {
    *pbuf += 1;
    s->accum >>= 8;
    s->numBits = 24; }
  return 0;
}
static MY_FORCE_INLINE UInt32 BitStream_Pull(CBitStream *s, unsigned numBits) {
  s->numBits -= numBits;
  UInt32 v = s->accum >> s->numBits;
  s->accum = ((s->accum) & (((UInt32)1 << (s->numBits)) - 1));
  return v; }




static MY_FORCE_INLINE UInt32 FseDecodeExtra(CFseState *pstate,
    const CExtraEntry *table,
    CBitStream *s) {
  const CExtraEntry *e = &table[*pstate];
  UInt32 v = BitStream_Pull(s, e->totalBits);
  unsigned extraBits = e->extraBits;
  *pstate = (CFseState)(e->delta + (v >> extraBits));
  return e->vbase + ((v) & (((UInt32)1 << (extraBits)) - 1)); }






HRESULT CDecoder::DecodeLzfse(UInt32 unpackSize, Byte version) {
  ;
  UInt32 numLiterals;
  UInt32 litPayloadSize;
  Int32 literal_bits;
  UInt32 lit_state_0;
  UInt32 lit_state_1;
  UInt32 lit_state_2;
  UInt32 lit_state_3;
  UInt32 numMatches;
  UInt32 lmdPayloadSize;
  Int32 lmd_bits;
  CFseState l_state;
  CFseState m_state;
  CFseState d_state;
  UInt16 freqs[( 20 + 20 + 64 + 256)];
  if (version == kSignature_LZFSE_V1) {
    return E_NOTIMPL;
  }
  else {
    UInt32 headerSize; {
      const unsigned kPreHeaderSize = 4 * 2;
      const unsigned kHeaderSize = 8 * 3;
      Byte temp[kHeaderSize];
      if (m_InStream.ReadBytes(temp, kHeaderSize) != kHeaderSize)
        return S_FALSE;
      UInt64 v;
      v = GetUi64(temp);
      numLiterals = (UInt32) ((v >> (0)) & ((1 << (20)) - 1));;
      litPayloadSize = (UInt32) ((v >> (20)) & ((1 << (20)) - 1));;
      numMatches = (UInt32) ((v >> (40)) & ((1 << (20)) - 1));;
      literal_bits = (UInt32) ((v >> (60)) & ((1 << (3 + 1)) - 1));;
      literal_bits -= 7;
      if (literal_bits > 0)
        return S_FALSE;

      v = GetUi64(temp + 8);
      lit_state_0 = (UInt32) ((v >> (0)) & ((1 << (10)) - 1));;
      lit_state_1 = (UInt32) ((v >> (10)) & ((1 << (10)) - 1));;
      lit_state_2 = (UInt32) ((v >> (20)) & ((1 << (10)) - 1));;
      lit_state_3 = (UInt32) ((v >> (30)) & ((1 << (10)) - 1));;
      lmdPayloadSize = (UInt32) ((v >> (40)) & ((1 << (20)) - 1));;
      lmd_bits = (UInt32) ((v >> (60)) & ((1 << (3 + 1)) - 1));;
      lmd_bits -= 7;
      if (lmd_bits > 0)
        return S_FALSE;

      UInt32 v32 = GetUi32(temp + 20);




      l_state = (CFseState)((v32 >> (0)) & ((1 << (10)) - 1));;
      m_state = (CFseState)((v32 >> (10)) & ((1 << (10)) - 1));;
      d_state = (CFseState)((v32 >> (20)) & ((1 << (10 + 2)) - 1));;

      headerSize = GetUi32(temp + 16);
      if (headerSize <= kPreHeaderSize + kHeaderSize)
        return S_FALSE;
      headerSize -= kPreHeaderSize + kHeaderSize; }



    {
      static const Byte numBitsTable[32] = {
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14,
        2, 3, 2, 5, 2, 3, 2, 8, 2, 3, 2, 5, 2, 3, 2, 14
      };
      static const Byte valueTable[32] = {
        0, 2, 1, 4, 0, 3, 1, 8, 0, 2, 1, 5, 0, 3, 1, 24,
        0, 2, 1, 6, 0, 3, 1, 8, 0, 2, 1, 7, 0, 3, 1, 24
      };
      UInt32 accum = 0;
      unsigned numBits = 0;
      for (unsigned i = 0; i < ( 20 + 20 + 64 + 256); i++) {
        while (numBits <= 14 && headerSize != 0) {
          Byte b;
          if (!m_InStream.ReadByte(b))
            return S_FALSE;
          accum |= (UInt32)b << numBits;
          numBits += 8;
          headerSize--; }
        unsigned b = (unsigned)accum & 31;
        unsigned n = numBitsTable[b];
        if (numBits < n)
          return S_FALSE;
        numBits -= n;
        UInt32 f = valueTable[b];
        if (n >= 8)
          f += ((accum >> 4) & (0x3ff >> (14 - n)));
        accum >>= n;
        freqs[i] = (UInt16)f; }
      if (numBits >= 8 || headerSize != 0)
        return S_FALSE; } }
  ;
  if (numLiterals > (4 * 10000)
      || (numLiterals & 3) != 0
      || numMatches > 10000
      || lit_state_0 >= (1 << 10)
      || lit_state_1 >= (1 << 10)
      || lit_state_2 >= (1 << 10)
      || lit_state_3 >= (1 << 10)
      || l_state >= (1 << 6)
      || m_state >= (1 << 6)
      || d_state >= (1 << 8))
    return S_FALSE;

  if ( SumFreqs((freqs), 20) != (1 << 6)
      || SumFreqs(((freqs) + 20), 20) != (1 << 6)
      || SumFreqs((((freqs) + 20) + 20), 64) != (1 << 8)
      || SumFreqs(((((freqs) + 20) + 20) + 64), 256) != (1 << 10))
    return S_FALSE;
  const unsigned kPad = 16;

  {
    _literals.AllocAtLeast((4 * 10000) + 16);
    _buffer.AllocAtLeast(kPad + litPayloadSize);
    memset(_buffer, 0, kPad);
    if (m_InStream.ReadBytes(_buffer + kPad, litPayloadSize) != litPayloadSize)
      return S_FALSE;
    UInt32 lit_decoder[(1 << 10)];
    InitLitTable(((((freqs) + 20) + 20) + 64), lit_decoder);
    const Byte *buf_start = _buffer + kPad;
    const Byte *buf_check = buf_start - 4;
    const Byte *buf = buf_start + litPayloadSize;
    CBitStream in;
    if (FseInStream_Init(&in, literal_bits, &buf) != 0)
      return S_FALSE;
    Byte *lit = _literals;
    const Byte *lit_limit = lit + numLiterals;
    for (; lit < lit_limit; lit += 4) {
      { unsigned nbits = (31 - in.numBits) & -8; if (nbits) { buf -= (nbits >> 3); if (buf < buf_check) return S_FALSE; UInt32 v = GetUi32(buf); in.accum = (in.accum << nbits) | ((v) & (((UInt32)1 << (nbits)) - 1)); in.numBits += nbits; }}
      { UInt32 e = lit_decoder[lit_state_0]; lit_state_0 = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); lit[0] = (Byte)(e >> 8); };
      { UInt32 e = lit_decoder[lit_state_1]; lit_state_1 = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); lit[1] = (Byte)(e >> 8); };
      { unsigned nbits = (31 - in.numBits) & -8; if (nbits) { buf -= (nbits >> 3); if (buf < buf_check) return S_FALSE; UInt32 v = GetUi32(buf); in.accum = (in.accum << nbits) | ((v) & (((UInt32)1 << (nbits)) - 1)); in.numBits += nbits; }}
      { UInt32 e = lit_decoder[lit_state_2]; lit_state_2 = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); lit[2] = (Byte)(e >> 8); };
      { UInt32 e = lit_decoder[lit_state_3]; lit_state_3 = (CFseState)((e >> 16) + BitStream_Pull(&in, e & 0xff)); lit[3] = (Byte)(e >> 8); }; }
    if ((buf_start - buf) * 8 != (int)in.numBits)
      return S_FALSE; }

  _buffer.AllocAtLeast(kPad + lmdPayloadSize);
  memset(_buffer, 0, kPad);
  if (m_InStream.ReadBytes(_buffer + kPad, lmdPayloadSize) != lmdPayloadSize)
    return S_FALSE;
  CExtraEntry l_decoder[(1 << 6)];
  CExtraEntry m_decoder[(1 << 6)];
  CExtraEntry d_decoder[(1 << 8)];
  InitExtraDecoderTable((1 << 6), 20, (freqs), k_L_extra, l_decoder);
  InitExtraDecoderTable((1 << 6), 20, ((freqs) + 20), k_M_extra, m_decoder);
  InitExtraDecoderTable((1 << 8), 64, (((freqs) + 20) + 20), k_D_extra, d_decoder);
  const Byte *buf_start = _buffer + kPad;
  const Byte *buf_check = buf_start - 4;
  const Byte *buf = buf_start + lmdPayloadSize;
  CBitStream in;
  if (FseInStream_Init(&in, lmd_bits, &buf))
    return S_FALSE;
  const Byte *lit = _literals;
  const Byte *lit_limit = lit + numLiterals;
  UInt32 D = 0;
  for (;;) {
    if (numMatches == 0)
      break;
    numMatches--;
    { unsigned nbits = (31 - in.numBits) & -8; if (nbits) { buf -= (nbits >> 3); if (buf < buf_check) return S_FALSE; UInt32 v = GetUi32(buf); in.accum = (in.accum << nbits) | ((v) & (((UInt32)1 << (nbits)) - 1)); in.numBits += nbits; }}
    unsigned L = (unsigned)FseDecodeExtra(&l_state, l_decoder, &in);
    { unsigned nbits = (31 - in.numBits) & -8; if (nbits) { buf -= (nbits >> 3); if (buf < buf_check) return S_FALSE; UInt32 v = GetUi32(buf); in.accum = (in.accum << nbits) | ((v) & (((UInt32)1 << (nbits)) - 1)); in.numBits += nbits; }}
    unsigned M = (unsigned)FseDecodeExtra(&m_state, m_decoder, &in);
    { unsigned nbits = (31 - in.numBits) & -8; if (nbits) { buf -= (nbits >> 3); if (buf < buf_check) return S_FALSE; UInt32 v = GetUi32(buf); in.accum = (in.accum << nbits) | ((v) & (((UInt32)1 << (nbits)) - 1)); in.numBits += nbits; }} {
      UInt32 new_D = FseDecodeExtra(&d_state, d_decoder, &in);
      if (new_D)
        D = new_D; }
    if (L != 0) {
      if (L > (size_t)(lit_limit - lit))
        return S_FALSE;
      unsigned cur = L;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      m_OutWindowStream.PutBytes(lit, cur);
      unpackSize -= cur;
      lit += cur;
      if (cur != L)
        return S_FALSE; }
    if (M != 0) {
      if (unpackSize == 0 || D == 0)
        return S_FALSE;
      unsigned cur = M;
      if (cur > unpackSize)
        cur = (unsigned)unpackSize;
      if (!m_OutWindowStream.CopyBlock(D - 1, cur))
        return S_FALSE;
      unpackSize -= cur;
      if (cur != M)
        return S_FALSE; } }
  if (unpackSize != 0)
    return S_FALSE;


  if ((buf - buf_start) * 8 + in.numBits != 64)
    return S_FALSE;
  if (GetUi64(buf_start) != 0)
    return S_FALSE;
  return S_OK; }
STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress) {
  ;
  const UInt32 kLzfseDictSize = 1 << 18;
  if (!m_OutWindowStream.Create(kLzfseDictSize))
    return E_OUTOFMEMORY;
  if (!m_InStream.Create(1 << 18))
    return E_OUTOFMEMORY;
  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InStream.SetStream(inStream);
  m_InStream.Init();
  CCoderReleaser coderReleaser(this);
  UInt64 prevOut = 0;
  UInt64 prevIn = 0;
  for (;;) {
    const UInt64 pos = m_OutWindowStream.GetProcessedSize();
    const UInt64 packPos = m_InStream.GetProcessedSize();
    if (progress && ((pos - prevOut) >= (1 << 22) || (packPos - prevIn) >= (1 << 22))) {
      RINOK(progress->SetRatioInfo(&packPos, &pos));
      prevIn = packPos;
      prevOut = pos; }
    const UInt64 rem = *outSize - pos;
    UInt32 v;
    RINOK(GetUInt32(v))
    if ((v & 0xFFFFFF) != 0x787662)
      return S_FALSE;
    v >>= 24;
    if (v == 0x24)
      break;
    UInt32 unpackSize;
    RINOK(GetUInt32(unpackSize));
    UInt32 cur = unpackSize;
    if (cur > rem)
      cur = (UInt32)rem;
    unpackSize -= cur;
    HRESULT res;
    if (v == kSignature_LZFSE_V1 || v == kSignature_LZFSE_V2)
      res = DecodeLzfse(cur, (Byte)v);
    else if (v == 0x6E)
      res = DecodeLzvn(cur);
    else if (v == 0x2D)
      res = DecodeUncompressed(cur);
    else
      return E_NOTIMPL;
    if (res != S_OK)
      return res;
    if (unpackSize != 0)
      return S_FALSE; }
  coderReleaser.NeedFlush = false;
  HRESULT res = m_OutWindowStream.Flush();
  if (res == S_OK)
    if (*inSize != m_InStream.GetProcessedSize()
        || *outSize != m_OutWindowStream.GetProcessedSize())
      res = S_FALSE;
  return res; }
STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress) {
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return E_OUTOFMEMORY; }

}
}}
