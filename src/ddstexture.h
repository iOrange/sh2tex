#pragma once
#include "mycommon.h"

#define DDS_FOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

constexpr uint32_t DDS_MAGIC        = DDS_FOURCC('D','D','S',' ');
constexpr uint32_t DDSD_CAPS        = 0x1;
constexpr uint32_t DDSD_HEIGHT      = 0x2;
constexpr uint32_t DDSD_WIDTH       = 0x4;
constexpr uint32_t DDSD_PITCH       = 0x8;
constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
constexpr uint32_t DDSCAPS_TEXTURE  = 0x1000;
constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
constexpr uint32_t DDPF_FOURCC      = 0x4;
constexpr uint32_t DDPF_RGB         = 0x40;

constexpr uint32_t DDS_FOURCC_DXT1  = DDS_FOURCC('D','X','T','1');
constexpr uint32_t DDS_FOURCC_DXT2  = DDS_FOURCC('D','X','T','2');
constexpr uint32_t DDS_FOURCC_DXT3  = DDS_FOURCC('D','X','T','3');
constexpr uint32_t DDS_FOURCC_DXT4  = DDS_FOURCC('D','X','T','4');
constexpr uint32_t DDS_FOURCC_DXT5  = DDS_FOURCC('D','X','T','5');


class DDSTexture {
public:
    DDSTexture();
    ~DDSTexture();

    bool            LoadFromFile(const fs::path& path);
    bool            LoadFromStream(MemStream& stream);

    bool            SaveToFile(const fs::path& path);

    uint32_t        GetWidth() const;
    uint32_t        GetHeight() const;
    uint32_t        GetFormat() const;
    const uint8_t*  GetData() const;

    void            SetWidth(const uint32_t width);
    void            SetHeight(const uint32_t height);
    void            SetFormat(const uint32_t format);
    void            SetData(const uint8_t* data, const size_t dataSize);

private:
    uint32_t    mWidth;
    uint32_t    mHeight;
    uint32_t    mFormat;
    BytesArray  mData;
};
