#pragma once

#include <cstddef>
#include <cstdint>

// Fixed contract between the resource pack loader and the per-release generated
// implementation. The concrete algorithm, tables and per-file parameters are produced by
// Supplementary/Tools/ResourceEncryptor/generator.exe, which writes both the implementation
// (Body.Encryption.cpp) and this header; neither is committed.
//
// This checked in copy is the placeholder: it must declare exactly what the loader calls, or
// the tree will not build without running the generator first. Changing a signature here
// means changing it in the generator's HEADER_TEMPLATE as well.
namespace ResourceCipher
{
	// Container layout selector. It carries no authentication of its own: a pack is
	// only accepted once its header mac verifies, so a file with the wrong selector
	// simply fails to load.
	constexpr size_t kHeaderTagLen = 4;
	constexpr size_t kPackIdLen = 8;
	constexpr size_t kPackNonceLen = 16;
	constexpr size_t kNameKeyLen = 16;
	// Width of the random id at the head of every index entry prefix. It is not derived from
	// the file name: the index carries no name derived value at all, see the note on
	// PackIndexEntryHeader below.
	constexpr size_t kEntryIdLen = 16;
	// The decrypted index is padded up to a multiple of this, so its length does not report
	// the exact total length of the names inside it. DecryptIndex accepts that much slack and
	// no more.
	constexpr size_t kIndexPadBucket = 4096;
	constexpr size_t kMacLen = 16;
	constexpr size_t kIvLen = 16;
	constexpr size_t kAesKeyLen = 32;
	constexpr size_t kAesBlockLen = 16;

	// Container limits enforced by the loader.
	constexpr uint32_t kMaxKeyPartLen = 64;
	constexpr uint32_t kMaxHeadTailSample = 32;
	// Longest file name the index name table holds. The packer refuses anything longer.
	constexpr uint32_t kMaxNameLen = 1024;
	constexpr uint32_t kMaxIndexSize = 10u << 20;
	constexpr uint32_t kMaxEntrySize = 200u << 20;

	// Fixed 48 byte prefix of every index entry; a variable tail follows. The order of the
	// fields is drawn from the release seed by the generator, so no byte offset in here is
	// fixed across releases; the canonical order below is what the placeholder uses. Read the
	// fields by name.
	//
	// The head of the decrypted index holds a name table, ahead of the entries, and it is the
	// only place a file name appears: a u32 byte length, a u32 entry count, then that many
	// records of (u32 ordinal, u16 name length, lower case name) in an order the packer
	// shuffled. The entries follow in packing order and any bytes left over are padding. The
	// table sits inside the index plaintext, so it is encrypted with the index key and covered
	// by the index mac: looking a name up therefore means decrypting the index first, and the
	// index deliberately offers no per candidate check that works without the key.
#pragma pack(push, 1)
	struct PackIndexEntryHeader
	{
		uint8_t  entryId[kEntryIdLen];   // random per entry, not derived from the name
		uint16_t slot;
		uint16_t entryKeyPartLen;
		uint16_t saltLen;
		uint16_t prevChainLen;
		uint32_t offset;
		uint32_t encSize;
		uint32_t rawSize;
		uint64_t parentInfo;
		uint16_t paramsLen;
		uint16_t macLen;
	};
#pragma pack(pop)
	static_assert(sizeof(PackIndexEntryHeader) == 48, "PackIndexEntryHeader must be 48 bytes");

	struct LayoutInfo
	{
		uint16_t variant;
		uint32_t packKeyPartLen;
		uint32_t indexKeyPartLen;
		uint32_t entryKeyPartLen;
		uint32_t headTailSampleLen;
		uint32_t fixedHeaderBytes;
	};

	struct PackContext
	{
		uint16_t layoutVariant;
		uint32_t bindingIndex;
		const uint8_t* packId;
		const uint8_t* packNonce;
		const uint8_t* packKeyPart;
		size_t packKeyPartLen;
		const uint8_t* indexKeyPart;
		size_t indexKeyPartLen;
		const uint8_t* indexMac;
		size_t macLen;
		uint32_t indexEncSize;
		uint32_t indexPlainSize;
		uint64_t dataRegionSize;
	};

	struct FileContext
	{
		uint32_t slot;
		uint32_t entryIndex;
		uint64_t parentInfo;
		uint32_t offset;
		uint32_t encSize;
		uint32_t rawSize;
		const uint8_t* entryKeyPart;
		size_t entryKeyPartLen;
		const uint8_t* salt;
		size_t saltLen;
		const uint8_t* prevChain;
		size_t prevChainLen;
		const uint8_t* params;
		size_t paramsLen;
		const uint8_t* entryMac;
		size_t entryMacLen;
		const uint8_t* cipher;
		size_t cipherLen;
	};

	// Called once before any pack is loaded. Returns false on the placeholder
	// implementation so that pack loading fails gracefully.
	bool Initialize();

	// Number of independent transform slots.
	uint32_t SlotCount();

	// Layout description for a container variant. Unsupported variants fail.
	bool GetLayoutInfo(uint16_t variant, LayoutInfo* out);

	// Pack binding lookup. A miss means the DLL does not match the pack.
	bool LookupPackBinding(const uint8_t packId[kPackIdLen], uint32_t* outBindingIndex);

	// Header integrity check over header[0, headerLen) against storedMac.
	bool VerifyHeaderMac(const uint8_t* header, size_t headerLen,
		const uint8_t* storedMac, const PackContext& ctx);

	// In-place index decryption and integrity check.
	bool DecryptIndex(uint8_t* buf, size_t len, size_t* outPlainLen, const PackContext& ctx);

	// Single file decryption: outer inverse transform, AES-256-CBC, MAC check and
	// plain head/tail binding check. name must be the canonical name the entry was
	// packed with: the content key is derived from it, so a wrong name never decrypts.
	bool DecryptPackFile(const char* name, size_t nameLen,
		const PackContext& pack, const FileContext& file,
		uint8_t* buf, size_t len, size_t* outPlainLen);
}
