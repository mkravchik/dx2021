#include "StdAfx.h"
#include "../../../C/Alloc.h"
#include "../Common/StreamUtils.h"
#include "Lzma2Decoder.h"



namespace NCompress {
namespace NLzma2 {
CDecoder::CDecoder():
      _dec(NULL)
    , _inProcessed(0)
    , _prop(0xFF)
    , _finishMode(false)
    , _inBufSize(1 << 20)
    , _outStep(1 << 20)

    , _tryMt(1)
    , _numThreads(1)
    , _memUsage((UInt64)(sizeof(size_t)) << 28)

{}
CDecoder::~CDecoder() {
  if (_dec)
    Lzma2DecMt_Destroy(_dec); }
STDMETHODIMP CDecoder::SetInBufSize(UInt32 , UInt32 size) { _inBufSize = size; return S_OK; }
STDMETHODIMP CDecoder::SetOutBufSize(UInt32 , UInt32 size) { _outStep = size; return S_OK; }
STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size) {
  if (size != 1)
    return E_NOTIMPL;
  if (prop[0] > 40)
    return E_NOTIMPL;
  _prop = prop[0];
  return S_OK; }
STDMETHODIMP CDecoder::SetFinishMode(UInt32 finishMode) {
  _finishMode = (finishMode != 0);
  return S_OK; }

static UInt64 Get_ExpectedBlockSize_From_Dict(UInt32 dictSize) {
  const UInt32 kMinSize = (UInt32)1 << 20;
  const UInt32 kMaxSize = (UInt32)1 << 28;
  UInt64 blockSize = (UInt64)dictSize << 2;
  if (blockSize < kMinSize) blockSize = kMinSize;
  if (blockSize > kMaxSize) blockSize = kMaxSize;
  if (blockSize < dictSize) blockSize = dictSize;
  blockSize += (kMinSize - 1);
  blockSize &= ~(UInt64)(kMinSize - 1);
  return blockSize; }






STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress) {
  _inProcessed = 0;
  if (!_dec) {
    _dec = Lzma2DecMt_Create(

      &g_Alloc,
      &g_MidAlloc);
    if (!_dec)
      return E_OUTOFMEMORY; }
  CLzma2DecMtProps props;
  Lzma2DecMtProps_Init(&props);
  props.inBufSize_ST = _inBufSize;
  props.outStep_ST = _outStep;

  {
    props.numThreads = 1;
    UInt32 numThreads = _numThreads;
    if (_tryMt && numThreads >= 1) {
      UInt64 useLimit = _memUsage;
      UInt32 dictSize = ((_prop) == 40 ? 0xFFFFFFFF : (((UInt32)2 | ((_prop) & 1)) << ((_prop) / 2 + 11)));
      UInt64 expectedBlockSize64 = Get_ExpectedBlockSize_From_Dict(dictSize);
      size_t expectedBlockSize = (size_t)expectedBlockSize64;
      size_t inBlockMax = expectedBlockSize + expectedBlockSize / 16;
      if (expectedBlockSize == expectedBlockSize64 && inBlockMax >= expectedBlockSize) {
        props.outBlockMax = expectedBlockSize;
        props.inBlockMax = inBlockMax;
        const size_t kOverheadSize = props.inBufSize_MT + (1 << 16);
        UInt64 okThreads = useLimit / (props.outBlockMax + props.inBlockMax + kOverheadSize);
        if (numThreads > okThreads)
          numThreads = (UInt32)okThreads;
        if (numThreads == 0)
          numThreads = 1;
        props.numThreads = numThreads; } } }

  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;
  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);
  SRes res;
  UInt64 inProcessed = 0;
  int isMT = False;

  isMT = _tryMt;


  res = Lzma2DecMt_Decode(_dec, _prop, &props,
      &outWrap.vt, outSize, _finishMode,
      &inWrap.vt,
      &inProcessed,
      &isMT,
      progress ? &progressWrap.vt : NULL);






  if (props.numThreads > 1)
    _tryMt = isMT;

  _inProcessed = inProcessed;
  if (progressWrap.Res != S_OK ) return progressWrap.Res;
  if (outWrap.Res != S_OK ) return outWrap.Res;
  if (inWrap.Res != S_OK && res == SZ_ERROR_READ) return inWrap.Res;
  if (res == SZ_OK && _finishMode) {
    if (inSize && *inSize != inProcessed)
      res = SZ_ERROR_DATA;
    if (outSize && *outSize != outWrap.Processed)
      res = SZ_ERROR_DATA; }
  return SResToHRESULT(res); }
STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value) {
  *value = _inProcessed;
  return S_OK; }

STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 numThreads) {
  _numThreads = numThreads;
  return S_OK; }
STDMETHODIMP CDecoder::SetMemLimit(UInt64 memUsage) {
  _memUsage = memUsage;
  return S_OK; }


STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize) {
  CLzma2DecMtProps props;
  Lzma2DecMtProps_Init(&props);
  props.inBufSize_ST = _inBufSize;
  props.outStep_ST = _outStep;
  _inProcessed = 0;
  if (!_dec) {
    _dec = Lzma2DecMt_Create(&g_AlignedAlloc, &g_MidAlloc);
    if (!_dec)
      return E_OUTOFMEMORY; }
  _inWrap.Init(_inStream);
  SRes res = Lzma2DecMt_Init(_dec, _prop, &props, outSize, _finishMode, &_inWrap.vt);
  if (res != SZ_OK)
    return SResToHRESULT(res);
  return S_OK; }
STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream) { _inStream = inStream; return S_OK; }
STDMETHODIMP CDecoder::ReleaseInStream() { _inStream.Release(); return S_OK; }
STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize) {
  if (processedSize)
    *processedSize = 0;
  size_t size2 = size;
  UInt64 inProcessed = 0;
  SRes res = Lzma2DecMt_Read(_dec, (Byte *)data, &size2, &inProcessed);
  _inProcessed += inProcessed;
  if (processedSize)
    *processedSize = (UInt32)size2;
  if (res != SZ_OK)
    return SResToHRESULT(res);
  return S_OK; }

}}
