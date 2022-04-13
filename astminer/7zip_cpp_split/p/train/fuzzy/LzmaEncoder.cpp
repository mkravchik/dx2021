#include "StdAfx.h"
#include "../../../C/Alloc.h"
#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"
#include "LzmaEncoder.h"

namespace NCompress {
namespace NLzma {
CEncoder::CEncoder() {
  _encoder = NULL;
  _encoder = LzmaEnc_Create(&g_AlignedAlloc);
  if (!_encoder)
    throw 1; }
CEncoder::~CEncoder() {
  if (_encoder)
    LzmaEnc_Destroy(_encoder, &g_AlignedAlloc, &g_BigAlloc); }
static inline wchar_t GetUpperChar(wchar_t c) {
  if (c >= 'a' && c <= 'z')
    c -= 0x20;
  return c; }
static int ParseMatchFinder(const wchar_t *s, int *btMode, int *numHashBytes) {
  wchar_t c = GetUpperChar(*s++);
  if (c == L'H') {
    if (GetUpperChar(*s++) != L'C')
      return 0;
    int numHashBytesLoc = (int)(*s++ - L'0');
    if (numHashBytesLoc < 4 || numHashBytesLoc > 4)
      return 0;
    if (*s != 0)
      return 0;
    *btMode = 0;
    *numHashBytes = numHashBytesLoc;
    return 1; }
  if (c != L'B')
    return 0;
  if (GetUpperChar(*s++) != L'T')
    return 0;
  int numHashBytesLoc = (int)(*s++ - L'0');
  if (numHashBytesLoc < 2 || numHashBytesLoc > 4)
    return 0;
  if (*s != 0)
    return 0;
  *btMode = 1;
  *numHashBytes = numHashBytesLoc;
  return 1; }

HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep) {
  if (propID == NCoderPropID::kMatchFinder) {
    if (prop.vt != VT_BSTR)
      return E_INVALIDARG;
    return ParseMatchFinder(prop.bstrVal, &ep.btMode, &ep.numHashBytes) ? S_OK : E_INVALIDARG; }
  if (propID > NCoderPropID::kReduceSize)
    return S_OK;
  if (propID == NCoderPropID::kReduceSize) {
    if (prop.vt == VT_UI8)
      ep.reduceSize = prop.uhVal.QuadPart;
    else
      return E_INVALIDARG;
    return S_OK; }
  if (prop.vt != VT_UI4)
    return E_INVALIDARG;
  UInt32 v = prop.ulVal;
  switch (propID) {
    case NCoderPropID::kDefaultProp: if (v > 31) return E_INVALIDARG; ep.dictSize = (UInt32)1 << (unsigned)v; break;
    case NCoderPropID::kLevel: ep.level = v; break;
    case NCoderPropID::kNumFastBytes: ep.fb = v; break;
    case NCoderPropID::kMatchFinderCycles: ep.mc = v; break;
    case NCoderPropID::kAlgorithm: ep.algo = v; break;
    case NCoderPropID::kDictionarySize: ep.dictSize = v; break;
    case NCoderPropID::kPosStateBits: ep.pb = v; break;
    case NCoderPropID::kLitPosBits: ep.lp = v; break;
    case NCoderPropID::kLitContextBits: ep.lc = v; break;
    case NCoderPropID::kNumThreads: ep.numThreads = v; break;
    default: return E_INVALIDARG; }
  return S_OK; }
STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps) {
  CLzmaEncProps props;
  LzmaEncProps_Init(&props);
  for (UInt32 i = 0; i < numProps; i++) {
    const PROPVARIANT &prop = coderProps[i];
    PROPID propID = propIDs[i];
    switch (propID) {
      case NCoderPropID::kEndMarker:
        if (prop.vt != VT_BOOL) return E_INVALIDARG; props.writeEndMark = (prop.boolVal != VARIANT_FALSE); break;
      default:
        RINOK(SetLzmaProp(propID, prop, props)); } }
  return SResToHRESULT(LzmaEnc_SetProps(_encoder, &props)); }
STDMETHODIMP CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps) {
  for (UInt32 i = 0; i < numProps; i++) {
    const PROPVARIANT &prop = coderProps[i];
    PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kExpectedDataSize)
      if (prop.vt == VT_UI8)
        LzmaEnc_SetDataSize(_encoder, prop.uhVal.QuadPart); }
  return S_OK; }
STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream) {
  Byte props[LZMA_PROPS_SIZE];
  size_t size = LZMA_PROPS_SIZE;
  RINOK(LzmaEnc_WriteProperties(_encoder, props, &size));
  return WriteStream(outStream, props, size); }


STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * , const UInt64 * , ICompressProgressInfo *progress)
{
  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;
  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);
  SRes res = LzmaEnc_Encode(_encoder, &outWrap.vt, &inWrap.vt,
      progress ? &progressWrap.vt : NULL, &g_AlignedAlloc, &g_BigAlloc);
  _inputProcessed = inWrap.Processed;
  if (inWrap.Res != S_OK ) return inWrap.Res;
  if (outWrap.Res != S_OK ) return outWrap.Res;
  if (progressWrap.Res != S_OK ) return progressWrap.Res;
  return SResToHRESULT(res); }
}}
