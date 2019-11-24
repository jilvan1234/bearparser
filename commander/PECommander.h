#pragma once

#include "ExeCommander.h"

namespace cmd_util {
    PEFile* getPEFromContext(CmdContext *ctx);
    void printSectionMapping(SectionHdrWrapper *sec, Executable::addr_type aType);
    void printResourceTypes(PEFile *pe);
    void printStrings(PEFile *pe, size_t limit);
    void dumpResourcesInfo(PEFile *pe, pe::resource_type type, size_t wrapperId);
    void listDataDirs(PEFile *pe);
};

//----------------------------------------------

class PECommander : public ExeCommander
{
public:
    PECommander(ExeCmdContext *v_context)
        : ExeCommander(v_context)
    {
        initCommands();
    }

protected:
    virtual void initCommands();
};

class SectionByAddrCommand : public Command
{
public:
    SectionByAddrCommand(Executable::addr_type v_addrType, std::string desc)
        : Command(desc), addrType(v_addrType) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *peExe = cmd_util::getPEFromContext(context);
        if (!peExe) return;

        offset_t offset = cmd_util::readOffset(addrType);

        SectionHdrWrapper* sec = peExe->getSecHdrAtOffset(offset, addrType, true, true);
        if (sec == NULL) {
            printf("NOT found addr [0x%llX] in any section!\n", 
                static_cast<unsigned long long>(offset)
            );
            printf("----------------------------\n");
            return;
        }
        offset_t delta = offset - sec->getContentOffset(addrType);
        printf("Found addr [0x%llX] in section:\n",
            static_cast<unsigned long long>(offset)
        );
        printf("F: %8llX\n",
            static_cast<unsigned long long>(offset)
        );
        printf("offset from the sec. bgn: %8llX\n",
            static_cast<unsigned long long>(delta)
        );
        printf ("V: %8llX - %8llX (%llX)\n",
            static_cast<unsigned long long>(sec->getContentOffset(Executable::RVA)),
            static_cast<unsigned long long>(sec->getContentEndOffset(Executable::RVA, false)),
            static_cast<unsigned long long>(sec->getContentEndOffset(Executable::RVA, true))
        );
        printf ("R: %8llX - %8llX (%llX)\n",
            static_cast<unsigned long long>(sec->getContentOffset(Executable::RAW)),
            static_cast<unsigned long long>(sec->getContentEndOffset(Executable::RAW, false)),
            static_cast<unsigned long long>(sec->getContentEndOffset(Executable::RAW, true))
        );

        cmd_util::dumpEntryInfo(sec);
        printf("----------------------------\n");
    }

protected:
    Executable::addr_type addrType;
};

class PrintStringsCommand : public Command
{
public:
    PrintStringsCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *pe = cmd_util::getPEFromContext(context);
        if (!pe) return;

        ResourcesContainer* container = pe->getResourcesOfType(pe::RESTYPE_STRING);
        if (container == NULL) {
            std::cout << "No such resource type!" << std::endl;
            return;
        }
        size_t max = container->entriesCount();
        std::cout << "Total: " << max << std::endl;
        size_t limit = 0;
        if (max > 100) {
            limit = cmd_util::readNumber("max");
        }

       cmd_util::printStrings(pe, limit);
    }
protected:
    int wrapperId; //TODO: fetch it from params!
};

class PrintWrapperTypesCommand : public Command
{
public:
    PrintWrapperTypesCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *pe = cmd_util::getPEFromContext(context);
        if (!pe) return;
        ResourcesAlbum *album = pe->getResourcesAlbum();
        if (!album) return;

        cmd_util::printResourceTypes(pe);
    }
};

class WrapperInfoCommand : public Command
{
public:
    WrapperInfoCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *pe = cmd_util::getPEFromContext(context);
        if (!pe) return;

        ResourcesAlbum *album = pe->getResourcesAlbum();
        if (album == NULL) return;

        size_t dirsCount =  album->dirsCount();
        if (dirsCount == 0) {
            std::cout << "No resources!" << std::endl;
            return;
        }
        cmd_util::printResourceTypes(pe);

        pe::resource_type type = (pe::resource_type) cmd_util::readNumber("wrapper type");

        ResourcesContainer* wrappers = pe->getResourcesOfType(type);
        if (wrappers == NULL) {
            std::cout << "No such resource type!" << std::endl;
            return;
        }
        size_t wrappersCount = wrappers->count();
        if (wrappersCount == 0) {
            return;
        }
        size_t wrapperIndx = 0;
        if (wrappersCount > 1) {
            std::cout << "Wrappers count: " << std::dec << wrappersCount << std::endl;
            wrapperIndx = cmd_util::readNumber("wrapperIndex");
        }
        cmd_util::dumpResourcesInfo(pe, type, wrapperIndx);
    }
};

class MoveDataDirEntryCommand : public Command
{
public:
    MoveDataDirEntryCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *pe = cmd_util::getPEFromContext(context);
        if (!pe) return;

