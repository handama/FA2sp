#include "Body.h"
#include "../../FA2sp/Logger.h"
#include <iostream>
#include <cstring>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <CFinalSunApp.h>
#include <XCC/mix_file.h>
#include <openssl/bn.h>
#include <openssl/blowfish.h>
#include "../../FA2sp.h"

namespace MinInfo
{
    struct MixFileStruct {
        std::vector<MixEntry> files;
        uint32_t flags{};
        uint32_t size{};
        uint32_t offset{};
    };

    static inline uint32_t align_up(uint32_t x, uint32_t a) {
        return (x + (a - 1)) & ~(a - 1);
    }

    static bool read_exact(std::istream& in, void* dst, size_t len) {
        in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(len));
        return static_cast<size_t>(in.gcount()) == len;
    }

    static uint16_t read_u16(std::istream& in) {
        uint8_t b[2];
        if (!read_exact(in, b, 2)) throw std::runtime_error("read_u16 failed");
        return (uint16_t)(b[0] | (uint16_t(b[1]) << 8));
    }

    static uint32_t read_u32(std::istream& in) {
        uint8_t b[4];
        if (!read_exact(in, b, 4)) throw std::runtime_error("read_u32 failed");
        return (uint32_t)(b[0] | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24));
    }

    static std::vector<MixEntry> read_entries(std::istream& in, uint16_t count) {
        std::vector<MixEntry> v;
        v.reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            MixEntry e;
            e.id = read_u32(in);
            e.offset = read_u32(in);
            e.size = read_u32(in);
            v.push_back(e);
        }
        return v;
    }

    static void byteswap(std::vector<uint8_t>& buf) {
        std::reverse(buf.begin(), buf.end());
    }

    static void byteswap(uint8_t* p, size_t n) {
        for (size_t i = 0, j = n ? n - 1 : 0; i < j; ++i, --j) {
            std::swap(p[i], p[j]);
        }
    }

    static std::vector<uint8_t> rsa_transform_raw(const uint8_t* block, size_t block_len,
        const uint8_t* mod, size_t mod_len,
        uint32_t e) {
        std::vector<uint8_t> out;

        BIGNUM* x = BN_bin2bn(block, static_cast<int>(block_len), nullptr);
        BIGNUM* n = BN_bin2bn(mod, static_cast<int>(mod_len), nullptr);
        BIGNUM* y = BN_new();
        BIGNUM* be = BN_new();

        BN_CTX* ctx = BN_CTX_new();
        if (!x || !n || !y || !be || !ctx) goto cleanup;

        if (!BN_set_word(be, e)) goto cleanup;
        if (!BN_mod_exp(y, x, be, n, ctx)) goto cleanup;

        {
            int nbytes = BN_num_bytes(y);
            out.resize(nbytes);
            BN_bn2bin(y, out.data());
        }

    cleanup:
        if (ctx) BN_CTX_free(ctx);
        if (x) BN_free(x);
        if (n) BN_free(n);
        if (y) BN_free(y);
        if (be) BN_free(be);
        return out;
    }

    class BlowfishECBReader {
    public:
        BlowfishECBReader(std::istream& in, const uint8_t* key, size_t key_len)
            : in_(in) {
            BF_set_key(&key_, static_cast<int>(key_len), key);
        }

        void read(uint8_t* dst, size_t len) {
            ensure(len);
            size_t n = std::min(len, buffer_.size() - rdpos_);
            std::memcpy(dst, buffer_.data() + rdpos_, n);
            rdpos_ += n;
            if (n != len) throw std::runtime_error("ECB short read");
            if (rdpos_ > 4096) {
                buffer_.erase(buffer_.begin(), buffer_.begin() + rdpos_);
                rdpos_ = 0;
            }
        }

    private:
        void ensure(size_t need) {
            size_t avail = buffer_.size() - rdpos_;
            if (avail >= need) return;

            const size_t blksz = 8;
            size_t missing = need - avail;
            size_t nbytes = ((missing + blksz - 1) / blksz) * blksz;

            std::vector<uint8_t> enc(nbytes, 0);
            if (!read_exact(in_, enc.data(), nbytes)) {
                throw std::runtime_error("ECB underlying stream EOF");
            }

            for (size_t off = 0; off < nbytes; off += blksz) {
                BF_ecb_encrypt(enc.data() + off, enc.data() + off, &key_, BF_DECRYPT);
            }

            if (rdpos_ == 0) {
                buffer_.insert(buffer_.end(), enc.begin(), enc.end());
            }
            else {
                buffer_.erase(buffer_.begin(), buffer_.begin() + rdpos_);
                rdpos_ = 0;
                buffer_.insert(buffer_.end(), enc.begin(), enc.end());
            }
        }

        std::istream& in_;
        BF_KEY key_;
        std::vector<uint8_t> buffer_;
        size_t rdpos_ = 0;
    };

    static uint16_t read_u16_ecb(BlowfishECBReader& r) {
        uint8_t b[2];
        r.read(b, 2);
        return (uint16_t)(b[0] | (uint16_t(b[1]) << 8));
    }

    static uint32_t read_u32_ecb(BlowfishECBReader& r) {
        uint8_t b[4];
        r.read(b, 4);
        return (uint32_t)(b[0] | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24));
    }

    static std::vector<MixEntry> read_entries_ecb(BlowfishECBReader& r, uint16_t count) {
        std::vector<MixEntry> v;
        v.reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            MixEntry e;
            e.id = read_u32_ecb(r);
            e.offset = read_u32_ecb(r);
            e.size = read_u32_ecb(r);
            v.push_back(e);
        }
        return v;
    }

    static const uint8_t RSA_MODULUS[] = {
        0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39, 0xfc, 0xe4,
        0x56, 0x51, 0x60, 0xd6, 0x51, 0x71, 0x3f, 0xa2,
        0xe8, 0xaa, 0x54, 0xfa, 0x66, 0x82, 0xb0, 0x4a,
        0xab, 0xdd, 0x0e, 0x6a, 0xf8, 0xb0, 0xc1, 0xe6,
        0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43, 0x7f, 0x15,
    };
    static const uint32_t RSA_E = 0x10001;

    static std::vector<uint8_t> blowfish_key_from_keysource(const uint8_t keySource[80]) {
        uint8_t ks[80];
        std::memcpy(ks, keySource, 80);
        byteswap(ks, 80);

        const uint8_t* left = ks;
        const uint8_t* right = ks + 40;

        std::vector<uint8_t> s0 = rsa_transform_raw(left, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);
        std::vector<uint8_t> s1 = rsa_transform_raw(right, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);

        BIGNUM* a = BN_bin2bn(s0.data(), (int)s0.size(), nullptr);
        BIGNUM* b = BN_bin2bn(s1.data(), (int)s1.size(), nullptr);
        BIGNUM* c = BN_new();
        BIGNUM* d = BN_new();
        BN_CTX* ctx = BN_CTX_new();

        if (!a || !b || !c || !d || !ctx) {
            if (a) BN_free(a); if (b) BN_free(b); if (c) BN_free(c); if (d) BN_free(d);
            if (ctx) BN_CTX_free(ctx);
            return {};
        }
        if (!BN_lshift(c, a, 312)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }
        if (!BN_add(d, b, c)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }

        int n = BN_num_bytes(d);
        std::vector<uint8_t> key(n);
        BN_bn2bin(d, key.data());
        byteswap(key);

        BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);

        if (key.size() > 56) key.resize(56);
        if (key.size() < 4) key.resize(4, 0);

        return key;
    }

    static std::unique_ptr<MixFileStruct> unpack_mix(std::istream& in, uint32_t total_size) {
        uint16_t count_or_zero = read_u16(in);

        if (count_or_zero != 0) {
            uint16_t count = count_or_zero;
            (void)read_u32(in);
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = 0;
            mf->offset = 6u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }

        uint16_t flags16 = read_u16(in);
        uint32_t flags = uint32_t(flags16) << 16;

        constexpr uint32_t flagEncrypted = 0x00020000u;

        if (flags & flagEncrypted) {
            uint8_t keySource[80];
            if (!read_exact(in, keySource, 80)) throw std::runtime_error("read keySource failed");

            std::vector<uint8_t> bf_key = blowfish_key_from_keysource(keySource);
            if (bf_key.empty()) throw std::runtime_error("derive blowfish key failed");

            BlowfishECBReader ecb(in, bf_key.data(), bf_key.size());

            uint16_t count = read_u16_ecb(ecb);
            (void)read_u32_ecb(ecb);
            auto entries = read_entries_ecb(ecb, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = flags;

            uint32_t index_len = 6u + 12u * static_cast<uint32_t>(count);
            mf->offset = 84u + align_up(index_len, 8u);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
        else {
            uint16_t count = read_u16(in);
            (void)read_u32(in);
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = flags;
            mf->offset = 10u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
    }

    static uint32_t file_size(std::ifstream& f) {
        auto pos = f.tellg();
        f.seekg(0, std::ios::end);
        std::streamoff end = f.tellg();
        f.seekg(pos, std::ios::beg);
        return static_cast<uint32_t>(end);
    }

    const uint32_t adlerTable[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
        0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
        0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
        0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
        0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
        0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
        0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
        0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
        0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
        0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
        0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
        0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
        0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
        0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
        0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
        0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
        0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
        0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
        0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
        0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    static uint32_t crc32(const std::vector<uint8_t>& bytes) {
        uint32_t c = 0 ^ 0xFFFFFFFF;
        for (size_t i = 0; i < bytes.size(); i++) {
            c = adlerTable[(c ^ bytes[i]) & 0xFF] ^ (c >> 8);
        }
        return (c ^ 0xFFFFFFFF);
    }

    static uint32_t wwGetID(const std::string& filename) {
        if (filename.empty()) return 0;
        std::string upper = filename;
        for (char& c : upper) {
            c = std::toupper(c);
        }
        std::vector<uint8_t> bytes;
        for (char c : upper) {
            bytes.push_back(c & 0xFF);
        }
        size_t align = upper.length() & (~3);
        int counter = static_cast<int>(upper.length() - align);
        if (counter > 0) {
            uint8_t repeat = upper[align] & 0xFF;
            bytes.push_back(counter & 0xFF);
            counter = 3 - counter;
            for (int i = 0; i < counter; i++) {
                bytes.push_back(repeat);
            }
        }
        return crc32(bytes);
    }

    static std::vector<MixEntry> GetMixInfo(const std::string& cPath) {
        VEHGuard guard(false);
        try {
            std::ifstream f(cPath, std::ios::binary);
            if (!f)  return {};

            uint32_t total = file_size(f);
            f.seekg(0, std::ios::beg);

            auto mix = unpack_mix(f, total);
            for (auto& e : mix->files) {
                e.offset += mix->offset;
            }
            return std::move(mix->files);
        }
        catch (...) {
            return {};
        }
    }

    static std::vector<MixEntry> GetMixInfoFromRange(const std::string& cPath, uint32_t offset, uint32_t size) {
        VEHGuard guard(false);
        try {
            if (offset < 0 || size <= 0)  return {};

            std::ifstream f(cPath, std::ios::binary);
            if (!f)  return {};

            f.seekg(0, std::ios::end);
            uint32_t file_end = static_cast<uint32_t>(f.tellg());
            uint32_t start = static_cast<uint32_t>(offset);
            uint32_t length = static_cast<uint32_t>(size);
            if (start > file_end || start + length > file_end)  return {};

            std::vector<char> buf(length);
            f.seekg(static_cast<std::streamoff>(start), std::ios::beg);
            if (!read_exact(f, buf.data(), buf.size()))  return {};

            std::istringstream sub(std::string(buf.data(), buf.size()));
            auto mix = unpack_mix(sub, length);
            for (auto& e : mix->files) {
                e.offset += start + mix->offset;
            }
            return std::move(mix->files);
        }
        catch (...) {
            return {};
        }
    }
} 

inline int _stricmp(const char* a, const char* b) {
    std::string sa(a), sb(b);
    std::transform(sa.begin(), sa.end(), sa.begin(), ::tolower);
    std::transform(sb.begin(), sb.end(), sb.begin(), ::tolower);
    return sa.compare(sb);
}

uint32_t MixLoader::GetFileID(const std::string& fileName)
{
    return MinInfo::wwGetID(fileName);
}

MixLoader& MixLoader::Instance() {
    static MixLoader inst;
    return inst;
}

bool MixLoader::LoadTopMix(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin.is_open()) return false;
    auto entries = MinInfo::GetMixInfo(path);

    MixFile mf;
    mf.path = path;
    mf.stream = std::move(fin);
    mf.entries = std::move(entries);
    mf.isNested = false;
    mf.baseOffset = 0;

    for (const auto& e : mf.entries) {
        if (fileMap.find(e.id) == fileMap.end())
            fileMap[e.id] = { mixFiles.size(), &e};
    }

    mixFiles.push_back(std::move(mf));
    return true;
}

bool MixLoader::LoadNestedMix(MixFile& parent, const MixEntry& entry) {
    MixFile mf;
    mf.path = parent.path;
    mf.stream.open(parent.path, std::ios::binary);
    if (!mf.stream.is_open()) return false;

    auto entries = MinInfo::GetMixInfoFromRange(parent.path, entry.offset, entry.size);
    mf.entries = std::move(entries);
    mf.isNested = true;
    mf.baseOffset = entry.offset;

    for (const auto& e : mf.entries) {
        if (fileMap.find(e.id) == fileMap.end())
            fileMap[e.id] = { mixFiles.size(), &e };
    }

    mixFiles.push_back(std::move(mf));
    return true;
}

bool MixLoader::LoadMixFile(const std::string& path, int* parentIndex) {
    if (std::filesystem::exists(path)) {
        if (parentIndex) *parentIndex = -1;
        return LoadTopMix(path);
    }

    auto name = std::filesystem::path(path).filename().string();
    uint32_t id = GetFileID(name);
    for (size_t i = 0; i < mixFiles.size(); ++i) {
        auto& mf = mixFiles[i];
        for (const auto& e : mf.entries) {
            if (e.id == id) {
                if (parentIndex) *parentIndex = i;
                return LoadNestedMix(mf, e);
            }
        }
    }

    return false;
}

int MixLoader::QueryFileIndex(const std::string& fileName, int mixIdx) {
    if (mixFiles.empty()) return -1;
    uint32_t id = GetFileID(fileName);
    if (mixIdx >= 0 && mixIdx < (int)mixFiles.size()) {
        auto& mix = mixFiles[mixIdx];
        for (const auto& e: mix.entries) {
            if (e.id == id)
                return mixIdx;
        }
        return -1;
    }

    auto itr = fileMap.find(id);
    if (itr != fileMap.end())
        return static_cast<int>(itr->second.mixIndex);

    return -1;
}

std::unique_ptr<uint8_t[]> MixLoader::LoadFile(const std::string& fileName, size_t* outSize, int mixIdx) {
    if (mixFiles.empty()) return nullptr;
    if (outSize) *outSize = 0;
    uint32_t id = GetFileID(fileName);
    if (mixIdx >= 0 && mixIdx < (int)mixFiles.size()) {
        auto& mf = mixFiles[mixIdx];
        for (auto& e : mf.entries) {
            if (e.id == id) {
                if (!mf.stream.is_open()) {
                    mf.stream.open(mf.path, std::ios::binary);
                    if (!mf.stream.is_open()) return nullptr;
                }

                mf.stream.seekg(e.offset, std::ios::beg);
                if (!mf.stream.good()) return nullptr;

                auto buf = std::make_unique<uint8_t[]>((size_t)e.size);
                mf.stream.read(reinterpret_cast<char*>(buf.get()), e.size);
                std::streamsize got = mf.stream.gcount();
                if (got != (std::streamsize)e.size) {
                    return nullptr;
                }
                if (outSize) *outSize = (size_t)e.size;
                return buf;
            }
        }
        return nullptr;
    }

    auto itr = fileMap.find(id);
    if (itr != fileMap.end())
    {
        auto& mf = mixFiles[itr->second.mixIndex];
        const auto& e = itr->second.entry;
        if (!mf.stream.is_open()) {
            mf.stream.open(mf.path, std::ios::binary);
            if (!mf.stream.is_open()) return nullptr;
        }

        mf.stream.seekg(e->offset, std::ios::beg);
        if (!mf.stream.good()) return nullptr;

        auto buf = std::make_unique<uint8_t[]>((size_t)e->size);
        mf.stream.read(reinterpret_cast<char*>(buf.get()), e->size);
        std::streamsize got = mf.stream.gcount();
        if (got != (std::streamsize)e->size) {
            return nullptr;
        }
        if (outSize) *outSize = (size_t)e->size;
        return buf;
    }

    return nullptr;
}

bool MixLoader::ExtractFile(const std::string& fileName, const std::string& outPath, int mixIdx)
{
    size_t size = 0;
    if (auto file = LoadFile(fileName, &size, mixIdx))
    {
        std::ofstream fout(outPath, std::ios::binary);
        if (!fout) {
            return false;
        }

        fout.write((const char*)file.get(), size);
        return fout.good();
    }
    return false;
}

void MixLoader::Clear() {
    mixFiles.clear();
    fileMap.clear();
}