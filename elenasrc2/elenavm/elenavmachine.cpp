//---------------------------------------------------------------------------
//		E L E N A   P r o j e c t:  ELENA JIT Compiler Engine
//
//                                              (C)2009-2019, by Alexei Rakov
//---------------------------------------------------------------------------

#include "elena.h"
// --------------------------------------------------------------------------
#include "elenavmachine.h"
#include "bytecode.h"
#include "rtman.h"

#include <stdarg.h>

using namespace _ELENA_;

#define NMODULE_LEN getlength(NATIVE_MODULE)

#define TEMPLATE_CATEGORY           "configuration/templates/*"
#define PRIMITIVE_CATEGORY          "configuration/primitives/*"
#define FORWARD_CATEGORY            "configuration/forwards/*"

#define PROJECT_TEMPLATE            "configuration/project/template"
//#define SYSTEM_MAXTHREAD            _T("maxthread")
#define LINKER_MGSIZE               "configuration/linker/mgsize"
#define LINKER_YGSIZE               "configuration/linker/ygsize"
#define SYSTEM_PLATFORM             "configuration/system/platform"
#define LIBRARY_PATH                "configuration/project/libpath"

// --- InstanceConfig ---

void InstanceConfig :: loadForwardList(XmlConfigFile& config)
{
   _ConfigFile::Nodes nodes;
   config.select(FORWARD_CATEGORY, nodes);

   _ConfigFile::Nodes::Iterator it = nodes.start();
   while (!it.Eof()) {
      ident_t key = (*it).Attribute("key");
      ident_t value = (*it).Content();

      // if it is a wildcard
      if (key[getlength(key) - 1] == '*') {
         NamespaceName alias(key);
         NamespaceName module(value);

         moduleForwards.erase(alias);
         moduleForwards.add(alias, StrFactory::clone(module));
      }
      else {
         forwards.erase(key);
         forwards.add(key, StrFactory::clone(value));
      }
      it++;
   }
}

void InstanceConfig :: loadList(XmlConfigFile& config, const char* category, path_t path, Map<ident_t, char*>* list)
{
   _ConfigFile::Nodes nodes;
   config.select(category, nodes);

   _ConfigFile::Nodes::Iterator it = nodes.start();
   while (!it.Eof()) {
      ident_t key = (*it).Attribute("key");
      ident_t value = (*it).Content();

      if(emptystr(value))
         value = key;

      // add path if provided
      if (!emptystr(path)) {
         Path filePath(path);

         if (value[0] == '~') {
            filePath.copy(value + 1);
         }
         else filePath.combine(value);

         list->add(key, IdentifierString::clonePath(filePath.c_str()));
      }
      else list->add(key, StrFactory::clone(value));

      it++;
   }
}

bool InstanceConfig :: load(path_t path, Templates* templates)
{
   XmlConfigFile config;
   if (_ELENA_::emptystr(path) || !config.load(path, feUTF8)) {
      return false;
   }

   if (templates != NULL) {
      // load template
      IdentifierString projectTemplate(config.getSetting(PROJECT_TEMPLATE));

      if (!_ELENA_::emptystr(projectTemplate)) {
         Path templatePath(templates->get(projectTemplate));

         load(templatePath.c_str(), templates);
      }
   }

   _ELENA_::Path configPath;
   configPath.copySubPath(path);

   // init config
   init(configPath.c_str(), config);

   return true;
}

void InstanceConfig :: init(path_t configPath, XmlConfigFile& config)
{
   // compiler options
   //maxThread = config.getIntSetting(SYSTEM_CATEGORY, SYSTEM_MAXTHREAD, maxThread);
   mgSize = config.getHexSetting(LINKER_MGSIZE, mgSize);
   ygSize = config.getHexSetting(LINKER_YGSIZE, ygSize);
   platform = config.getIntSetting(SYSTEM_PLATFORM, platform);

   const char* path = config.getSetting(LIBRARY_PATH);
   if (!emptystr(path)) {
      libPath.copy(configPath);
      libPath.combine(path);
   }

   loadList(config, PRIMITIVE_CATEGORY, configPath, &primitives);
   loadForwardList(config);
}