        printf("Available DataDirs: \n");
        cmd_util::listDataDirs(pe);

        pe::dir_entry entryId = static_cast<pe::dir_entry> (cmd_util::readNumber("DataDir id"));
        if (pe->getDataDirEntry(entryId) == NULL) {
            printf("No such wrapper\n");
            return;
        }

        offset_t offset = cmd_util::readOffset(Executable::RAW);
        try {
            if (pe->moveDataDirEntry(entryId, offset) == false) {
                printf("Failed\n");
                return;
            }
            printf("Done!\n");
        } catch (CustomException &e){
            std::cerr << "[ERROR] "<< e.what() << std::endl;
        }
    }
};

class SectionDumpCommand : public Command
{
public:
    SectionDumpCommand(std::string desc, bool v_saveToFile = false)
        : Command(desc), saveToFile(v_saveToFile)
    {
    }

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *pe = cmd_util::getPEFromContext(context);
        if (!pe) return;

        size_t sectHdrCount = pe->getSectionsCount(false);
        size_t sectCount = pe->getSectionsCount(true);
        printf("Sections count = %lu\n",
            static_cast<unsigned long>(sectCount)
        );
        if (sectCount == 0) {
            printf("No sections!\n");
            return;
        }
        printf("Available indexes: %lu-%lu\n", 0UL, static_cast<unsigned long>(sectCount - 1));
        size_t secId = cmd_util::readNumber("Chose the section by index");

        SectionHdrWrapper *sec = pe->getSecHdr(secId);
        if (sec == NULL) {
            printf("No such section\n");
            return;
        }
        Executable::addr_type aType = Executable::RAW;
        offset_t start = sec->getContentOffset(aType, true);
        bufsize_t size = sec->getContentSize(aType, true);

        printf("Section %s\n", sec->getName().toStdString().c_str());
        cmd_util::printSectionMapping(sec, Executable::RAW);
        cmd_util::printSectionMapping(sec, Executable::RVA);

        if (saveToFile) {
            BufferView *secView = pe->createSectionView(secId);
            if (secView == NULL) return;
            
            QString fileName = makeFileName(secId);

            bufsize_t dSize = FileBuffer::dump(fileName, *secView, true);
            printf("Dumped size: %lu into: %s\n",
                static_cast<unsigned long>(dSize),
                fileName.toStdString().c_str()
            );
            delete secView;
        }
    }

protected:
    QString makeFileName(const size_t secId) 
    {
        std::stringstream sstr;
        sstr << "section" << "_" << std::hex << secId << ".bin";
        return QString(sstr.str().c_str());
    }
    
    bool saveToFile;
};

class ExportsListCommand : public Command
{
public:
    ExportsListCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *peExe = cmd_util::getPEFromContext(context);
        if (!peExe) return;
                
        ExportDirWrapper* exports = dynamic_cast<ExportDirWrapper*>(peExe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_EXPORT));
        if (!exports) return;
        
        char* libName = exports->getLibraryName();
        if (libName) {
            std::cout << "Lib Name: " << libName << "\n";
        }
        const size_t entriesCnt = exports->getEntriesCount();
        std::cout << "Entries:  " << entriesCnt << "\n";
        if (entriesCnt == 0) return;
        
        for(int i = 0; i < entriesCnt; i++) {
            ExportEntryWrapper* entry = dynamic_cast<ExportEntryWrapper*>(exports->getEntryAt(i));
            if (!entry) continue;

            QString forwarder = entry->getForwarderStr();
            std::cout << std::hex << entry->getFuncRva() << " : " << entry->getName().toStdString();
            if (forwarder.length()) {
                std::cout << " : " << forwarder.toStdString();
            }
            std::cout <<  "\n";
        }
    }
};

class ImportsListCommand : public Command
{
public:
    ImportsListCommand(std::string desc)
        : Command(desc) {}

    virtual void execute(CmdParams *params, CmdContext  *context)
    {
        PEFile *peExe = cmd_util::getPEFromContext(context);
        if (!peExe) return;
                
       ImportDirWrapper* imports = dynamic_cast<ImportDirWrapper*>(peExe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
        if (!imports) return;
        
        const size_t librariesCount = imports->getEntriesCount();
        std::cout << "Libraries Count: " << librariesCount << "\n";
        
        QList<offset_t> thunks = imports->getThunksList();
        const size_t functionsCount = thunks.size();
        std::cout << "Functions Count: " << functionsCount << "\n";
        
        for (int i = 0; i < thunks.size(); i++) {
            offset_t thunk = thunks[i];
            if (thunk == 0 || thunk == INVALID_ADDR) continue;

            QString lib =  imports->thunkToLibName(thunk);
            QString func = imports->thunkToFuncName(thunk);
            std::cout << std::hex << thunk << " : " << lib.toStdString() << "." << func.toStdString() << "\n";
        }
    }
};