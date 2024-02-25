#include "sh2texture.h"
#include "libs/bcdec/bcdec.h" // no implementation, just size helpers
#include <fstream>

#define FIX_WRONG_DATASIZE 0

constexpr uint16_t kSpriteMarker = 0x9900;
constexpr uint32_t kTextureContainerMagic = 0x19990901;

constexpr uint32_t kTextureContainerMarker_PS2 = 0xA7A7A7A7;
constexpr uint16_t kSpriteMarker_PS2 = 0x9999;

// PS2 specific info and PSM values - https://openkh.dev/common/tm2.html
enum PS2_PSM {
    PSMCT32  = 0,       // RGBA32, uses 32 - bit per pixel.
    PSMCT24  = 1,       // RGB24, uses 24 - bit per pixel with the upper 8 bit unused.
    PSMCT16  = 2,       // RGBA16 unsigned, pack two pixels in 32 - bit in little endian order.
    PSMCT16S = 10,      // RGBA16 signed, pack two pixels in 32 - bit in little endian order.
    PSMT8    = 19,      // 8 - bit indexed, packing 4 pixels per 32 - bit.
    PSMT4    = 20,      // 4 - bit indexed, packing 8 pixels per 32 - bit.
    PSMT8H   = 27,      // 8 - bit indexed, but the upper 24 - bit are unused.
    PSMT4HL  = 26,      // 4 - bit indexed, but the upper 24 - bit are unused.
    PSMT4HH  = 44,      // 4 - bit indexed, where the bits 4 - 7 are evaluated and the rest discarded.
};

static uint8_t FromPS2Alpha(const uint8_t ps2Alpha) {
    return (ps2Alpha == 0x80) ? 0xFF : ((ps2Alpha << 1) | (ps2Alpha & 1));
}

static uint8_t ToPS2Alpha(const uint8_t alpha) {
    return (alpha == 0xFF) ? 0x80 : (alpha >> 1);
}

static void FromPS2Palette(uint8_t* palette) {
    uint32_t* palette32 = rcast<uint32_t*>(palette);
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            std::swap(palette32[8 + i * 32 + j], palette32[16 + i * 32 + j]);
        }
    }

    for (size_t i = 0; i < 1024; i += 4) {
        std::swap(palette[i + 0], palette[i + 2]);
        // weird PS2 alpha
        palette[i + 3] = FromPS2Alpha(palette[i + 3]);
    }
}

static void ToPS2Palette(uint8_t* palette) {
    for (size_t i = 0; i < 1024; i += 4) {
        std::swap(palette[i + 0], palette[i + 2]);
        // weird PS2 alpha
        palette[i + 3] = ToPS2Alpha(palette[i + 3]);
    }

    uint32_t* palette32 = rcast<uint32_t*>(palette);
    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            std::swap(palette32[8 + i * 32 + j], palette32[16 + i * 32 + j]);
        }
    }
}


#define merror(str)     mErrors.push_back(str)
#define mwarning(str)   mWarnings.push_back(str)

SH2Texture::SH2Texture()
    : mHeader{}
    , mHeader2{}
    , mFormat{}
    , mOriginalDataSize{0u}
    // PS2 stuff
    , mIsPS2File(false)
    , mHeader_PS2{}
    , mPaletteHeader_PS2{}
    , mPaletteIdx(0)
{
}
SH2Texture::~SH2Texture() {
}

bool SH2Texture::LoadFromFile(const fs::path& path) {
    return false;
}

