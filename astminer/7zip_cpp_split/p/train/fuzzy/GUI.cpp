#include "StdAfx.h"
#include "../../../../C/DllSecur.h"
#include "../../../Common/MyWindows.h"
#include <shlwapi.h>
#include "../../../Common/MyInitGuid.h"
#include "../../../Common/CommandLineParser.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StringConvert.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/NtCheck.h"
#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"
#include "../FileManager/StringUtils.h"
#include "../FileManager/MyWindowsNew.h"
#include "BenchmarkDialog.h"
#include "ExtractGUI.h"
#include "HashGUI.h"
#include "UpdateGUI.h"
#include "ExtractRes.h"



using namespace NWindows;
HINSTANCE g_hInstance;

DWORD g_ComCtl32Version;
static DWORD GetDllVersion(LPCTSTR dllName) {
  DWORD dwVersion = 0;
  HINSTANCE hinstDll = LoadLibrary(dllName);
  if (hinstDll) {
    DLLGETVERSIONPROC pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
    if (pDllGetVersion) {
      DLLVERSIONINFO dvi;
      ZeroMemory(&dvi, sizeof(dvi));
      dvi.cbSize = sizeof(dvi);
      HRESULT hr = (*pDllGetVersion)(&dvi);
      if (SUCCEEDED(hr))
        dwVersion = MAKELONG(dvi.dwMinorVersion, dvi.dwMajorVersion); }
    FreeLibrary(hinstDll); }
  return dwVersion; }

bool g_LVN_ITEMACTIVATE_Support = true;
static void ErrorMessage(LPCWSTR message) {
  MessageBoxW(NULL, message, L"7-Zip", MB_ICONERROR | MB_OK);
}
static void ErrorMessage(const char *s) {
  ErrorMessage(GetUnicodeString(s)); }
static void ErrorLangMessage(UINT resourceID) {
  ErrorMessage(LangString(resourceID)); }
static const char * const kNoFormats = "7-Zip cannot find the code that works with archives.";
static int ShowMemErrorMessage() {
  ErrorLangMessage(IDS_MEM_ERROR);
  return NExitCode::kMemoryError; }
static int ShowSysErrorMessage(DWORD errorCode) {
  if (errorCode == E_OUTOFMEMORY)
    return ShowMemErrorMessage();
  ErrorMessage(HResultToMessage(errorCode));
  return NExitCode::kFatalError; }
static void ThrowException_if_Error(HRESULT res) {
  if (res != S_OK)
    throw CSystemException(res); }
