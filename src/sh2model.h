#pragma once
#include "mycommon.h"

struct SH2MDLContainerHeader {
    uint32_t    noTex;
    uint32_t    id;
    uint32_t    numTextures;
    uint32_t    texturesOffset;     // inline TBN2
    uint32_t    stub[12];
};
static_assert(sizeof(SH2MDLContainerHeader) == 64);

class SH2TextureContainer;

class SH2Model {
public:
    SH2Model();
    ~SH2Model();

    bool    LoadFromFile(const fs::path& path);
    bool    LoadFromStream(MemStream& stream);

    bool    SaveToFile(const fs::path& path);
    bool    SaveToStream(MemWriteStream& stream);

    RefPtr<SH2TextureContainer> GetTexturesContainer();

private:
    SH2MDLContainerHeader        mHeader;
    MyArray<uint8_t>             mGeometryData;
    RefPtr<SH2TextureContainer>  mTexturesContainer;
};