bool SH2Texture::LoadFromStream(MemStream& stream) {
    // trying to detect PS2 file
    stream.ReadStruct(mHeader_PS2);
    stream.RewindBytes(sizeof(mHeader_PS2));

    if (mHeader_PS2.marker == kSpriteMarker_PS2) {
        return this->LoadFromStream_PS2(stream);
    }

    stream.ReadStruct(mHeader);
    stream.ReadStruct(mHeader2);

    mSprites.resize(mHeader.numSprites);
    for (auto& sprHdr : mSprites) {
        stream.ReadStruct(sprHdr);

        if (sprHdr.marker != kSpriteMarker) {
            merror("Couldn't locate sprite end marker!");
            return false;
        }

        if (sprHdr.dataSize > 0) {
            mFormat = scast<SH2Texture::Format>(sprHdr.format);

            mOriginalDataSize = sprHdr.dataSize;
            const uint32_t expectedDataSize = this->CalculateDataSize();
#if FIX_WRONG_DATASIZE
            if (expectedDataSize != sprHdr.dataSize) {
                // a hack to detect possibly hand-edited textures (sh2ee has them)
                if (stream.Remains() >= expectedDataSize) {
                    sprHdr.dataSize = expectedDataSize;
                    sprHdr.dataSize2 = expectedDataSize + 16;
                    mwarning("Texture data size is wrong,\ntrying to read texture using proper guessed data size.\nIf texture looks okay, consider re-saving\nto fix data structure!");
                } else {
                    merror("Texture data size is wrong!");
                    return false;
                }
            }
#endif

            mData.resize(expectedDataSize);
            stream.ReadToBuffer(mData.data(), mData.size());

            if (mFormat == SH2Texture::Format::Paletted) {
                SH2SpriteHeader paletteHeader; // WHY ????
                stream.ReadStruct(paletteHeader);

                assert(paletteHeader.id == sprHdr.id);
                assert(paletteHeader.dataSize == 1024);

                mPalette.resize(256 * 4);
                stream.ReadToBuffer(mPalette.data(), mPalette.size());
            }
        }
    }

    return true;
}

// swizzling info - https://ps2linux.no-ip.info/playstation2-linux.com/download/ezswizzle/TextureSwizzling.pdf
extern void writeTexPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);
extern void readTexPSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);
extern void readTexPSMT4(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);

// https://youtu.be/LbcZCEAN1nY

bool SH2Texture::LoadFromStream_PS2(MemStream& stream) {
    mIsPS2File = true;

    const size_t startOffset = stream.GetCursor();

    stream.ReadStruct(mHeader_PS2);

    mFormat = scast<SH2Texture::Format>(mHeader_PS2.format);

    mOriginalDataSize = mHeader_PS2.dataSize;
    const uint32_t expectedDataSize = this->CalculateDataSize();

    const size_t pixelsOffset = mHeader_PS2.dataSize2 - mHeader_PS2.dataSize;

    const bool bloatedPalette = (mHeader_PS2.isCompressed != 0);

    stream.SetCursor(startOffset + pixelsOffset);
    mData.resize(expectedDataSize);
    stream.ReadToBuffer(mData.data(), mData.size());

    if (mFormat == Format::Paletted || mFormat == Format::Paletted4) {
        stream.ReadStruct(mPaletteHeader_PS2);

        mPalette.resize(256 * 4);

        mPalettePS2.resize(mPaletteHeader_PS2.paletteDataSize);
        stream.ReadToBuffer(mPalettePS2.data(), mPalettePS2.size());

        const int ww = this->GetWidth();
        const int hh = this->GetHeight();
        const int rrw = ww >> 1;
        const int rrh = (mFormat == Format::Paletted) ? (hh >> 1) : (hh >> 2);

        writeTexPSMCT32(0, rrw >> 6, 0, 0, rrw, rrh, mData.data());
        if (mFormat == Format::Paletted) {
            readTexPSMT8(0, ww >> 6, 0, 0, ww, this->GetHeight(), mData.data());
        } else {
            readTexPSMT4(0, ww >> 6, 0, 0, ww, this->GetHeight(), mData.data());

            // "eplode" 4bit image to 8bit
            const size_t bitsPerLine = mHeader_PS2.width * 4;
            const size_t bytesPerLine = (bitsPerLine >> 3) + ((bitsPerLine % 8 == 0) ? 0 : 1);

            BytesArray exploded(ww * hh);
            const uint8_t* src = mData.data();
            uint8_t* dst = exploded.data();
            for (size_t y = 0; y < mHeader_PS2.height; ++y) {
                const uint8_t* ptr4 = src + y * bytesPerLine;
                for (size_t x = 0; x < (mHeader_PS2.width - 1); x += 2) {
                    dst[0] = (ptr4[0] & 0xF);
                    dst[1] = ((ptr4[0] >> 4) & 0xF);

                    ptr4 += 1;
                    dst += 2;
                }

                if (mHeader_PS2.width & 1) {
                    dst[0] = (ptr4[0] & 0xF);

                    ++ptr4;
                    ++dst;
                }
            }

            mData = exploded;
        }

        mPaletteIdx = ~size_t(0);
        this->SetCurrentPaletteIdx(0);
    } else {
        for (size_t i = 0; i < mData.size(); i += 4) {
            std::swap(mData[i + 0], mData[i + 2]);
            // weird PS2 alpha
            mData[i + 3] = FromPS2Alpha(mData[i + 3]);
        }
    }

    return true;
}

