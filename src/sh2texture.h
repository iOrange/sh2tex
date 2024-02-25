#pragma once
#include "mycommon.h"

struct SH2SpriteHeader {
    uint32_t id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t  format;
    uint8_t  isCompressed;      // pad byte on PS2  (== 0x50 for bloated palette !!!)
    uint16_t importance;        // ???, I guess streaming stuff

    uint32_t dataSize;
    uint32_t dataSize2;         // always dataSize + 16 (+ 128 on PS2)
    union {
        uint32_t unknown_0;     // always 0 on PC
        struct {
            uint8_t sendpsm;    // palette PSM
            uint8_t drawpsm;    // picture PSM
            uint8_t bitshift;   // ???
            uint8_t tagpoint;   // ???
        } ps2_specific;
    };
    uint8_t  bitw;
    uint8_t  bith;
    uint16_t marker;            // always 0x9900 (0x9999 on PS2)
};
static_assert(sizeof(SH2SpriteHeader) == 32);

struct SH2TextureHeader {
    uint32_t id;
    uint16_t width;
    uint16_t height;
    uint16_t width2;            // always same as width
    uint16_t height2;           // always same as height
    uint16_t numSprites;
    uint16_t unknown_0;
};
static_assert(sizeof(SH2TextureHeader) == 16);

struct SH2TextureHeader2 {      // ????
    uint32_t unknown_0;
    uint32_t unknown_1;
    uint32_t unknown_2;
    uint32_t unknown_3;
};
static_assert(sizeof(SH2TextureHeader2) == 16);

struct SH2TextureContainerHeader {
    uint32_t magic;
    uint32_t unknown_0;
    uint32_t unknown_1;
    uint32_t unknown_2;
};
static_assert(sizeof(SH2TextureContainerHeader) == 16);

struct SH2TextureContainerHeader_PS2 {
    uint32_t magic;
    uint32_t headerSize;
    uint32_t headerAndDataSize;
    uint32_t marker;            // 0xA7A7A7A7
    uint32_t unknown[12];
};
static_assert(sizeof(SH2TextureContainerHeader_PS2) == 64);

struct SH2TexturePaletteHeader_SH2 {
    uint32_t paletteDataSize;
    uint32_t unknown_0;         // = paletteDataSize, or 0 if bloated
    uint32_t unknown_1;         // = paletteDataSize, or 0 if bloated
    uint16_t palettesCount;     // 1024 * bloatFactor == paletteDataSize
    uint8_t  numColors;         // per block
    uint8_t  readSize;          // useful block part
    uint32_t unknown[8];        // always zeroes
};
static_assert(sizeof(SH2TexturePaletteHeader_SH2) == 48);

class SH2Texture {
public:
    enum class Format {
        DXT1,
        DXT2,
        DXT3,
        DXT4,
        DXT5,           // 4bit paletted for PS2
        Paletted = 8,
        RGBX8 = 24,
        RGBA8 = 32,

        Paletted4 = 4,  // PS2 only
    };

    SH2Texture();
    ~SH2Texture();

    bool                        LoadFromFile(const fs::path& path);
    bool                        LoadFromStream(MemStream& stream);

    bool                        LoadFromStream_PS2(MemStream& stream);

    bool                        SaveToFile(const fs::path& path);
    bool                        SaveToStream(MemWriteStream& stream);

    bool                        SaveToStream_PS2(MemWriteStream& stream);

    uint32_t                    GetID() const;
    uint32_t                    GetWidth() const;
    uint32_t                    GetHeight() const;
    Format                      GetFormat() const;
    const uint8_t*              GetData() const;
    const uint8_t*              GetPalette() const;

    // PS2 specific palette funcs
    size_t                      GetPalettesCount() const;
    size_t                      GetCurrentPaletteIdx() const;
    void                        SetCurrentPaletteIdx(const size_t idx);
    void                        ImportPalette();

    bool                        IsCompressed() const;
    bool                        IsPremultiplied() const;
    bool                        IsPS2File() const;

    uint32_t                    GetOriginalDataSize() const;
    uint32_t                    CalculateDataSize() const;

    void                        Replace(const Format format, const uint32_t width, const uint32_t height, const uint8_t* data, const uint8_t* palette = nullptr);
    bool                        Replace_PS2(const uint8_t* data, const uint8_t* palette);

    const StringArray&          GetErrors() const;
    const StringArray&          GetWarnings() const;

private:
    SH2TextureHeader            mHeader;
    SH2TextureHeader2           mHeader2;
    MyArray<SH2SpriteHeader>    mSprites;
    Format                      mFormat;            // cached from sprite that has data
    uint32_t                    mOriginalDataSize;  // cached from sprite that has data
    BytesArray                  mData;
    BytesArray                  mPalette;

    StringArray                 mErrors;
    StringArray                 mWarnings;

    // PS2 stuff
    bool                        mIsPS2File;
    SH2SpriteHeader             mHeader_PS2;
    SH2TexturePaletteHeader_SH2 mPaletteHeader_PS2;
    BytesArray                  mPalettePS2;        // this will hold all the PS2 palette bytes
    size_t                      mPaletteIdx;        // PS2 only
};

class SH2TextureContainer {
public:
    SH2TextureContainer();
    ~SH2TextureContainer();

    bool                            LoadFromFile(const fs::path& path);
    bool                            LoadFromStream(MemStream& stream);
    bool                            LoadFromStream_PS2(MemStream& stream);

    bool                            SaveToFile(const fs::path& path);
    bool                            SaveToStream(MemWriteStream& stream);
    bool                            SaveToStream_PS2(MemWriteStream& stream);

    size_t                          GetNumTextures() const;
    SH2Texture*                     GetTexture(const size_t idx);

    void                            SetVirtual(const bool isVirtual);
    void                            AddTexture(SH2Texture* texture);

    const StringArray&              GetErrors() const;
    const StringArray&              GetWarnings() const;

    bool                            IsPS2File() const;

private:
    SH2TextureContainerHeader       mHeader;
    MyArray<SH2Texture*>            mTextures;
    StringArray                     mErrors;
    StringArray                     mWarnings;

    // PS2 stuff
    bool                            mIsPS2File;
    bool                            mHasPS2Header;
    SH2TextureContainerHeader_PS2   mHeader_PS2;

    // mostly for XBOX can can be any platform
    bool                            mHasTrailingHeader;
    SH2TextureHeader                mTrailingHeader;

    // virtual container (sh2tex)
    bool                            mIsVirtual;
};
