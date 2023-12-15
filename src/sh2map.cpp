#include "sh2map.h"
#include "sh2texture.h"

#include <fstream>

constexpr uint32_t kMapFileMagic = 0x20010510;

SH2Map::SH2Map()
    : mHeader{}
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
                mTexturesContainer = container;
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

    for (auto& sd : mSubDatas) {
        auto& subDataHeader = sd.first;

        if (subDataHeader.subDataType == 2) {    // textures
            // save our textures container to stream to know the subData size
            MemWriteStream texturesStream;
            if (!mTexturesContainer->SaveToStream(texturesStream)) {
                return false;
            }

            subDataHeader.subDataSize = scast<uint32_t>(texturesStream.GetWrittenBytesCount());

            stream.Write(subDataHeader);
            stream.Write(texturesStream.Data(), texturesStream.GetWrittenBytesCount());
        } else {
            stream.Write(subDataHeader);
            stream.Write(sd.second, subDataHeader.subDataSize);
        }
    }

    return true;
}

RefPtr<SH2TextureContainer> SH2Map::GetTexturesContainer() {
    return mTexturesContainer;
}
