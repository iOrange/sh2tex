#include "sh2model.h"
#include "sh2texture.h"

#include <fstream>


SH2Model::SH2Model()
    : mHeader{}
{
}
SH2Model::~SH2Model() {
}

bool SH2Model::LoadFromFile(const fs::path& path) {
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

bool SH2Model::LoadFromStream(MemStream& stream) {
    if (stream.Remains() <= sizeof(SH2MDLContainerHeader)) {
        return false;
    }

    stream.ReadStruct(mHeader);
    if (!mHeader.numTextures) {
        return true;
    }

    if (mHeader.texturesOffset <= sizeof(SH2MDLContainerHeader) || mHeader.texturesOffset >= stream.Length()) {
        return false;
    }

    MemStream texturesStream = stream.Substream(mHeader.texturesOffset, stream.Length() - mHeader.texturesOffset);
    RefPtr<SH2TextureContainer> textures = MakeRefPtr<SH2TextureContainer>();
    if (!textures->LoadFromStream(texturesStream)) {
        return false;
    }
    assert(texturesStream.Remains() == 0);

    mTexturesContainer = textures;

    mGeometryData.resize(mHeader.texturesOffset - stream.GetCursor());
    stream.ReadToBuffer(mGeometryData.data(), mGeometryData.size());

    return true;
}

bool SH2Model::SaveToFile(const fs::path& path) {
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

bool SH2Model::SaveToStream(MemWriteStream& stream) {
    stream.Write(mHeader);
    stream.Write(mGeometryData.data(), mGeometryData.size());
    return mTexturesContainer->SaveToStream(stream);
}

RefPtr<SH2TextureContainer> SH2Model::GetTexturesContainer() {
    return mTexturesContainer;

}
