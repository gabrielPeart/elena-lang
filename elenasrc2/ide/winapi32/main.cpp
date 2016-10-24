//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA IDE
//      WinAPI32 program entry 
//                                              (C)2005-2015, by Alexei Rakov
//---------------------------------------------------------------------------

#include "winide.h"
#include "winideconst.h"
////#include <direct.h>
//#include "winapi32\wingraphic.h"
#include "winapi32\winsplitter.h"
#include "..\appwindow.h"
//#include "..\settings.h"
//
//// --- command line arguments ---
//#define CMD_CONFIG_PATH    _T("-c")

using namespace _GUI_;

//class IDEWindowsDialog : public WindowsDialog
//{
//protected:
//   MainWindow*  _owner;
//   _Controller* _controller;
//   Model*       _model;
//
//   virtual void onCreate()
//   {
//      DocMapping::Iterator it = _model->mappings.start();
//      while (!it.Eof()) {
//         addWindow(it.key());
//
//         it++;
//      }
//
//      selectWindow(_owner->getCurrentDocumentIndex());
//   }
//
//   virtual void onOK()
//   {
//      _controller->doSelectWindow(getSelectedWindow());
//   }
//
//   virtual void onClose()
//   {
//      int count = 0;
//      int* selected = getSelectedWindows(count);
//      
//      int offset = 0;
//      for (int i = 0 ; i < count; i++) {
//         if(!_controller->doCloseFile(selected[i] - offset))
//            break;
//      
//         offset++;
//      }
//      free(selected);
//   }
//
//public:
//   IDEWindowsDialog(MainWindow* owner, _Controller* controller, Model* model)
//      : WindowsDialog(owner)
//   {
//      _controller = controller;
//      _model = model;
//      _owner = owner;
//   }
//};
//
//class Win32IDEView : public _View, public _DebugListener
//{
//public:
//   MainWindow appWindow;
//
//   virtual bool selectProject(Model* model, _ELENA_::Path& path)
//   {
//      FileDialog dialog(&appWindow, FileDialog::ProjectFilter, OPEN_PROJECT_CAPTION, model->paths.lastPath);
//
//      path.copy(dialog.openFile());
//
//      return !path.isEmpty();
//   }
//
//
//
//   virtual int newDocument(text_t name, Document* doc)
//   {
//      return appWindow.newDocument(name, doc);
//   }
//
//   virtual void checkMenuItemById(int id, bool doEnable)
//   {
//      appWindow.getMenu()->checkItemById(id, doEnable);
//   }
//
//   virtual bool configEditor(Model* model)
//   {
//      EditorSettings dlg(&appWindow, model);
//
//      return dlg.showModal() != 0;
//   }
//
//   virtual bool configDebugger(Model* model)
//   {
//      DebuggerSettings dlg(&appWindow, model);
//
//      return dlg.showModal() != 0;
//   }
//
//   virtual bool configurateForwards(_ProjectManager* project)
//   {
//      ProjectForwardsDialog dlg(&appWindow, project);
//
//      return dlg.showModal() != 0;
//   }
//
//   virtual bool about(Model* model)
//   {
//      AboutDialog dlg(&appWindow);
//
//      return dlg.showModal() != 0;
//   }
//
//   virtual bool find(Model* model, SearchOption* option, SearchHistory* searchHistory)
//   {
//      FindDialog dialog(&appWindow, false, option, searchHistory, NULL);
//
//      return dialog.showModal();
//   }
//
//   virtual bool replace(Model* model, SearchOption* option, SearchHistory* searchHistory, SearchHistory* replaceHistory)
//   {
//      FindDialog dialog(&appWindow, true, option, searchHistory, replaceHistory);
//
//      return dialog.showModal();
//   }
//
//   virtual bool gotoLine(int& row)
//   {
//      GoToLineDialog dlg(&appWindow, row);
//      if (dlg.showModal()) {
//         row = dlg.getLineNumber();
//
//         return true;
//      }
//      else return false;
//   }
//
//   virtual bool selectWindow(Model* model, _Controller* controller)
//   {
//      IDEWindowsDialog dialog(&appWindow, controller, model);
//
//      if (dialog.showModal() == -2) {
//         return true;
//      }
//      else return false;
//   }
//
//   virtual void reloadSettings()
//   {
//      appWindow.reloadSettings();
//   }
//
//   virtual void removeFile(const wchar_t* name)
//   {
//      _wremove(name);
//   }
//
//   virtual void switchToOutput()
//   {
//      appWindow.switchToOutput();
//   }
//
//   virtual void openOutput()
//   {
//      appWindow.openOutput();
//   }
//
//   virtual void closeOutput()
//   {
//      appWindow.closeOutput();
//   }
//
//   virtual void openVMConsole()
//   {
//      appWindow.openVMConsole();
//   }
//
//   virtual void closeVMConsole()
//   {
//      appWindow.closeVMConsole();
//   }
//
//   virtual void openMessageList()
//   {
//      appWindow.openMessageList();
//   }
//
//   virtual void closeMessageList()
//   {
//      appWindow.closeMessageList();
//   }
//
//   virtual void openDebugWatch()
//   {
//      appWindow.openDebugWatch();
//   }
//
//   virtual void closeDebugWatch()
//   {
//      appWindow.closeDebugWatch();
//   }
//
//   virtual void openCallList()
//   {
//      appWindow.openCallList();
//   }
//
//   virtual void closeCallList()
//   {
//      appWindow.closeCallList();
//   }
//
//   virtual void clearMessageList()
//   {
//      appWindow.clearMessageList();
//   }
//
//   virtual void openProjectView()
//   {
//      appWindow.openProjectView();
//   }
//
//   virtual void closeProjectView()
//   {
//      appWindow.closeProjectView();
//   }
//
//   virtual bool compileProject(_ProjectManager* project, int postponedAction)
//   {
//      return appWindow.compileProject(project, postponedAction);
//   }
//
//   virtual void resetDebugWindows()
//   {
//      appWindow.resetDebugWindows();
//   }
//
//   virtual void refreshDebugWindows(_ELENA_::_DebugController* debugController)
//   {
//      appWindow.refreshDebugWindows(debugController);
//   }
//
//   virtual void browseWatch(_ELENA_::_DebugController* debugController, void* watchNode)
//   {
//      appWindow.browseWatch(debugController, watchNode);
//   }
//
//   virtual void browseWatch(_ELENA_::_DebugController* debugController)
//   {
//      appWindow.browseWatch(debugController);
//   }
//
//   virtual void onStop(bool failed)
//   {
//      appWindow._notify(failed ? IDE_DEBUGGER_BREAK : IDE_DEBUGGER_STOP);
//   }
//   
//   void onCheckPoint(const wchar_t* message)
//   {
//      appWindow._notify(IDE_DEBUGGER_CHECKPOINT, message);
//   }
//   
//   void onNotification(const wchar_t* message, size_t address, int code)
//   {
//      appWindow._notify(IDM_DEBUGGER_EXCEPTION, message, address, code);
//   }
//
//   virtual void onStep(_ELENA_::ident_t ns, _ELENA_::ident_t source, int row, int disp, int length)
//   {
//      appWindow._notify(IDE_DEBUGGER_STEP, TextString(ns), TextString(source), HighlightInfo(row, disp, length));
//   }
//
//   virtual void onDebuggerHook()
//   {
//      appWindow._notify(IDE_DEBUGGER_HOOK);
//   }   
//
//   virtual void onStart()
//   {
//      appWindow._notify(IDE_DEBUGGER_START);
//   }
//
//   Win32IDEView(HINSTANCE hInstance, _Controller* controller, Model* model)
//      : appWindow(hInstance, _T("IDE"), controller, model)
//   {
//
//   }
//};
//
void initCommonControls()
{
   INITCOMMONCONTROLSEX icex;
   icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icex.dwICC  = ICC_WIN95_CLASSES|ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_USEREX_CLASSES|
	   ICC_TAB_CLASSES|ICC_LISTVIEW_CLASSES;

   ::InitCommonControlsEx(&icex);
}