// --- Instance::ImageReferenceHelper ---

void Instance::ImageReferenceHelper :: writeTape(MemoryWriter& tape, void* vaddress, int mask)
{
   int ref = (size_t)vaddress - (test(mask, mskRDataRef) ? _statBase : _codeBase);

   tape.writeDWord(ref | mask);
}

void Instance::ImageReferenceHelper :: writeReference(MemoryWriter& writer, ref_t reference, size_t disp, _Module* module)
{
   size_t pos = reference & ~mskImageMask;
   if (test(reference, mskRelCodeRef)) {
      writer.writeDWord(pos - writer.Position() - 4);
   }
   else writer.writeDWord((test(reference, mskRDataRef) ? _statBase : _codeBase) + pos + disp);
}

void Instance::ImageReferenceHelper :: writeReference(MemoryWriter& writer, void* vaddress, bool relative, size_t disp)
{
   ref_t address = (ref_t)vaddress;

   // calculate relative address
   if (relative)
      address -= ((ref_t)writer.Address() + 4);

   writer.writeDWord(address + disp);
}

void Instance::ImageReferenceHelper :: writeMTReference(MemoryWriter& writer)
{
   _Memory* section = _instance->getMessageSection();

   writer.writeDWord((ref_t)section->get(0));
}

void Instance::ImageReferenceHelper :: addBreakpoint(size_t position)
{
   MemoryWriter writer(_instance->getTargetDebugSection());

   writer.writeDWord(_codeBase + position);
}

// --- Instance ---

Instance :: Instance(ELENAVMMachine* machine)
   : _config(machine->config)
{
   _linker = NULL;
   _compiler = NULL;
   _messageTable = nullptr;
   _messageBodyTable = nullptr;

   _initialized = false;
   _debugMode = false;

//   _traceMode = traceMode;

   _machine = machine;

   // init loader based on default machine config
   initLoader(_machine->config);

   // create message table module
   _ConvertedMTSize = 0;
   LoadResult result = lrSuccessful;
   _Module* messages = _loader.createModule(MESSAGE_TABLE_MODULE, result);
   if (result == lrSuccessful) {
      _messageTable = messages->mapSection(messages->mapReference(MESSAGE_TABLE + getlength(MESSAGE_TABLE_MODULE)) | mskRDataRef, false);
      _messageBodyTable = messages->mapSection(messages->mapReference(MESSAGEBODY_TABLE + getlength(MESSAGE_TABLE_MODULE)) | mskRDataRef, false);

      clearMessageTable();
   }
   else throw EAbortException();
}

Instance :: ~Instance()
{
   freeobj(_linker);
   freeobj(_compiler);
}

ident_t Instance :: resolveForward(ident_t forward)
{
   ident_t reference = _config.forwards.get(forward);
   // if no forward mapping was found try to resolve on the module level
   if (emptystr(reference)) {
      NamespaceName alias(forward);

      ident_t module = _config.moduleForwards.get(alias);
      // if there is a module mapping create an appropriate forward
      if (!emptystr(module)) {
         ReferenceName name(forward);
         ReferenceNs newRefeference(module, name);

         _config.forwards.add(forward, StrFactory::clone(newRefeference));

         reference = _config.forwards.get(forward);
      }
   }
   return reference;
}

size_t Instance :: getLinkerConstant(int id)
{
   switch (id) {
      case lnGCMGSize:
         return _config.mgSize;
      case lnGCYGSize:
         return _config.ygSize;
      //case lnThreadCount:
      //   return (size_t)_config.maxThread;
      case lnObjectSize:
         return _compiler->getObjectHeaderSize();
      case lnVMAPI_Instance:
         return (size_t)this;
      default:
         return 0;
   }
}

void Instance :: printInfo(const wchar_t* msg, ...)
{
   va_list argptr;
   va_start(argptr, msg);

   vwprintf(msg, argptr);
   va_end(argptr);
   wprintf(L"\n");

   fflush(stdout);
}

