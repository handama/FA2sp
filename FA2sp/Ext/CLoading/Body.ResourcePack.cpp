#include "Body.h"
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>

FString ResourcePack::toHex(const unsigned char* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

FString ResourcePack::encrypt_filename(const FString& filename, const unsigned char* key) {
    const EVP_MD* md = EVP_sha256();
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;

    HMAC(md,
        key, 32,
        reinterpret_cast<const unsigned char*>(filename.data()), filename.size(),
        result, &result_len);

    return toHex(result, 16);
}

bool ResourcePack::load(const FString& filename)
{
    file_path = filename;
    file_stream.open(filename, std::ios::binary);
    if (!file_stream) return false;

    char header[8];
    file_stream.read(header, 8);
    if (file_stream.gcount() != 8 || memcmp(header, "RPCK", 4) != 0) return false;

    index_size = *reinterpret_cast<uint32_t*>(&header[4]);

    if (index_size == 0 || index_size > (10 << 20)) // index maximum 10MB
        return false;

    std::vector<uint8_t> encrypted_index(index_size);
    file_stream.read(reinterpret_cast<char*>(encrypted_index.data()), index_size);
    if (file_stream.gcount() != static_cast<std::streamsize>(index_size)) return false;

    std::vector<uint8_t> decrypted_index;
    if (!aesDecryptBlockwise(encrypted_index.data(), index_size, decrypted_index)) {
        return false;
    }

    size_t offset = 0;
    while (offset + 32 + sizeof(FileEntry) <= decrypted_index.size()) {
        FString name(reinterpret_cast<char*>(&decrypted_index[offset]), 32);
        name = name.c_str();
        offset += 32;

        FileEntry entry;
        memcpy(&entry, &decrypted_index[offset], sizeof(FileEntry));
        offset += sizeof(FileEntry);

        // File size cannot over 200MB
        if (entry.enc_size == 0 || entry.enc_size > (200 << 20)) {
            return false;
        }

        index_map[name] = entry;
    }

    return true;
}

bool ResourcePack::aesDecryptBlockwise(const uint8_t* input, size_t len, std::vector<uint8_t>& output)
{
    if (len % AES_BLOCK_SIZE != 0) return false;

    output.resize(len);
    AES_KEY aes;
    AES_set_decrypt_key(get_aes_key().data(), 256, &aes);

    uint8_t iv[AES_BLOCK_SIZE] = { 0 };
    AES_cbc_encrypt(input, output.data(), len, &aes, iv, AES_DECRYPT);

    return true;
}

std::unique_ptr<uint8_t[]> ResourcePack::getFileData(const FString& filename, size_t* out_size)
{
    FString raw_name = filename;
    raw_name.MakeLower();
    auto encrypted_filename = encrypt_filename(raw_name, get_aes_key().data());
    auto it = index_map.find(encrypted_filename);
    if (it == index_map.end()) return nullptr;

    const FileEntry& entry = it->second;

    size_t data_offset = 8 + index_size + entry.offset;

    file_stream.clear();
    file_stream.seekg(data_offset);
    if (!file_stream) return nullptr;

    std::vector<uint8_t> encrypted_data(entry.enc_size);
    file_stream.read(reinterpret_cast<char*>(encrypted_data.data()), entry.enc_size);
    if (file_stream.gcount() != static_cast<std::streamsize>(entry.enc_size)) return nullptr;

    std::vector<uint8_t> decrypted_data;
    if (!aesDecryptBlockwise(encrypted_data.data(), entry.enc_size, decrypted_data)) {
        return nullptr;
    }

    if (decrypted_data.size() < entry.original_size) return nullptr;

    std::unique_ptr<uint8_t[]> result(new uint8_t[entry.original_size]);
    memcpy(result.get(), decrypted_data.data(), entry.original_size);

    if (out_size)
        *out_size = entry.original_size;

    return result;
}

ResourcePackManager& ResourcePackManager::instance()
{
    static ResourcePackManager mgr;
    return mgr;
}

bool ResourcePackManager::loadPack(const FString& packPath)
{
    auto pack = std::make_unique<ResourcePack>();
    if (pack->load(packPath)) {
        packs.push_back(std::move(pack));
        return true;
    }
    return false;
}

std::unique_ptr<uint8_t[]> ResourcePackManager::getFileData(const FString& filename, size_t* out_size) 
{
    for (auto& pack : packs) {
        auto data = pack->getFileData(filename, out_size);
        if (data) return data;
    }
    return nullptr;
}

void ResourcePackManager::clear() 
{
    packs.clear();
}