static int Main2() {
  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);

  if (commandStrings.Size() > 0)
    commandStrings.Delete(0);

  if (commandStrings.Size() == 0) {
    MessageBoxW(0, L"Specify command", L"7-Zip", 0);
    return 0; }
  CArcCmdLineOptions options;
  CArcCmdLineParser parser;
  parser.Parse1(commandStrings, options);
  parser.Parse2(options);
  CREATE_CODECS_OBJECT
  codecs->CaseSensitiveChange = options.CaseSensitiveChange;
  codecs->CaseSensitive = options.CaseSensitive;
  ThrowException_if_Error(codecs->Load());
  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();
  if (codecs->Formats.Size() == 0 &&
        (isExtractGroupCommand
        || options.Command.IsFromUpdateGroup())) {






    throw kNoFormats; }
  CObjectVector<COpenType> formatIndices;
  if (!ParseOpenTypes(*codecs, options.ArcType, formatIndices)) {
    ErrorLangMessage(IDS_UNSUPPORTED_ARCHIVE_TYPE);
    return NExitCode::kFatalError; }
  CIntVector excludedFormatIndices;
  FOR_VECTOR (k, options.ExcludedArcTypes) {
    CIntVector tempIndices;
    if (!codecs->FindFormatForArchiveType(options.ExcludedArcTypes[k], tempIndices)
        || tempIndices.Size() != 1) {
      ErrorLangMessage(IDS_UNSUPPORTED_ARCHIVE_TYPE);
      return NExitCode::kFatalError; }
    excludedFormatIndices.AddToUniqueSorted(tempIndices[0]);

  }






  if (options.Command.CommandType == NCommandType::kBenchmark) {
    HRESULT res = Benchmark(EXTERNAL_CODECS_VARS_L options.Properties);





    ThrowException_if_Error(res); }
  else if (isExtractGroupCommand) {
    UStringVector ArchivePathsSorted;
    UStringVector ArchivePathsFullSorted;
    CExtractCallbackImp *ecs = new CExtractCallbackImp;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

    ecs->PasswordIsDefined = options.PasswordEnabled;
    ecs->Password = options.Password;

    ecs->Init();
    CExtractOptions eo;
    (CExtractOptionsBase &)eo = options.ExtractOptions;
    eo.StdInMode = options.StdInMode;
    eo.StdOutMode = options.StdOutMode;
    eo.YesToAll = options.YesToAll;
    eo.TestMode = options.Command.IsTestCommand();

    eo.Properties = options.Properties;

    bool messageWasDisplayed = false;

    CHashBundle hb;
    CHashBundle *hb_ptr = NULL;
    if (!options.HashMethods.IsEmpty()) {
      hb_ptr = &hb;
      ThrowException_if_Error(hb.SetMethods(EXTERNAL_CODECS_VARS_L options.HashMethods)); }

    {
      CDirItemsStat st;
      HRESULT hresultMain = EnumerateDirItemsAndSort(
          options.arcCensor,
          NWildcard::k_RelatPath,
          UString(),
          ArchivePathsSorted,
          ArchivePathsFullSorted,
          st,
          NULL
          );
      if (hresultMain != S_OK) {




        throw CSystemException(hresultMain); } }
    ecs->MultiArcMode = (ArchivePathsSorted.Size() > 1);
    HRESULT result = ExtractGUI(codecs,
          formatIndices, excludedFormatIndices,
          ArchivePathsSorted,
          ArchivePathsFullSorted,
          options.Censor.Pairs.Front().Head,
          eo,

          hb_ptr,

          options.ShowDialog, messageWasDisplayed, ecs);
    if (result != S_OK) {
      if (result != E_ABORT && messageWasDisplayed)
        return NExitCode::kFatalError;
      throw CSystemException(result); }
    if (!ecs->IsOK())
      return NExitCode::kFatalError; }
  else if (options.Command.IsFromUpdateGroup()) {

    bool passwordIsDefined = options.PasswordEnabled && !options.Password.IsEmpty();

    CUpdateCallbackGUI callback;


    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;


    callback.Init();
    if (!options.UpdateOptions.InitFormatIndex(codecs, formatIndices, options.ArchiveName) ||
        !options.UpdateOptions.SetArcPath(codecs, options.ArchiveName)) {
      ErrorLangMessage(IDS_UPDATE_NOT_SUPPORTED);
      return NExitCode::kFatalError; }
    bool messageWasDisplayed = false;
    HRESULT result = UpdateGUI(
        codecs, formatIndices,
        options.ArchiveName,
        options.Censor,
        options.UpdateOptions,
        options.ShowDialog,
        messageWasDisplayed,
        &callback);
    if (result != S_OK) {
      if (result != E_ABORT && messageWasDisplayed)
        return NExitCode::kFatalError;
      throw CSystemException(result); }
    if (callback.FailedFiles.Size() > 0) {
      if (!messageWasDisplayed)
        throw CSystemException(E_FAIL);
      return NExitCode::kWarning; } }
  else if (options.Command.CommandType == NCommandType::kHash) {
    bool messageWasDisplayed = false;
    HRESULT result = HashCalcGUI(EXTERNAL_CODECS_VARS_L
        options.Censor, options.HashOptions, messageWasDisplayed);
    if (result != S_OK) {
      if (result != E_ABORT && messageWasDisplayed)
        return NExitCode::kFatalError;
      throw CSystemException(result); }






  }
  else {
    throw "Unsupported command";
  }
  return 0; }

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE ,



  LPSTR

                 , int )
{
  g_hInstance = hInstance;



  InitCommonControls();

  g_ComCtl32Version = ::GetDllVersion(TEXT("comctl32.dll"));
  g_LVN_ITEMACTIVATE_Support = (g_ComCtl32Version >= MAKELONG(71, 4));



  OleInitialize(NULL);

  LoadLangOneTime();

  try {



    return Main2(); }
  catch(const CNewException &) {
    return ShowMemErrorMessage(); }
  catch(const CMessagePathException &e) {
    ErrorMessage(e);
    return NExitCode::kUserError; }
  catch(const CSystemException &systemError) {
    if (systemError.ErrorCode == E_ABORT)
      return NExitCode::kUserBreak;
    return ShowSysErrorMessage(systemError.ErrorCode); }
  catch(const UString &s) {
    ErrorMessage(s);
    return NExitCode::kFatalError; }
  catch(const AString &s) {
    ErrorMessage(s);
    return NExitCode::kFatalError; }
  catch(const wchar_t *s) {
    ErrorMessage(s);
    return NExitCode::kFatalError; }
  catch(const char *s) {
    ErrorMessage(s);
    return NExitCode::kFatalError; }
  catch(int v) {
    AString e ("Error: ");
    e.Add_UInt32(v);
    ErrorMessage(e);
    return NExitCode::kFatalError; }
  catch(...) {
    ErrorMessage("Unknown error");
    return NExitCode::kFatalError; } }