ident_t Instance :: resolveTemplateWeakReference(ident_t referenceName)
{
   ident_t resolvedName = resolveForward(referenceName + TEMPLATE_PREFIX_NS_LEN);
   if (emptystr(resolvedName)) {
      if (referenceName.endsWith(CLASSCLASS_POSTFIX)) {
         // HOTFIX : class class reference should be resolved simultaneously with class one
         IdentifierString classReferenceName(referenceName, getlength(referenceName) - getlength(CLASSCLASS_POSTFIX));

         classReferenceName.copy(resolveTemplateWeakReference(classReferenceName.c_str()));
         classReferenceName.append(CLASSCLASS_POSTFIX);

         addForward(referenceName + TEMPLATE_PREFIX_NS_LEN, classReferenceName.c_str());

         return resolveForward(referenceName + TEMPLATE_PREFIX_NS_LEN);
      }

      // COMPILER MAGIC : try to find a template implementation
      ref_t resolvedRef = 0;
      _Module* refModule = resolveWeakModule(referenceName + TEMPLATE_PREFIX_NS_LEN, resolvedRef);
      if (refModule != nullptr) {
         ident_t resolvedReferenceName = refModule->resolveReference(resolvedRef);
         if (isWeakReference(resolvedReferenceName)) {
            IdentifierString fullName(refModule->Name(), resolvedReferenceName);

            addForward(referenceName + TEMPLATE_PREFIX_NS_LEN, fullName);
         }
         else addForward(referenceName + TEMPLATE_PREFIX_NS_LEN, resolvedReferenceName);

         referenceName = resolveForward(referenceName + TEMPLATE_PREFIX_NS_LEN);
      }
      else throw JITUnresolvedException(referenceName);
   }
   else referenceName = resolvedName;

   return referenceName;
}

ReferenceInfo Instance :: retrieveReference(_Module* module, ref_t reference, ref_t mask)
{
   if (mask == mskLiteralRef || mask == mskInt32Ref || mask == mskRealRef || mask == mskInt64Ref || mask == mskCharRef || mask == mskWideLiteralRef) {
      return module->resolveConstant(reference);
   }
   // if it is a message
   else if (mask == 0) {
      ref_t signRef = 0;
      return module->resolveAction(reference, signRef);
   }
   // if it is constant
   else {
      ident_t referenceName = module->resolveReference(reference);
      while (isForwardReference(referenceName)) {
         ident_t resolvedName = resolveForward(referenceName + FORWARD_PREFIX_NS_LEN);
         if (!emptystr(resolvedName)) {
            referenceName = resolvedName;
         }
         else throw JITUnresolvedException(referenceName);
      }

      if (isWeakReference(referenceName)) {
         if (isTemplateWeakReference(referenceName)) {
            referenceName = resolveTemplateWeakReference(referenceName);
         }

         return ReferenceInfo(module, referenceName);
      }
      return ReferenceInfo(referenceName);
   }
}

ident_t Instance :: retrieveReference(void* address, ref_t mask)
{
   //if (mask == 0) {
   //   return retrieveKey(_actions.start(), (ref_t)address, DEFAULT_STR);
   //}
   //else {
      switch (mask & mskImageMask) {
         case mskRDataRef:
            return retrieveKey(_dataReferences.start(), (ref_t)address, DEFAULT_STR);
         default:
            return nullptr;
      }
   //}
}

_Module* Instance :: resolveModule(ident_t referenceName, LoadResult& result, ref_t& reference)
{
   while (isWeakReference(referenceName)) {
      referenceName = resolveForward(referenceName);
   }
   if (emptystr(referenceName)) {
      result = lrNotFound;

      return NULL;
   }
   return _loader.resolveModule(referenceName, result, reference);
}

_Module* Instance :: resolveWeakModule(ident_t weakReferenceName, ref_t& reference)
{
   LoadResult result = lrNotFound;
   _Module* module = _loader.resolveWeakModule(weakReferenceName, result, reference);
   if (result != lrSuccessful) {
      // Bad luck : try to resolve it indirectly
      module = _loader.resolveIndirectWeakModule(weakReferenceName, result, reference);
      if (result != lrSuccessful) {
         return NULL;
      }
      else return module;
   }
   else return module;
}