bool SH2Texture::SaveToFile(const fs::path& path) {
    MemWriteStream stream;
    if (!this->SaveToStream(stream)) {
        return false;
    }

    std::ofstream file(path, std::ios_base::binary);
    if (!file.good()) {
        return false;
    }

    file.write(rcast<const char*>(stream.Data()), stream.GetWrittenBytesCount());
    file.flush();
    file.close();

    return true;
}

bool SH2Texture::SaveToStream(MemWriteStream& stream) {
    stream.Write(mHeader);
    stream.Write(mHeader2);

    for (auto& sprHdr : mSprites) {
        stream.Write(sprHdr);

        if (sprHdr.dataSize > 0) {
            stream.Write(mData.data(), mData.size());

            if (mFormat == SH2Texture::Format::Paletted) {
                SH2SpriteHeader paletteHeader = {}; // WHY ????
                paletteHeader.id = sprHdr.id;
                paletteHeader.x = 8;
                paletteHeader.dataSize = 1024;
                paletteHeader.dataSize2 = 1024 + 16;
                paletteHeader.marker = kSpriteMarker;
                stream.Write(paletteHeader);
                stream.Write(mPalette.data(), mPalette.size());
            }
        }
    }

    return true;
}

// swizzling info - https://ps2linux.no-ip.info/playstation2-linux.com/download/ezswizzle/TextureSwizzling.pdf
extern void readTexPSMCT32(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);
extern void writeTexPSMT8(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);
extern void writeTexPSMT4(int dbp, int dbw, int dsax, int dsay, int rrw, int rrh, void* data);

bool SH2Texture::SaveToStream_PS2(MemWriteStream& stream) {
    stream.Write(mHeader_PS2);

    const size_t totalHeaderSize = mHeader_PS2.dataSize2 - mHeader_PS2.dataSize;
    const size_t paddingSize = totalHeaderSize - sizeof(mHeader_PS2);
    stream.WriteDupByte(0, paddingSize);

    BytesArray ps2Image = mData;

    if (mFormat == Format::RGBX8) {
        for (size_t i = 0; i < ps2Image.size(); i += 4) {
            std::swap(ps2Image[i + 0], ps2Image[i + 2]);
            // weird PS2 alpha
            ps2Image[i + 3] = ToPS2Alpha(mData[i + 3]);
        }
    } else { // paletted
        const int ww = this->GetWidth();
        const int hh = this->GetHeight();
        const int rrw = ww >> 1;
        const int rrh = (mFormat == Format::Paletted) ? (hh >> 1) : (hh >> 2);

        if (mFormat == Format::Paletted) {
            writeTexPSMT8(0, ww >> 6, 0, 0, ww, this->GetHeight(), ps2Image.data());
        } else {
            writeTexPSMT4(0, ww >> 6, 0, 0, ww, this->GetHeight(), ps2Image.data());
        }
        readTexPSMCT32(0, rrw >> 6, 0, 0, rrw, rrh, ps2Image.data());
    }

    stream.Write(ps2Image.data(), ps2Image.size());

    if (mFormat == Format::Paletted) {
        stream.Write(mPaletteHeader_PS2);
        stream.Write(mPalettePS2.data(), mPalettePS2.size());
    }

    return true;
}

uint32_t SH2Texture::GetID() const {
    return mIsPS2File ? mHeader_PS2.id : mHeader.id;
}

uint32_t SH2Texture::GetWidth() const {
    return mIsPS2File ? mHeader_PS2.width : mHeader.width;
}

uint32_t SH2Texture::GetHeight() const {
    return mIsPS2File ? mHeader_PS2.height : mHeader.height;
}

SH2Texture::Format SH2Texture::GetFormat() const {
    return mFormat;
}

