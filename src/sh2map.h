#pragma once
#include "mycommon.h"

// stripped-down class for loading just the textures part
struct SH2MapHeader {
    uint32_t magic;         // should be 0x20010510
    uint32_t fileSize;
    uint32_t numFiles;
    uint32_t unknown;
};
static_assert(sizeof(SH2MapHeader) == 16);

struct SH2MapSubDataHeader {
    uint32_t subDataType;
    uint32_t subDataSize;
    uint32_t unknown_0;
    uint32_t unknown_1;
};
static_assert(sizeof(SH2MapSubDataHeader) == 16);


class SH2TextureContainer;

class SH2Map {
public:
    SH2Map();
    ~SH2Map();

    bool    LoadFromFile(const fs::path& path);
    bool    LoadFromStream(MemStream& stream);

    bool    SaveToFile(const fs::path& path);
    bool    SaveToStream(MemWriteStream& stream);

    RefPtr<SH2TextureContainer> GetTexturesContainer();

private:
    using SubData = std::pair<SH2MapSubDataHeader, void*>;

    SH2MapHeader                 mHeader;
    MyArray<SubData>             mSubDatas;
    RefPtr<SH2TextureContainer>  mTexturesContainer;
};
