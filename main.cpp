#include <iostream>
#include "ObElfReader.h"
#include "ElfRebuilder.h"
#include "FDebug.h"
#include <getopt.h>
#include <stdio.h>

#ifdef __SO64__
#define TARGET_NAME "SoFixer64"
#else
#define TARGET_NAME "SoFixer32"
#endif


const char* short_options = "hdm:s:o:b:a:";
const struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"memso", 1, NULL, 'm'},
        {"source", 1, NULL, 's'},
        {"baseso", 1, NULL, 'b'},
        {"output", 1, NULL, 'o'},
        {"align", 1, NULL, 'a'},
        {nullptr, 0, nullptr, 0}
};
void useage();

auto is16Bit = [](const char* c) {
    auto len = strlen(c);
    if(len > 2) {
        if(c[0] == '0' & c[1] == 'x') return true;
    }
    bool is10bit = true;
    for(auto i = 0; i < len; i++) {
        if((c[i] > 'a' && c[i] < 'f') ||
            (c[i] > 'A' && c[i] < 'F')) {
            is10bit = false;
        }
    }
    return !is10bit;
};

                


bool main_loop(int argc, char* argv[]) {
    int c;

    ObElfReader elf_reader;

    std::string source, output, baseso;
    size_t page_size = -1;
    while((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (c) {
            case 'd':
                FLOGI("Use debug mode");
                break;
            case 's':
                source = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'b':
                baseso = optarg;
                break;
            case 'a': {
                    #ifndef __SO64__
                        page_size = strtoul(optarg, 0, is16Bit(optarg) ? 16: 10);
                    #else
                        page_size = strtoull(optarg, 0, is16Bit(optarg) ? 16: 10);
                    #endif
                }
                break;
            case 'm': {
                    #ifndef __SO64__
                        auto base = strtoul(optarg, 0, is16Bit(optarg) ? 16: 10);
                    #else
                        auto base = strtoull(optarg, 0, is16Bit(optarg) ? 16: 10);
                    #endif
                        elf_reader.setDumpSoBaseAddr(base);
                }
                break;

            default:
                return false;
        }
    }

    auto file = fopen(source.c_str(), "rb");
    if(nullptr == file) {
        FLOGE("source so file cannot found!!!");
        return false;
    }
#ifdef __LARGE64_FILES
    auto fd = file->_file;
#else
    auto fd = fileno(file);
#endif

    FLOGI("start to rebuild elf file");
    if (!elf_reader.setSource(source.c_str())) {
        FLOGE("unable to open source file");
        return false;
    }
    if (!baseso.empty()) {
        elf_reader.setBaseSoName(baseso.c_str());
    }

    if (page_size == -1) {
        FLOGE("page size value is not found");
        return false;
    }

    if(!elf_reader.Load(page_size)) {
        FLOGE("source so file is invalid");
        return false;
    }

    ElfRebuilder elf_rebuilder(&elf_reader);
    if(!elf_rebuilder.Rebuild(page_size)) {
        FLOGE("error occured in rebuilding elf file");
        return false;
    }
    fclose(file);

    if (!output.empty()) {
        file = fopen(output.c_str(), "wb+");
        if(nullptr == file) {
            FLOGE("output so file cannot write !!!");
            return false;
        }
        fwrite(elf_rebuilder.getRebuildData(), 1, elf_rebuilder.getRebuildSize(),  file);
        fclose(file);
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (main_loop(argc, argv)) {
        FLOGI("Done!!!");
        return 0;
    }
    useage();
    return -1;
}

void useage() {
    FLOGI(TARGET_NAME "v2.1 author F8LEFT(currwin)");
    FLOGI("Useage: SoFixer <option(s)> -s sourcefile -o generatefile");
    FLOGI(" try rebuild shdr with phdr");
    FLOGI(" Options are:");

    FLOGI("  -d --debug                                 Show debug info");
    FLOGI("  -m --memso memBaseAddr(16bit format)       the memory address x which the source so is dump from");
    FLOGI("  -s --source sourceFilePath                 Source file path");
    FLOGI("  -b --baseso baseFilePath                   Original so file path.(used to get base information)(experimental)");
    FLOGI("  -o --output generateFilePath               Generate file path");
    FLOGI("  -h --help                                  Display this information");
    FLOGI("  -a --align                                 Set the page size for alignment");
}