SectionInfo Instance :: getSectionInfo(ReferenceInfo referenceInfo, size_t mask, bool silentMode)
{
   SectionInfo sectionInfo;
   LoadResult result;

   if (referenceInfo.isRelative()) {
      ref_t referenceID = referenceInfo.module->mapReference(referenceInfo.referenceName, true);

      sectionInfo.module = referenceInfo.module;
      sectionInfo.section = sectionInfo.module->mapSection(referenceID | mask, true);
   }
   else {
      ref_t      referenceID = 0;

      if (referenceInfo.referenceName.compare(NATIVE_MODULE, NMODULE_LEN) && referenceInfo.referenceName[NMODULE_LEN] == '\'') {
         sectionInfo.module = _loader.resolveNative(referenceInfo.referenceName, result, referenceID);
      }
      else sectionInfo.module = resolveModule(referenceInfo.referenceName, result, referenceID);

      sectionInfo.section = sectionInfo.module ? sectionInfo.module->mapSection(referenceID | mask, true) : NULL;
   }

   if (sectionInfo.section == NULL && !silentMode) {
      throw JITUnresolvedException(referenceInfo);
   }

   return sectionInfo;
}

SectionInfo Instance :: getCoreSectionInfo(ref_t reference, size_t mask)
{
   SectionInfo sectionInfo;

   LoadResult result = lrNotFound;
   sectionInfo.module = _loader.resolveCore(reference, result);
   sectionInfo.section = sectionInfo.module ? sectionInfo.module->mapSection(reference | mask, true) : NULL;

   if (sectionInfo.section == NULL) {
      throw InternalError("Internal error");
   }

   return sectionInfo;
}

ClassSectionInfo Instance :: getClassSectionInfo(ReferenceInfo referenceInfo, size_t codeMask, size_t vmtMask, bool silentMode)
{
   ClassSectionInfo sectionInfo;

   ref_t referenceID = 0;
   LoadResult result;
   if (referenceInfo.isRelative()) {
      if (isTemplateWeakReference(referenceInfo.referenceName)) {
         sectionInfo.module = resolveModule(referenceInfo.referenceName, result, referenceID);
      }
      else {
         sectionInfo.module = referenceInfo.module;
         referenceID = referenceInfo.module->mapReference(referenceInfo.referenceName, true);
      }
   }
   else sectionInfo.module = resolveModule(referenceInfo.referenceName, result, referenceID);

   if (sectionInfo.module == NULL || referenceID == 0) {
      if (!silentMode)
         throw JITUnresolvedException(referenceInfo);
   }
   else {
      sectionInfo.codeSection = sectionInfo.module->mapSection(referenceID | codeMask, true);
      sectionInfo.vmtSection = sectionInfo.module->mapSection(referenceID | vmtMask, true);
   }

   return sectionInfo;
}

void Instance :: addForward(ident_t forward, ident_t reference)
{
   if (forward[getlength(forward) - 1] == '*') {
      NamespaceName alias(forward);
      NamespaceName module(reference);

      _config.moduleForwards.erase(alias);

      _config.moduleForwards.add(alias, StrFactory::clone(module));
   }
   else {
      _config.forwards.erase(forward);

      _config.forwards.add(forward, reference.clone());
   }
}

void Instance :: addForward(ident_t line)
{
   size_t sep = line.find('=', -1);
   if(sep != -1) {
      ident_t reference = line + sep + 1;
      IdentifierString forward(line, sep);

      addForward(forward, reference);
   }
}

void Instance :: onNewCode(SystemEnv* env)
{
   resolveMessageTable();

   env->Table->gc_rootcount = (_linker->getStaticCount() << 2);
}

ident_t Instance :: getSubject(ref_t subjectRef)
{
   return _linker->retrieveResolvedAction(subjectRef);
}

void* Instance :: loadSymbol(ident_t reference, int mask, bool silentMode)
{
   // reference should not be a forward one
   while (isForwardReference(reference)) {
      ident_t resolved = resolveForward(reference + FORWARD_PREFIX_NS_LEN);
      if (emptystr(resolved)) {

         throw JITUnresolvedException(reference);
      }
      else reference = resolved;
   }
   return _linker->resolve(reference, mask, silentMode);
}