void registerControlClasses(HINSTANCE hInstance)
{
   SDIWindow :: _registerClass(hInstance, ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON)), MAKEINTRESOURCE(IDR_MAIN_MENU));
   TextView :: _registerClass(hInstance);
   Splitter::_registerClass(hInstance, true);
   Splitter::_registerClass(hInstance, false);
}

// --- InitControls ---

void initControls(HINSTANCE instance)
{
   registerControlClasses(instance);
   initCommonControls();
}
//// --- loadCommandLine ---
//
//inline void setOption(Model* model, const wchar_t* parameter)
//{
//   if (parameter[0]!='-') {
//      if (_ELENA_::Path::checkExtension(parameter, _T("l"))) {
//         model->defaultFiles.add(wcsdup(parameter));
//      }
//      else if (_ELENA_::Path::checkExtension(parameter, _T("prj"))) {
//         model->defaultProject.copy(parameter);
//      }
//   }
//   else if (_ELENA_::StringHelper::compare(parameter, _T("-sclassic"))) {
//      model->scheme = 1;
//   }
//}
//
//void loadCommandLine(Model* model, char* cmdLine, _ELENA_::Path& configPath)
//{
//   wchar_t* cmdWLine = (wchar_t*)malloc((strlen(cmdLine) + 1) * 2);
//
//   size_t len = strlen(cmdLine);
//   _ELENA_::StringHelper::copy(cmdWLine, cmdLine, len, len);
//   cmdWLine[len] = 0;
//
//   int start = 0;
//   for (size_t i = 1 ; i <= wcslen(cmdWLine) ; i++) {
//      if (cmdWLine[i]==' ' || cmdWLine[i]==0) {
//         _ELENA_::Path parameter(cmdWLine + start, i - start);
//
//         // check if a custom config file should be loaded
//         if (_ELENA_::StringHelper::compare(parameter, CMD_CONFIG_PATH, _ELENA_::getlength(CMD_CONFIG_PATH))) {
//            configPath.copy(model->paths.appPath);
//            configPath.combine(parameter + _ELENA_::getlength(CMD_CONFIG_PATH));
//         }
//         else setOption(model, parameter);         
//
//         start = i + 1;
//      }
//   }
//
//   _ELENA_::freestr(cmdWLine);
//}
//
//// --- loadSettings ---
//
//void loadSettings(_ELENA_::path_t path, Model* model, Win32IDEView* view)
//{
//   _ELENA_::IniConfigFile file;
//
//   if (file.load(path, _ELENA_::feUTF8)) {
//      Settings::load(model, file);
//
//      view->appWindow.loadHistory(file, RECENTFILES_SECTION, RECENTRPOJECTS_SECTION);
//
//      view->appWindow.reloadSettings();
//   }
//}
//
//// --- saveSettings ---
//
//void saveSettings(_ELENA_::path_t path, Model* model, Win32IDEView* view)
//{
//   _ELENA_::IniConfigFile file;
//
//   Settings::save(model, file);
//
//   view->appWindow.saveHistory(file, RECENTFILES_SECTION, RECENTRPOJECTS_SECTION);
//
//   file.save(path, _ELENA_::feUTF8);
//}

