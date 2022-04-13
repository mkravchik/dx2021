#include "StdAfx.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/Wildcard.h"
#include "../../../Windows/FileName.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/ResourceString.h"
#include "../FileManager/HelpUtils.h"
#include "../FileManager/BrowseDialog.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/resourceGui.h"
#include "ExtractDialog.h"
#include "ExtractDialogRes.h"
#include "ExtractRes.h"



using namespace NWindows;
using namespace NFile;
using namespace NName;
extern HINSTANCE g_hInstance;
static const UInt32 kPathMode_IDs[] = {
  IDS_EXTRACT_PATHS_FULL,
  IDS_EXTRACT_PATHS_NO,
  IDS_EXTRACT_PATHS_ABS
};
static const UInt32 kOverwriteMode_IDs[] = {
  IDS_EXTRACT_OVERWRITE_ASK,
  IDS_EXTRACT_OVERWRITE_WITHOUT_PROMPT,
  IDS_EXTRACT_OVERWRITE_SKIP_EXISTING,
  IDS_EXTRACT_OVERWRITE_RENAME,
  IDS_EXTRACT_OVERWRITE_RENAME_EXISTING
};

static const

  int
  kPathModeButtonsVals[] = {
  NExtract::NPathMode::kFullPaths,
  NExtract::NPathMode::kNoPaths,
  NExtract::NPathMode::kAbsPaths
};
static const
  int

  kOverwriteButtonsVals[] = {
  NExtract::NOverwriteMode::kAsk,
  NExtract::NOverwriteMode::kOverwrite,
  NExtract::NOverwriteMode::kSkip,
  NExtract::NOverwriteMode::kRename,
  NExtract::NOverwriteMode::kRenameExisting
};
static const unsigned kHistorySize = 16;



void AddComboItems(NControl::CComboBox &combo, const UInt32 *langIDs, unsigned numItems, const int *values, int curVal) {
  int curSel = 0;
  for (unsigned i = 0; i < numItems; i++) {
    UString s = LangString(langIDs[i]);
    s.RemoveChar(L'&');
    int index = (int)combo.AddString(s);
    combo.SetItemData(index, i);
    if (values[i] == curVal)
      curSel = i; }
  combo.SetCurSel(curSel); }

bool GetBoolsVal(const CBoolPair &b1, const CBoolPair &b2) {
  if (b1.Def) return b1.Val;
  if (b2.Def) return b2.Val;
  return b1.Val; }
void CExtractDialog::CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2) {
  CheckButton(id, GetBoolsVal(b1, b2)); }
void CExtractDialog::GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2) {
  bool val = IsButtonCheckedBool(id);
  bool oldVal = GetBoolsVal(b1, b2);
  if (val != oldVal)
    b1.Def = b2.Def = true;
  b1.Val = b2.Val = val; }

bool CExtractDialog::OnInit() {
  _passwordControl.Attach(GetItem(IDE_EXTRACT_PASSWORD));
  _passwordControl.SetText(Password);
  _passwordControl.SetPasswordChar(TEXT('*'));
  _pathName.Attach(GetItem(IDE_EXTRACT_NAME));





  _info.Load();
  if (_info.PathMode == NExtract::NPathMode::kCurPaths)
    _info.PathMode = NExtract::NPathMode::kFullPaths;
  if (!PathMode_Force && _info.PathMode_Force)
    PathMode = _info.PathMode;
  if (!OverwriteMode_Force && _info.OverwriteMode_Force)
    OverwriteMode = _info.OverwriteMode;

  CheckButton_TwoBools(IDX_EXTRACT_NT_SECUR, NtSecurity, _info.NtSecurity);
  CheckButton_TwoBools(IDX_EXTRACT_ELIM_DUP, ElimDup, _info.ElimDup);
  CheckButton(IDX_PASSWORD_SHOW, _info.ShowPassword.Val);
  UpdatePasswordControl();

  _path.Attach(GetItem(IDC_EXTRACT_PATH));
  UString pathPrefix = DirPath;

  if (_info.SplitDest.Val) {
    CheckButton(IDX_EXTRACT_NAME_ENABLE, true);
    UString pathName;
    SplitPathToParts_Smart(DirPath, pathPrefix, pathName);
    if (pathPrefix.IsEmpty())
      pathPrefix = pathName;
    else
      _pathName.SetText(pathName); }
  else
    ShowItem_Bool(IDE_EXTRACT_NAME, false);

  _path.SetText(pathPrefix);

  for (unsigned i = 0; i < _info.Paths.Size() && i < kHistorySize; i++)
    _path.AddString(_info.Paths[i]);
  _pathMode.Attach(GetItem(IDC_EXTRACT_PATH_MODE));
  _overwriteMode.Attach(GetItem(IDC_EXTRACT_OVERWRITE_MODE));
  AddComboItems(_pathMode, kPathMode_IDs, ARRAY_SIZE(kPathMode_IDs), kPathModeButtonsVals, PathMode);
  AddComboItems(_overwriteMode, kOverwriteMode_IDs, ARRAY_SIZE(kOverwriteMode_IDs), kOverwriteButtonsVals, OverwriteMode);

  HICON icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));
  SetIcon(ICON_BIG, icon);


  NormalizePosition();
  return CModalDialog::OnInit(); }

