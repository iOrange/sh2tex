#include "sh2map.h"
#include "sh2texture.h"

#include <fstream>

constexpr uint32_t kMapFileMagic = 0x20010510;

SH2Map::SH2Map()
    : mIsPS2(false)
    , mHeader{}
{
}
SH2Map::~SH2Map() {
    std::for_each(mSubDatas.begin(), mSubDatas.end(), [](SubData& sd) {
        if (sd.second != nullptr) {
            std::free(sd.second);
        }
    });
    mSubDatas.clear();
}

bool SH2Map::LoadFromFile(const fs::path& path) {
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

bool SH2Map::LoadFromStream(MemStream& stream) {
    if (stream.Remains() < (sizeof(SH2MapHeader) + sizeof(SH2MapSubDataHeader))) {
        return false;
    }

    stream.ReadStruct(mHeader);

    if (mHeader.magic != kMapFileMagic) {
        // check for PS2 file
        if (mHeader.magic == 0x77777777) {
            stream.RewindBytes(sizeof(mHeader));
            return this->LoadFromStream_PS2(stream);
        }

        return false;
    }

    mSubDatas.resize(mHeader.numFiles);
    for (auto& sd : mSubDatas) {
        sd.second = nullptr;
        stream.ReadStruct(sd.first);

        const auto& subDataHeader = sd.first;
        if (subDataHeader.subDataType == 2) {    // textures
            RefPtr<SH2TextureContainer> container = MakeRefPtr<SH2TextureContainer>();
            MemStream subStream = stream.Substream(subDataHeader.subDataSize);
            if (container->LoadFromStream(subStream)) {
                mTexturesContainers.emplace_back(container);
            } else {
                return false;
            }
        } else {
            void* dataMem = std::malloc(subDataHeader.subDataSize);
            std::memcpy(dataMem, stream.GetDataAtCursor(), subDataHeader.subDataSize);
            sd.second = dataMem;
        }

        stream.SkipBytes(subDataHeader.subDataSize);
    }

    // create a virtual textures container that will hold ALL textures, just for the viewer
    mVirtualTexturesContainer = MakeRefPtr<SH2TextureContainer>();
    mVirtualTexturesContainer->SetVirtual(true);
    for (auto& container : mTexturesContainers) {
        for (size_t i = 0; i < container->GetNumTextures(); ++i) {
            mVirtualTexturesContainer->AddTexture(container->GetTexture(i));
        }
    }

    return true;
}

bool SH2Map::LoadFromStream_PS2(MemStream& stream) {
    uint32_t header[12] = {};
    stream.ReadToBuffer(header, sizeof(header));

    //const uint32_t texOffset0 = header[4];
    //const uint32_t texOffset1 = header[5];
    //const uint32_t texOffset1 = header[6];
    //const uint32_t palOffset0 = header[7];
    //const uint32_t palOffset1 = header[8];
    //const uint32_t palOffset1 = header[9];
    const uint32_t numTextures = header[10];

    mVirtualTexturesContainer = MakeRefPtr<SH2TextureContainer>();
    for (size_t i = 0; i < numTextures; ++i) {
        MemStream tstream = stream.Substream(header[i + 4], stream.Length());

        SH2Texture* texture = new SH2Texture();
        if (texture->LoadFromStream_PS2(tstream)) {
            mVirtualTexturesContainer->AddTexture(texture);
        } else {
            delete texture;
        }
    }

    mIsPS2 = true;

    return true;
}

bool SH2Map::SaveToFile(const fs::path& path) {
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

bool SH2Map::SaveToStream(MemWriteStream& stream) {
    stream.Write(mHeader);

    size_t containerIdx = 0;
    for (auto& sd : mSubDatas) {
        auto& subDataHeader = sd.first;

        if (subDataHeader.subDataType == 2) {    // textures
            // save our textures container to stream to know the subData size
            MemWriteStream texturesStream;
            if (!mTexturesContainers[containerIdx]->SaveToStream(texturesStream)) {
                return false;
            }

            subDataHeader.subDataSize = scast<uint32_t>(texturesStream.GetWrittenBytesCount());

            stream.Write(subDataHeader);
            stream.Write(texturesStream.Data(), texturesStream.GetWrittenBytesCount());

            ++containerIdx;
        } else {
            stream.Write(subDataHeader);
            stream.Write(sd.second, subDataHeader.subDataSize);
        }
    }

    return true;
}

bool SH2Map::IsPS2() const {
    return mIsPS2;
}

RefPtr<SH2TextureContainer> SH2Map::GetTexturesContainer() {
    return mVirtualTexturesContainer;
}