const uint8_t* SH2Texture::GetData() const {
    return mData.data();
}

const uint8_t* SH2Texture::GetPalette() const {
    return mPalette.data();
}

// PS2 specific palette funcs
size_t SH2Texture::GetPalettesCount() const {
    return mIsPS2File ? mPaletteHeader_PS2.palettesCount : 1;
}

size_t SH2Texture::GetCurrentPaletteIdx() const {
    return mIsPS2File ? mPaletteIdx : 0;
}

void SH2Texture::SetCurrentPaletteIdx(const size_t idx) {
    if (mIsPS2File && idx < this->GetPalettesCount()) {
        if (idx != mPaletteIdx) {
            mPaletteIdx = idx;

            const bool is4Bit = (this->GetFormat() == SH2Texture::Format::Paletted4);
            const size_t paletteEntries = is4Bit ? 16 : 256;
            const size_t paletteSize = paletteEntries * 4;
            const size_t paletteBlockSize = is4Bit ? 32 : 64;
            const size_t numPaletteBlocks = 256 / paletteBlockSize;
            const size_t numBlocksPerPalette = paletteSize / paletteBlockSize;

            if (mPalettePS2.size() == paletteSize) {
                std::memcpy(mPalette.data(), mPalettePS2.data(), mPalettePS2.size());
            } else {
                const size_t bigOffset = (idx / numPaletteBlocks) * 4096;
                const size_t smallOffset = (idx % numPaletteBlocks) * paletteBlockSize;

                uint8_t* dst = mPalette.data();
                const uint8_t* src = mPalettePS2.data() + bigOffset + smallOffset;
                for (size_t i = 0; i < numBlocksPerPalette; ++i) {
                    std::memcpy(dst, src, paletteBlockSize);
                    dst += paletteBlockSize;
                    src += 256;
                }
            }

            FromPS2Palette(mPalette.data());
        }
    }
}

void SH2Texture::ImportPalette() {
    BytesArray ps2Palette = mPalette;
    ToPS2Palette(ps2Palette.data());

    const bool is4Bit = (this->GetFormat() == SH2Texture::Format::Paletted4);
    const size_t paletteEntries = is4Bit ? 16 : 256;
    const size_t paletteSize = paletteEntries * 4;
    const size_t paletteBlockSize = is4Bit ? 32 : 64;
    const size_t numPaletteBlocks = 256 / paletteBlockSize;
    const size_t numBlocksPerPalette = paletteSize / paletteBlockSize;

    if (mPalettePS2.size() == paletteSize) {
        std::memcpy(mPalettePS2.data(), ps2Palette.data(), mPalettePS2.size());
    } else {
        const size_t bigOffset = (mPaletteIdx / numPaletteBlocks) * 4096;
        const size_t smallOffset = (mPaletteIdx % numPaletteBlocks) * paletteBlockSize;

        uint8_t* dst = mPalettePS2.data() + bigOffset + smallOffset;
        const uint8_t* src = mPalette.data();
        for (size_t i = 0; i < numBlocksPerPalette; ++i) {
            std::memcpy(dst, src, paletteBlockSize);
            dst += 256;
            src += paletteBlockSize;
        }
    }
}

bool SH2Texture::IsCompressed() const {
    return mIsPS2File == false &&
           ( mFormat == Format::DXT1 ||
             mFormat == Format::DXT2 ||
             mFormat == Format::DXT3 ||
             mFormat == Format::DXT4 ||
             mFormat == Format::DXT5  );
}

bool SH2Texture::IsPremultiplied() const {
    return mFormat == Format::DXT2 || mFormat == Format::DXT4;
}

bool SH2Texture::IsPS2File() const {
    return mIsPS2File;
}

uint32_t SH2Texture::GetOriginalDataSize() const {
    return mOriginalDataSize;
}