bool Instance :: initLoader(InstanceConfig& config)
{
   // load paths
   _loader.setRootPath(config.libPath.c_str());

   // load primitives
   Primitives::Iterator it = config.primitives.start();
   while (!it.Eof()) {
      Path path(*it);
      if (it.key().compare(CORE_ALIAS)) {
         _loader.addCorePath(path.c_str());
      }
      else _loader.addPrimitivePath(it.key(), path.c_str());

      it++;
   }

   return true;
}

void Instance :: clearMessageTable()
{
   _messageTable->trim(0);
   _messageBodyTable->trim(0);

   _messageTable->writeBytes(0, 0, 8); // write dummy place holder
   _messageBodyTable->writeBytes(0, 0, 4); // write dummy place holder
}

void Instance :: resolveMessageTable()
{
   while (_messageTable->Length() > _ConvertedMTSize) {
      // !! HOTFIX : the message section should be overwritten
      getMessageSection()->trim(0);

      _ConvertedMTSize = _messageTable->Length();

      _linker->resolve(MESSAGE_TABLE, mskMessageTableRef, true);
   }
}

bool Instance :: restart(SystemEnv* env, void* sehTable, bool debugMode)
{
   printInfo(ELENAVM_GREETING, ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ELENAVM_REVISION);
   printInfo(L"Initializing...");

   clearReferences();
   clearMessageTable();
   _ConvertedMTSize = 0;

   //TODO: clear message table?

   _literalClass.copy(_config.forwards.get(STR_FORWARD));
   _wideLiteralClass.copy(_config.forwards.get(WIDESTR_FORWARD));
   _characterClass.copy(_config.forwards.get(CHAR_FORWARD));
   _intClass.copy(_config.forwards.get(INT_FORWARD));
   _realClass.copy(_config.forwards.get(REAL_FORWARD));
   _longClass.copy(_config.forwards.get(LONG_FORWARD));
   _msgClass.copy(_config.forwards.get(MESSAGE_FORWARD));
   _extMsgClass.copy(_config.forwards.get(EXT_MESSAGE_FORWARD));
   _subjClass.copy(_config.forwards.get(MESSAGENAME_FORWARD));

   // init debug section
   if (_debugMode) {
      printInfo(L"Debug mode...");
   }

   _compiler->setTLSKey((void*)*env->TLSIndex);
   _compiler->setThreadTable(env->ThreadTable);
   _compiler->setEHTable(sehTable);
   _compiler->setGCTable(env->Table);

   // load predefined code
   _linker->prepareCompiler();

   // HOTFIX : literal constant is refered in the object, so it should be preloaded
   _linker->resolve(_literalClass, mskVMTRef, true);

   // HOTFIX : resolve message table
   resolveMessageTable();

   _initialized = true;

   // set debug ptr if requiered
   if (_debugMode) {
      env->Table->dbg_ptr = (pos_t)loadDebugSection();
   }

   // HOTFIX : set gc_roots
   env->Table->gc_roots = (pos_t)getTargetSection(mskStatRef)->get(0);
   env->Table->gc_rootcount = (_linker->getStaticCount() << 2);

   printInfo(L"Done...");

   return true;
}