void CExtractDialog::UpdatePasswordControl() {
  _passwordControl.SetPasswordChar(IsShowPasswordChecked() ? 0 : TEXT('*'));
  UString password;
  _passwordControl.GetText(password);
  _passwordControl.SetText(password); }

bool CExtractDialog::OnButtonClicked(int buttonID, HWND buttonHWND) {
  switch (buttonID) {
    case IDB_EXTRACT_SET_PATH:
      OnButtonSetPath();
      return true;

    case IDX_EXTRACT_NAME_ENABLE:
      ShowItem_Bool(IDE_EXTRACT_NAME, IsButtonCheckedBool(IDX_EXTRACT_NAME_ENABLE));
      return true;
    case IDX_PASSWORD_SHOW: {
      UpdatePasswordControl();
      return true; }

  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND); }
void CExtractDialog::OnButtonSetPath() {
  UString currentPath;
  _path.GetText(currentPath);
  UString title = LangString(IDS_EXTRACT_SET_FOLDER);
  UString resultPath;
  if (!MyBrowseForFolder(*this, title, currentPath, resultPath))
    return;

  _path.SetCurSel(-1);

  _path.SetText(resultPath); }
void AddUniqueString(UStringVector &list, const UString &s) {
  FOR_VECTOR (i, list)
    if (s.IsEqualTo_NoCase(list[i]))
      return;
  list.Add(s); }
void CExtractDialog::OnOK() {

  int pathMode2 = kPathModeButtonsVals[_pathMode.GetCurSel()];
  if (PathMode != NExtract::NPathMode::kCurPaths ||
      pathMode2 != NExtract::NPathMode::kFullPaths)
    PathMode = (NExtract::NPathMode::EEnum)pathMode2;
  OverwriteMode = (NExtract::NOverwriteMode::EEnum)kOverwriteButtonsVals[_overwriteMode.GetCurSel()];

  _passwordControl.GetText(Password);



  GetButton_Bools(IDX_EXTRACT_NT_SECUR, NtSecurity, _info.NtSecurity);
  GetButton_Bools(IDX_EXTRACT_ELIM_DUP, ElimDup, _info.ElimDup);
  bool showPassword = IsShowPasswordChecked();
  if (showPassword != _info.ShowPassword.Val) {
    _info.ShowPassword.Def = true;
    _info.ShowPassword.Val = showPassword; }
  if (_info.PathMode != pathMode2) {
    _info.PathMode_Force = true;
    _info.PathMode = (NExtract::NPathMode::EEnum)pathMode2;





  }
  if (!OverwriteMode_Force && _info.OverwriteMode != OverwriteMode)
    _info.OverwriteMode_Force = true;
  _info.OverwriteMode = OverwriteMode;



  UString s;



  int currentItem = _path.GetCurSel();
  if (currentItem == CB_ERR) {
    _path.GetText(s);
    if (_path.GetCount() >= kHistorySize)
      currentItem = _path.GetCount() - 1; }
  else
    _path.GetLBText(currentItem, s);

  s.Trim();
  NName::NormalizeDirPathPrefix(s);

  bool splitDest = IsButtonCheckedBool(IDX_EXTRACT_NAME_ENABLE);
  if (splitDest) {
    UString pathName;
    _pathName.GetText(pathName);
    pathName.Trim();
    s += pathName;
    NName::NormalizeDirPathPrefix(s); }
  if (splitDest != _info.SplitDest.Val) {
    _info.SplitDest.Def = true;
    _info.SplitDest.Val = splitDest; }

  DirPath = s;

  _info.Paths.Clear();

  AddUniqueString(_info.Paths, s);

  for (int i = 0; i < _path.GetCount(); i++)
    if (i != currentItem) {
      UString sTemp;
      _path.GetLBText(i, sTemp);
      sTemp.Trim();
      AddUniqueString(_info.Paths, sTemp); }
  _info.Save();

  CModalDialog::OnOK(); }


void CExtractDialog::OnHelp() {
  ShowHelpWindow("fm/plugins/7-zip/extract.htm");
  CModalDialog::OnHelp(); }