uint32_t SH2Texture::CalculateDataSize() const {
    const uint32_t width = this->GetWidth();
    const uint32_t height = this->GetHeight();

    if (!width || !height) {
        return 0u;
    }

    if (this->IsCompressed()) {
        return (mFormat == Format::DXT1) ? BCDEC_BC1_COMPRESSED_SIZE(width, height) : BCDEC_BC3_COMPRESSED_SIZE(width, height);
    } else if (mFormat == Format::Paletted) {
        return width * height;
    } else if (mFormat == Format::Paletted4) {
        const uint32_t bitsPerLine = width * 4;
        const uint32_t bytesPerLine = (bitsPerLine >> 3) + ((bitsPerLine % 8 == 0) ? 0 : 1);
        return bytesPerLine * height;
    } else /*if (mFormat == Format::RGBX8 || mFormat == Format::RGBA8)*/ {
        return width * height * 4;
    }
}

void SH2Texture::Replace(const SH2Texture::Format format, const uint32_t width, const uint32_t height, const uint8_t* data, const uint8_t* palette) {
    mFormat = format;
    mHeader.width = width;
    mHeader.width2 = width;
    mHeader.height = height;
    mHeader.height2 = height;

    const uint32_t dataSize = this->CalculateDataSize();

    for (auto& sprHdr : mSprites) {
        sprHdr.width = width;
        sprHdr.height = height;
        sprHdr.format = scast<uint8_t>(mFormat);
        sprHdr.isCompressed = scast<uint8_t>(this->IsCompressed());

#if FIX_WRONG_DATASIZE
        if (sprHdr.dataSize > 0) {
            sprHdr.dataSize = dataSize;
            sprHdr.dataSize2 = dataSize + 16;
        }
#endif
    }

    mData.resize(dataSize);
    std::memcpy(mData.data(), data, dataSize);

    if (mFormat == Format::Paletted && palette) {
        mPalette.resize(256 * 4);
        std::memcpy(mPalette.data(), palette, mPalette.size());
    } else {
        mPalette.clear();
    }
}

bool SH2Texture::Replace_PS2(const uint8_t* data, const uint8_t* palette) {
    const uint32_t width = this->GetWidth();
    const uint32_t height = this->GetHeight();

    if (mFormat == Format::RGBX8) {
        if (palette) {
            return false;
        }

        std::memcpy(mData.data(), data, width * height * 4);
    } else if (mFormat == Format::Paletted) {
        if (!palette) {
            return false;
        }

        std::memcpy(mData.data(), data, width * height);
        std::memcpy(mPalette.data(), palette, mPalette.size());

        this->ImportPalette();
    } else {
        return false;
    }

    return true;
}

const StringArray& SH2Texture::GetErrors() const {
    return mErrors;
}

const StringArray& SH2Texture::GetWarnings() const {
    return mWarnings;
}





SH2TextureContainer::SH2TextureContainer()
    : mHeader{}
    // PS2 stuff
    , mIsPS2File(false)
    , mHasPS2Header(false)
    , mHeader_PS2{}
    , mIsVirtual(false)
    // mostly for XBOX can can be any platform
    , mHasTrailingHeader(false)
    , mTrailingHeader{}
{
}

SH2TextureContainer::~SH2TextureContainer() {
    if (!mIsVirtual) {
        std::for_each(mTextures.begin(), mTextures.end(), [](SH2Texture* tex) {
            delete tex;
        });
    }
    mTextures.clear();
}

bool SH2TextureContainer::LoadFromFile(const fs::path& path) {
    std::ifstream file(path, std::ios_base::binary);
    if (!file.good()) {
        merror("Couldn't read file!");
        return false;
    }

    file.seekg(0, std::ios_base::end);
    const size_t fileSize = file.tellg();
    file.seekg(0, std::ios_base::beg);

    void* data = std::malloc(fileSize);
    file.read(rcast<char*>(data), fileSize);
    file.close();

    MemStream stream(data, fileSize, true);
    return this->LoadFromStream(stream);
}

