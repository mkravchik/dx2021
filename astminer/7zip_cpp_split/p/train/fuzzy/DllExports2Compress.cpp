#include "StdAfx.h"
#include "../../Common/MyInitGuid.h"
#include "../ICoder.h"
#include "../Common/RegisterCodec.h"

extern "C"
BOOL WINAPI DllMain(



  HINSTANCE

                 , DWORD , LPVOID )
{
  return TRUE; }
STDAPI CreateCoder(const GUID *clsid, const GUID *iid, void **outObject);
STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject) {
  return CreateCoder(clsid, iid, outObject); }