void Instance :: translate(MemoryReader& reader, ImageReferenceHelper& helper, MemoryDump& dump, int terminator)
{
   MemoryWriter ecodes(&dump);

   ecodes.writeDWord(0);            // write size place holder
   int procPtr = ecodes.Position();

   // open 0
   ecodes.writeByte(bcOpen);
   ecodes.writeDWord(0);

   // resolve tape
   size_t command = reader.getDWord();
   void*  extra_param;
   while (command != terminator) {
      ident_t arg = NULL;
      size_t param = reader.getDWord();
      if (test(command, LITERAL_ARG_MASK)) {
         arg = (const char*)reader.Address();

         reader.seek(reader.Position() + param);  // goes to the next record
      }

      // in debug mode place a breakpoint excluding prefix command
      if (_debugMode)
         ecodes.writeByte(bcBreakpoint);

      switch(command) {
         case ARG_TAPE_MESSAGE_ID:
            extra_param = loadSymbol(arg, mskVMTRef);
            break;
         case CALL_TAPE_MESSAGE_ID: 
            //callr
            //pusha
            ecodes.writeByte(bcCallR);
            helper.writeTape(ecodes, loadSymbol(arg, mskSymbolRef), mskCodeRef);
            ecodes.writeByte(bcPushA);
            break;
         case PUSH_VAR_MESSAGE_ID:
            // pushfi param
            ecodes.writeByte(bcPushFI);
            ecodes.writeDWord(param);
            break;
         case ASSIGN_VAR_MESSAGE_ID:
            // popa
            // asavefi param
            ecodes.writeByte(bcPopA);
            ecodes.writeByte(bcASaveFI);
            ecodes.writeDWord(param);
            break;
         case PUSH_TAPE_MESSAGE_ID:
            //pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskConstantRef), mskRDataRef);
            //level++;

            break;
         case PUSHS_TAPE_MESSAGE_ID:
            //pushr constant
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskLiteralRef), mskRDataRef);
            break;
         case PUSHN_TAPE_MESSAGE_ID:
            //pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskInt32Ref), mskRDataRef);
            break;
         case PUSHR_TAPE_MESSAGE_ID:
            //pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskRealRef), mskRDataRef);
            break;
         case PUSHL_TAPE_MESSAGE_ID:
            //pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskInt64Ref), mskRDataRef);
            break;
         case PUSHM_TAPE_MESSAGE_ID:
            // pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskMessage), mskRDataRef);
            break;
         case PUSHE_TAPE_MESSAGE_ID:
            // pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskExtMessage), mskRDataRef);
            break;
         case PUSHG_TAPE_MESSAGE_ID:
            // pushr r
            ecodes.writeByte(bcPushR);
            helper.writeTape(ecodes, loadSymbol(arg, mskMessageName), mskRDataRef);
            break;
         case POP_TAPE_MESSAGE_ID:
            //popi param
            ecodes.writeByte(bcPopI);
            ecodes.writeDWord(param);
            break;
         case REVERSE_TAPE_MESSAGE_ID:
            if (param == 2) {
               // popa
               // aswapsi 0
               // pusha
               ecodes.writeByte(bcPopA);
               ecodes.writeByte(bcASwapSI);
               ecodes.writeDWord(0);
               ecodes.writeByte(bcPushA);
            }
            else {
               int length = param >> 1;
               param--;

               // popa
               // aswapsi 0
               // pusha
               ecodes.writeByte(bcPopA);
               ecodes.writeByte(bcASwapSI);
               ecodes.writeDWord(param - 1);
               ecodes.writeByte(bcPushA);

               for (int i = 1 ; i < length ; i++) {
                  // aloadsi i
                  // aswapsi n - 1 - i
                  ecodes.writeByte(bcALoadSI);
                  ecodes.writeDWord(i);
                  ecodes.writeByte(bcASwapSI);
                  ecodes.writeDWord(param - i);
               }
            }
            break;
         case SEND_TAPE_MESSAGE_ID:
            //copym message
            //aloadsi 0
            //acallvi 0
            //pusha

            ecodes.writeByte(bcCopyM);
            ecodes.writeDWord(_linker->parseMessage(arg, false));
            ecodes.writeByte(bcALoadSI);
            ecodes.writeDWord(0);
            ecodes.writeByte(bcACallVI);
            ecodes.writeDWord(0);
            ecodes.writeByte(bcPushA);

            break;
         case NEW_TAPE_MESSAGE_ID:
         {
            int level = param;

            // new n, vmt
            ecodes.writeByte(bcNew);
            helper.writeTape(ecodes, extra_param, /*mskVMTRef*/mskRDataRef);
            ecodes.writeDWord(param);

            // ; assign content
            // bcopya
            
            ecodes.writeByte(bcBCopyA);

            if (level > 5) {
               // dcopy level
               // labNext
               // dec
               // popa
               // xset
               // greatern 0,labNext
               ecodes.writeByte(bcDCopy);
               ecodes.writeDWord(level);
               ecodes.writeByte(bcNop);
               ecodes.writeByte(bcDec);
               ecodes.writeByte(bcPopA);
               ecodes.writeByte(bcXSet);
               ecodes.writeByte(bcGreaterN);
               ecodes.writeDWord(0);
               ecodes.writeDWord(-13);
            }
            else {
               // ; repeat param-time
               // popa
               // axsavebi i
               while (level > 0) {
                  ecodes.writeByte(bcPopA);
                  ecodes.writeByte(bcAXSaveBI);
                  level--;
                  ecodes.writeDWord(level);
               }
            }

            // pushb
            ecodes.writeByte(bcPushB);
            break;
         }
      }
      command = reader.getDWord();
   }
   // EOP breakpoint
   if (_debugMode)
      ecodes.writeByte(bcBreakpoint);

   // popa
   // close
   // quit
   ecodes.writeByte(bcPopA);
   ecodes.writeByte(bcClose);
   ecodes.writeByte(bcQuit);

   dump[procPtr - 4] = ecodes.Position() - procPtr;
}