// --- WinMain ---

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE, LPSTR cmdLine, int)
{
   Model         model;

//   // get app path
//   wchar_t appPath[MAX_PATH];
//   ::GetModuleFileName(NULL, appPath, MAX_PATH);
//   ::PathRemoveFileSpec(appPath);
//
//   // get default path
//   wchar_t defPath[MAX_PATH];
//   _wgetcwd(defPath, MAX_PATH);
//
//   // init paths & settings
//   Paths::init(&model, appPath, defPath);
//   Settings::init(&model, _T("..\\src30"), _T("..\\lib30"));
//
//   _ELENA_::Path configPath(model.paths.appPath);
//   configPath.combine(_T("ide.cfg"));
//
//   // load command line argiments
//   loadCommandLine(&model, cmdLine, configPath);

   // init IDE Controls
   initControls(hInstance);

//   Win32AppDebugController controller;

   IDEController ide;
   IDEWindow     view(hInstance, _T("IDE"), &ide, &model);

//   // init IDE settings
//   loadSettings(configPath, &model, &view);
//
////   controller.assign(ide.getAppWindow());

   // start IDE
   ide.start(&view, &view, &model);

   // main program loop
   MSG msg;
   msg.wParam = 0;
   Menu* menu = view.getMenu();
   HWND hwnd = view.getHandle();;
   while (::GetMessage(&msg, NULL, 0, 0)) {
      if (!menu->_translate(hwnd, &msg)) {
         ::TranslateMessage(&msg);
         ::DispatchMessage(&msg);
      }
   }
//   saveSettings(configPath, &model, &view);
//   Font::releaseFontCache();
//
   return 0;
}

