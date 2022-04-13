#include "StdAfx.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileName.h"
#include "../../../Windows/Thread.h"
#include "../FileManager/ExtractCallback.h"
#include "../FileManager/FormatUtils.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/resourceGui.h"
#include "../FileManager/OverwriteDialogRes.h"
#include "../Common/ArchiveExtractCallback.h"
#include "../Common/PropIDUtils.h"
#include "../Explorer/MyMessages.h"
#include "resource2.h"
#include "ExtractRes.h"
#include "ExtractDialog.h"
#include "ExtractGUI.h"
#include "HashGUI.h"
#include "../FileManager/PropertyNameRes.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
static const wchar_t * const kIncorrectOutDir = L"Incorrect output directory path";

static void AddValuePair(UString &s, UINT resourceID, UInt64 value, bool addColon = true) {
  AddLangString(s, resourceID);
  if (addColon)
    s += ':';
  s.Add_Space();
  char sz[32];
  ConvertUInt64ToString(value, sz);
  s += sz;
  s.Add_LF(); }
static void AddSizePair(UString &s, UINT resourceID, UInt64 value) {
  AddLangString(s, resourceID);
  s += ": ";
  AddSizeValue(s, value);
  s.Add_LF(); }

class CThreadExtracting: public CProgressThreadVirt {
  HRESULT ProcessVirt();
public:
  CCodecs *codecs;
  CExtractCallbackImp *ExtractCallbackSpec;
  const CObjectVector<COpenType> *FormatIndices;
  const CIntVector *ExcludedFormatIndices;
  UStringVector *ArchivePaths;
  UStringVector *ArchivePathsFull;
  const NWildcard::CCensorNode *WildcardCensor;
  const CExtractOptions *Options;

  CHashBundle *HashBundle;
  virtual void ProcessWasFinished_GuiVirt();

  CMyComPtr<IExtractCallbackUI> ExtractCallback;
  UString Title;
  CPropNameValPairs Pairs;
};

void CThreadExtracting::ProcessWasFinished_GuiVirt() {
  if (HashBundle && !Pairs.IsEmpty())
    ShowHashResults(Pairs, *this); }

HRESULT CThreadExtracting::ProcessVirt() {
  CDecompressStat Stat;






  HRESULT res = Extract(codecs,
      *FormatIndices, *ExcludedFormatIndices,
      *ArchivePaths, *ArchivePathsFull,
      *WildcardCensor, *Options, ExtractCallbackSpec, ExtractCallback,

        HashBundle,

      FinalMessage.ErrorMessage.Message, Stat);

  if (res == S_OK && ExtractCallbackSpec->IsOK()) {
    if (HashBundle) {
      AddValuePair(Pairs, IDS_ARCHIVES_COLON, Stat.NumArchives);
      AddSizeValuePair(Pairs, IDS_PROP_PACKED_SIZE, Stat.PackSize);
      AddHashBundleRes(Pairs, *HashBundle); }
    else if (Options->TestMode) {
      UString s;
      AddValuePair(s, IDS_ARCHIVES_COLON, Stat.NumArchives, false);
      AddSizePair(s, IDS_PROP_PACKED_SIZE, Stat.PackSize);
      if (Stat.NumFolders != 0)
        AddValuePair(s, IDS_PROP_FOLDERS, Stat.NumFolders);
      AddValuePair(s, IDS_PROP_FILES, Stat.NumFiles);
      AddSizePair(s, IDS_PROP_SIZE, Stat.UnpackSize);
      if (Stat.NumAltStreams != 0) {
        s.Add_LF();
        AddValuePair(s, IDS_PROP_NUM_ALT_STREAMS, Stat.NumAltStreams);
        AddSizePair(s, IDS_PROP_ALT_STREAMS_SIZE, Stat.AltStreams_UnpackSize); }
      s.Add_LF();
      AddLangString(s, IDS_MESSAGE_NO_ERRORS);
      FinalMessage.OkMessage.Title = Title;
      FinalMessage.OkMessage.Message = s; } }

  return res; }
HRESULT ExtractGUI(
    CCodecs *codecs,
    const CObjectVector<COpenType> &formatIndices,
    const CIntVector &excludedFormatIndices,
    UStringVector &archivePaths,
    UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    CExtractOptions &options,

    CHashBundle *hb,

    bool showDialog,
    bool &messageWasDisplayed,
    CExtractCallbackImp *extractCallback,
    HWND hwndParent) {
  messageWasDisplayed = false;
  CThreadExtracting extracter;
  extracter.codecs = codecs;
  extracter.FormatIndices = &formatIndices;
  extracter.ExcludedFormatIndices = &excludedFormatIndices;
  if (!options.TestMode) {
    FString outputDir = options.OutputDir;

    if (outputDir.IsEmpty())
      GetCurrentDir(outputDir);

    if (showDialog) {
      CExtractDialog dialog;
      FString outputDirFull;
      if (!MyGetFullPathName(outputDir, outputDirFull)) {
        ShowErrorMessage(kIncorrectOutDir);
        messageWasDisplayed = true;
        return E_FAIL; }
      NName::NormalizeDirPathPrefix(outputDirFull);
      dialog.DirPath = fs2us(outputDirFull);
      dialog.OverwriteMode = options.OverwriteMode;
      dialog.OverwriteMode_Force = options.OverwriteMode_Force;
      dialog.PathMode = options.PathMode;
      dialog.PathMode_Force = options.PathMode_Force;
      dialog.ElimDup = options.ElimDup;
      if (archivePathsFull.Size() == 1)
        dialog.ArcPath = archivePathsFull[0];


      dialog.NtSecurity = options.NtOptions.NtSecurity;
      if (extractCallback->PasswordIsDefined)
        dialog.Password = extractCallback->Password;

      if (dialog.Create(hwndParent) != IDOK)
        return E_ABORT;
      outputDir = us2fs(dialog.DirPath);
      options.OverwriteMode = dialog.OverwriteMode;
      options.PathMode = dialog.PathMode;
      options.ElimDup = dialog.ElimDup;


      options.NtOptions.NtSecurity = dialog.NtSecurity;
      extractCallback->Password = dialog.Password;
      extractCallback->PasswordIsDefined = !dialog.Password.IsEmpty();

    }
    if (!MyGetFullPathName(outputDir, options.OutputDir)) {
      ShowErrorMessage(kIncorrectOutDir);
      messageWasDisplayed = true;
      return E_FAIL; }
    NName::NormalizeDirPathPrefix(options.OutputDir);
  }
  UString title = LangString(options.TestMode ? IDS_PROGRESS_TESTING : IDS_PROGRESS_EXTRACTING);
  extracter.Title = title;
  extracter.ExtractCallbackSpec = extractCallback;
  extracter.ExtractCallbackSpec->ProgressDialog = &extracter;
  extracter.ExtractCallback = extractCallback;
  extracter.ExtractCallbackSpec->Init();
  extracter.CompressingMode = false;
  extracter.ArchivePaths = &archivePaths;
  extracter.ArchivePathsFull = &archivePathsFull;
  extracter.WildcardCensor = &wildcardCensor;
  extracter.Options = &options;

  extracter.HashBundle = hb;

  extracter.IconID = IDI_ICON;
  RINOK(extracter.Create(title, hwndParent));
  messageWasDisplayed = extracter.ThreadFinishedOK && extracter.MessagesDisplayed;
  return extracter.Result; }