bool SH2TextureContainer::LoadFromStream(MemStream& stream) {
    if (stream.Remains() < sizeof(SH2TextureContainerHeader)) {
        merror("Invalid texture container size!");
        return false;
    }

    stream.ReadStruct(mHeader);

    // trying to detect PS2 file
    // try #1: an actual container
    if (mHeader.unknown_2 == kTextureContainerMarker_PS2) {
        mIsPS2File = true;
        mHasPS2Header = true;
    }
    stream.RewindBytes(sizeof(mHeader));
    // try #2: just a texture (sprite), has a marker at the end of it's header
    if (!mIsPS2File) {
        SH2SpriteHeader testSpriteHeader = {};
        stream.ReadStruct(testSpriteHeader);
        mIsPS2File = testSpriteHeader.marker == kSpriteMarker_PS2;
        stream.RewindBytes(sizeof(testSpriteHeader));
    }

    if (mIsPS2File) {
        return this->LoadFromStream_PS2(stream);
    } else {
        stream.SkipBytes(sizeof(mHeader));
    }


    if (mHeader.magic != kTextureContainerMagic) {
        merror("Invalid texture container magic!");
        return false;
    }

    while (stream.Remains() >= sizeof(SH2TextureHeader)) {
        const uint32_t testId = stream.ReadU32();
        stream.RewindBytes(sizeof(testId));

        if (testId == 0u) {
            mHasTrailingHeader = true;
            stream.ReadStruct(mTrailingHeader);
        } else {
            SH2Texture* texture = new SH2Texture();
            if (!texture->LoadFromStream(stream)) {
                auto& errors = texture->GetErrors();
                auto& warnings = texture->GetWarnings();

                mErrors.insert(mErrors.end(), errors.begin(), errors.end());
                mWarnings.insert(mWarnings.end(), warnings.begin(), warnings.end());

                delete texture;
                return false;
            } else {
                mTextures.push_back(texture);

                auto& warnings = texture->GetWarnings();
                mWarnings.insert(mWarnings.end(), warnings.begin(), warnings.end());
            }
        }
    }

    return true;
}

bool SH2TextureContainer::LoadFromStream_PS2(MemStream& stream) {
    if (mHasPS2Header) {
        stream.ReadStruct(mHeader_PS2);
    }

    SH2Texture* texture = new SH2Texture();
    if (!texture->LoadFromStream(stream)) {
        auto& errors = texture->GetErrors();
        auto& warnings = texture->GetWarnings();

        mErrors.insert(mErrors.end(), errors.begin(), errors.end());
        mWarnings.insert(mWarnings.end(), warnings.begin(), warnings.end());

        delete texture;
        return false;
    } else {
        mTextures.push_back(texture);

        auto& warnings = texture->GetWarnings();
        mWarnings.insert(mWarnings.end(), warnings.begin(), warnings.end());
    }

    return true;
}

bool SH2TextureContainer::SaveToFile(const fs::path& path) {
    MemWriteStream stream;

    const bool ok = mIsPS2File ? this->SaveToStream_PS2(stream) : this->SaveToStream(stream);
    if (!ok) {
        return false;
    }

    std::ofstream file(path, std::ios_base::binary);
    if (!file.good()) {
        return false;
    }

    file.write(rcast<const char*>(stream.Data()), stream.GetWrittenBytesCount());
    file.flush();
    file.close();

    return true;
}

bool SH2TextureContainer::SaveToStream(MemWriteStream& stream) {
    stream.Write(mHeader);

    for (auto texture : mTextures) {
        if (!texture->SaveToStream(stream)) {
            return false;
        }
    }

    if (mTextures.empty() || mTextures.size() > 1 || mHasTrailingHeader) {
        // looks like TBN2 containers always have 16 tail zeroes
        stream.Write(mTrailingHeader);
    }

    return true;
}

bool SH2TextureContainer::SaveToStream_PS2(MemWriteStream& stream) {
    if (mHasPS2Header) {
        stream.Write(mHeader_PS2);
    }

    if (!mTextures.empty()) {
        return mTextures.front()->SaveToStream_PS2(stream);
    } else {
        return true;
    }
}

size_t SH2TextureContainer::GetNumTextures() const {
    return mTextures.size();
}

SH2Texture* SH2TextureContainer::GetTexture(const size_t idx) {
    return mTextures[idx];
}

void SH2TextureContainer::SetVirtual(const bool isVirtual) {
    mIsVirtual = true;
}

void SH2TextureContainer::AddTexture(SH2Texture* texture) {
    mTextures.push_back(texture);
}

const StringArray& SH2TextureContainer::GetErrors() const {
    return mErrors;
}

const StringArray& SH2TextureContainer::GetWarnings() const {
    return mWarnings;
}

bool SH2TextureContainer::IsPS2File() const {
    return mIsPS2File;
}