bool Instance :: loadTemplate(ident_t name)
{
   Path path(_machine->templates.get(name));

   if (!_config.load(path.c_str(), &_machine->templates)) {
      setStatus("Cannot load the template:", name);

      return false;
   }

   return initLoader(_config);
}

void Instance :: setPackagePath(ident_t package, path_t path)
{
   _loader.setNamespace(package, path);
}

void Instance :: setPackagePath(ident_t line)
{
   size_t sep = line.find('=', -1);
   if (sep != -1) {
      Path path(line + sep + 1);
      IdentifierString package(line, sep);

      setPackagePath(package, path.c_str());
   }
   else {
      Path path(line);

      setPackagePath(NULL, path.c_str());
   }
}

void Instance :: addPackagePath(ident_t package, ident_t path)
{
   _loader.addPackage(package, path);
}

void Instance :: addPackagePath(ident_t line)
{
   size_t sep = line.find('=', -1);
   if(sep != -1) {
      IdentifierString path(line + sep + 1);
      IdentifierString package(line, sep);

      addPackagePath(package, path);
   }
   else addPackagePath(NULL, line);
}

void Instance :: configurate(SystemEnv* env, void* sehTable, MemoryReader& reader, int terminator)
{
   size_t pos = reader.Position();

   size_t command = reader.getDWord();
   while (command != terminator) {
      ident_t arg = NULL;
      size_t param = reader.getDWord();
      if (test(command, LITERAL_ARG_MASK)) {
         arg = (const char*)reader.Address();

         reader.seek(reader.Position() + param);  // goes to the next record
      }

      switch (command) {
         case USE_VM_MESSAGE_ID:
            if (emptystr(_loader.getNamespace())) {
               setPackagePath(arg);
            }
            else addPackagePath(arg);
            break;
         case MAP_VM_MESSAGE_ID:
            addForward(arg);
            break;
         case LOAD_VM_MESSAGE_ID:
            if(!loadTemplate(arg))
               throw EAbortException();

            break;
         case OPEN_VM_CONSOLE:
            if (_debugMode)
               createConsole();
            break;
         case START_VM_MESSAGE_ID:
            createConsole();

            if(!restart(env, sehTable, _debugMode))
               throw EAbortException();

            break;
         default:
            reader.seek(pos);
            return;
      }

      pos = reader.Position();
      command = reader.getDWord();
   }
   reader.seek(pos);
}

