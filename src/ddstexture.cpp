#include "ddstexture.h"
#include "libs/bcdec/bcdec.h" // no implementation, just size helpers
#include <fstream>

struct DDCOLORKEY {
    uint32_t        dwUnused0;
    uint32_t        dwUnused1;
};

struct DDPIXELFORMAT {
    uint32_t        dwSize;
    uint32_t        dwFlags;
    uint32_t        dwFourCC;
    uint32_t        dwRGBBitCount;
    uint32_t        dwRBitMask;
    uint32_t        dwGBitMask;
    uint32_t        dwBBitMask;
    uint32_t        dwRGBAlphaBitMask;
};

struct DDSCAPS2 {
    uint32_t        dwCaps;
    uint32_t        dwCaps2;
    uint32_t        dwCaps3;
    uint32_t        dwCaps4;
};

struct DDSURFACEDESC2 {
    uint32_t        dwSize;
    uint32_t        dwFlags;
    uint32_t        dwHeight;
    uint32_t        dwWidth;
    union {
        int         lPitch;
        uint32_t    dwLinearSize;
    };
    uint32_t        dwBackBufferCount;
    uint32_t        dwMipMapCount;
    uint32_t        dwAlphaBitDepth;
    uint32_t        dwUnused0;
    uint32_t        lpSurface;
    DDCOLORKEY      unused0;
    DDCOLORKEY      unused1;
    DDCOLORKEY      unused2;
    DDCOLORKEY      unused3;
    DDPIXELFORMAT   ddpfPixelFormat;
    DDSCAPS2        ddsCaps;
    uint32_t        dwUnused1;
};



DDSTexture::DDSTexture()
    : mWidth(0)
    , mHeight(0)
    , mFormat(0)
{
}
DDSTexture::~DDSTexture() {
}

bool DDSTexture::LoadFromFile(const fs::path& path) {
    std::ifstream file(path, std::ios_base::binary);
    if (!file.good()) {
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

bool DDSTexture::LoadFromStream(MemStream& stream) {
    if (stream.Remains() < 128) {
        return false;
    }

    const uint32_t magic = stream.ReadU32();
    if (magic != DDS_MAGIC) {
        return false;
    }

    DDSURFACEDESC2 hdr;
    stream.ReadStruct(hdr);

    bool isDXT = false;

    if (hdr.ddpfPixelFormat.dwFlags & DDPF_FOURCC) {
        if (hdr.ddpfPixelFormat.dwFourCC == DDS_FOURCC_DXT1 || hdr.ddpfPixelFormat.dwFourCC == DDS_FOURCC_DXT2 ||
            hdr.ddpfPixelFormat.dwFourCC == DDS_FOURCC_DXT3 || hdr.ddpfPixelFormat.dwFourCC == DDS_FOURCC_DXT4 || hdr.ddpfPixelFormat.dwFourCC == DDS_FOURCC_DXT5) {
            isDXT = true;
            mFormat = hdr.ddpfPixelFormat.dwFourCC;
        } else {
            return false;
        }
    } else if (hdr.ddpfPixelFormat.dwRGBBitCount == 32 && hdr.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF000000 &&
               hdr.ddpfPixelFormat.dwRBitMask == 0x00FF0000 && hdr.ddpfPixelFormat.dwGBitMask == 0x0000FF00 &&
               hdr.ddpfPixelFormat.dwBBitMask == 0x000000FF) {
        mFormat = 32;
    } else {
        return false;
    }

    mWidth = hdr.dwWidth;
    mHeight = hdr.dwHeight;

    size_t dataSize;
    if (isDXT) {
        dataSize = (mFormat == DDS_FOURCC_DXT1) ? BCDEC_BC1_COMPRESSED_SIZE(mWidth, mHeight) : BCDEC_BC3_COMPRESSED_SIZE(mWidth, mHeight);
    } else {
        dataSize = mWidth * mHeight * 4;
    }

    mData.resize(dataSize);
    stream.ReadToBuffer(mData.data(), dataSize);

    return true;
}

bool DDSTexture::SaveToFile(const fs::path& path) {
    std::ofstream file(path, std::ios_base::binary);
    if (!file.good()) {
        return false;
    }

    MemWriteStream stream;

    stream.WriteU32(DDS_MAGIC);

    DDSURFACEDESC2 hdr = {};
    hdr.dwSize = sizeof(DDSURFACEDESC2);
    hdr.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
    hdr.dwWidth = mWidth;
    hdr.dwHeight = mHeight;
    hdr.dwMipMapCount = 1;
    hdr.dwBackBufferCount = 1; // again, some viewers/editors want this, even though this is stupid
    hdr.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    hdr.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    if (mFormat == 32) {
        // some editors / viewers demand these to be set for the uncompressed texture
        hdr.dwFlags |= DDSD_PITCH;
        hdr.dwLinearSize = mWidth * 4;

        hdr.ddpfPixelFormat.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
        hdr.ddpfPixelFormat.dwRGBBitCount = 32;
        hdr.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
        hdr.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        hdr.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        hdr.ddpfPixelFormat.dwBBitMask = 0x000000FF;
    } else {
        hdr.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
        hdr.ddpfPixelFormat.dwFourCC = mFormat;
    }

    stream.Write(hdr);
    stream.Write(mData.data(), mData.size());

    file.write(rcast<const char*>(stream.Data()), stream.GetWrittenBytesCount());
    file.flush();
    file.close();

    return true;
}

uint32_t DDSTexture::GetWidth() const {
    return mWidth;
}

uint32_t DDSTexture::GetHeight() const {
    return mHeight;
}

uint32_t DDSTexture::GetFormat() const {
    return mFormat;
}

const uint8_t* DDSTexture::GetData() const {
    return mData.data();
}

void DDSTexture::SetWidth(const uint32_t width) {
    mWidth = width;
}

void DDSTexture::SetHeight(const uint32_t height) {
    mHeight = height;
}

void DDSTexture::SetFormat(const uint32_t format) {
    mFormat = format;
}

void DDSTexture::SetData(const uint8_t* data, const size_t dataSize) {
    mData.resize(dataSize);
    std::memcpy(mData.data(), data, dataSize);
}
