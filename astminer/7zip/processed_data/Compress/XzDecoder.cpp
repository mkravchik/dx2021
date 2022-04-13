#include "StdAfx.h"
#include "../../../C/Alloc.h"
#include "../Common/CWrappers.h"
#include "XzDecoder.h"

namespace NCompress {
namespace NXz {




static HRESULT SResToHRESULT_Code(SRes res) throw() {
  if (res < 0)
    return res;
  switch (res) {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL; }
  return S_FALSE; }
HRESULT CDecoder::Decode(ISequentialInStream *seqInStream, ISequentialOutStream *outStream,
    const UInt64 *outSizeLimit, bool finishStream, ICompressProgressInfo *progress) {
  MainDecodeSRes = S_OK;
  MainDecodeSRes_wasUsed = false;
  XzStatInfo_Clear(&Stat);
  if (!xz) {
    xz = XzDecMt_Create(&g_Alloc, &g_MidAlloc);
    if (!xz)
      return E_OUTOFMEMORY; }
  CXzDecMtProps props;
  XzDecMtProps_Init(&props);
  int isMT = False;

  {
    props.numThreads = 1;
    UInt32 numThreads = _numThreads;
    if (_tryMt && numThreads > 1) {
      size_t memUsage = (size_t)_memUsage;
      if (memUsage != _memUsage)
        memUsage = (size_t)0 - 1;
      props.memUseMax = memUsage;
      isMT = (numThreads > 1); }
    props.numThreads = numThreads; }

  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;
  inWrap.Init(seqInStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);
  SRes res = XzDecMt_Decode(xz,
      &props,
      outSizeLimit, finishStream,
      &outWrap.vt,
      &inWrap.vt,
      &Stat,
      &isMT,
      progress ? &progressWrap.vt : NULL);
  MainDecodeSRes = res;



  if (outWrap.Res != S_OK ) return outWrap.Res;
  if (progressWrap.Res != S_OK ) return progressWrap.Res;
  if (inWrap.Res != S_OK && res == SZ_ERROR_READ) return inWrap.Res;

  MainDecodeSRes_wasUsed = true;
  if (res == SZ_OK && finishStream) {




    if (outSizeLimit && *outSizeLimit != outWrap.Processed)
      res = SZ_ERROR_DATA; }
  return SResToHRESULT_Code(res); }
HRESULT CComDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * , const UInt64 *outSize, ICompressProgressInfo *progress)
{
  return Decode(inStream, outStream, outSize, _finishStream, progress); }
STDMETHODIMP CComDecoder::SetFinishMode(UInt32 finishMode) {
  _finishStream = (finishMode != 0);
  return S_OK; }
STDMETHODIMP CComDecoder::GetInStreamProcessedSize(UInt64 *value) {
  *value = Stat.InSize;
  return S_OK; }

STDMETHODIMP CComDecoder::SetNumberOfThreads(UInt32 numThreads) {
  _numThreads = numThreads;
  return S_OK; }
STDMETHODIMP CComDecoder::SetMemLimit(UInt64 memUsage) {
  _memUsage = memUsage;
  return S_OK; }

}}