int Instance :: interprete(SystemEnv* env, void* sehTable, void* tape, bool standAlone)
{
   ByteArray    tapeArray(tape, -1);
   MemoryReader tapeReader(&tapeArray);

   stopVM();

   // configurate VM instance
   configurate(env, sehTable, tapeReader, 0);

   if (!_initialized)
      throw InternalError("ELENAVM is not initialized");

   // exit if nothing to interprete
   if (tapeArray[tapeReader.Position()] == 0)
      return -1;

   //if (_debugMode) {
   //   // remove subject list from the debug section
   //   _Memory* debugSection = getTargetDebugSection();
   //   if ((*debugSection)[0] > 0)
   //      debugSection->trim((*debugSection)[0]);
   //}

   // !! probably, it is better to use jitlinker reference helper class
   ImageReferenceHelper helper(this);

   // create byte code tape
   MemoryDump   ecodes;
   translate(tapeReader, helper, ecodes, 0);

   // generate module initializers
   MemoryDump  initTape(0);
   _linker->generateInitTape(initTape);
   if (initTape.Length() > 0) {
      ecodes[0] = ecodes[0] + initTape.Length();
      ecodes.insert(4, initTape.get(0), initTape.Length());
   }      

   // compile byte code
   MemoryReader reader(&ecodes);

   void* vaddress = _linker->resolveTemporalByteCode(helper, reader, TAPE_SYMBOL, (void*)tape);

   // update debug section size if available
   if (_debugMode) {
      _Memory* debugSection = getTargetDebugSection();

      (*debugSection)[0] = debugSection->Length();

      //// add subject list to the debug section
      //_ELENA_::MemoryWriter debugWriter(debugSection);
      //saveActionNames(&debugWriter);
   }

   onNewCode(env);

   resumeVM();

   // raise an exception to warn debugger
   if (_debugMode) {
      raiseBreakpoint();
   }      

   _Entry entry;
   entry.address = vaddress;

   int retVal = 0;
   if (!standAlone) {
      retVal = __routineProvider.ExecuteInFrame(env, entry);
   }
   else retVal = __routineProvider.ExecuteInNewFrame(env, entry);
   //else retVal = (*entry.entry)();

   if (retVal == 0)
      setStatus("Broken");
      
   return retVal;
}

bool Instance :: loadAddressInfo(void* address, char* buffer, size_t& maxLength)
{
   RTManager manager;
   MemoryReader reader(getTargetDebugSection(), 8u);
   reader.getLiteral(DEFAULT_STR);

   maxLength = manager.readAddressInfo(reader, (size_t)address, &_loader, buffer, maxLength);

   return maxLength > 0;
}

void* Instance :: parseMessage(ident_t message)
{
   IdentifierString messageName;
   size_t subject = 0;
   size_t param = 0;
   int paramCount = -1;
   for (size_t i = 0; i < getlength(message); i++) {
      if (message[i] == '[') {
         if (message[getlength(message) - 1] == ']') {
            messageName.copy(message + i + 1, getlength(message) - i - 2);
            paramCount = messageName.ident().toInt();
            if (paramCount > ARG_COUNT)
               return nullptr;
         }
         else return nullptr;

         param = i;
      }
      else if (message[i] >= 65 || (message[i] >= 48 && message[i] <= 57)) {
      }
      else if (message[i] == ']' && i == (getlength(message) - 1)) {
      }
      else return nullptr;
   }

   ref_t flags = 0;

   if (param != 0) {
      messageName.copy(message + subject, param - subject);
   }
   else messageName.copy(message + subject);

   ref_t actionRef = getSubjectRef(messageName.ident());
   if (!actionRef)
      return nullptr;

   return (void*)(encodeMessage(actionRef, paramCount, flags));
}

// --- ELENAMachine::Config ---

bool ELENAVMMachine::Config :: load(path_t path, Templates* templates)
{
   XmlConfigFile config;
   _ELENA_::Path rootPath;

   rootPath.copySubPath(path);

   if (_ELENA_::emptystr(path) || !config.load(path, feUTF8)) {
      return false;
   }

   // load templates
   if (templates) {
      loadList(config, TEMPLATE_CATEGORY, rootPath.c_str(), templates);
   }

   init(rootPath.c_str(), config);

   return true;
}

// --- ELENAVMMachine ---

ELENAVMMachine :: ELENAVMMachine(path_t rootPath)
   : templates(NULL, freestr), _rootPath(rootPath)
{
   Path configPath(rootPath, "elenavm.cfg");

   config.load(configPath.c_str(), &templates);
}

void ELENAVMMachine :: startSTA(ProgramHeader* frameHeader, SystemEnv* env, void* sehTable, void* tape)
{
   // setting up system
   __routineProvider.Prepare();
   __routineProvider.InitSTA((SystemEnv*)env, frameHeader);

   if (tape != nullptr) {
      // if it is a stand alone application
      _instance->interprete(env, sehTable, tape, true);

      // winding down system
      Exit(0);
   }
   // if it is part of the terminal - do nothing
}

void ELENAVMMachine :: Exit(int exitCode)
{
   __routineProvider.Exit(exitCode);
}
