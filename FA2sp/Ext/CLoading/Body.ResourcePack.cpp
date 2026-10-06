#include "Body.h"
#include <cstring>
#include <string>

namespace
{
	constexpr size_t kPrefixBytes = ResourceCipher::kHeaderTagLen + 2
		+ ResourceCipher::kPackIdLen + ResourceCipher::kPackNonceLen;

	uint16_t ReadU16(const uint8_t* p) { uint16_t v; memcpy(&v, p, 2); return v; }
	uint32_t ReadU32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
	uint64_t ReadU64(const uint8_t* p) { uint64_t v; memcpy(&v, p, 8); return v; }

}

bool ResourcePack::load(const FString& filename)
{
	file_path = filename;
	file_stream.open(filename, std::ios::binary);
	if (!file_stream) return false;

	if (!ResourceCipher::Initialize()) return false;

	// The prefix carries an unsigned layout selector; nothing about it is trusted.
	// Whether the file belongs to us is decided later by the header mac, so there is
	// no cheap signature to probe with.
	uint8_t prefix[kPrefixBytes];
	file_stream.read(reinterpret_cast<char*>(prefix), sizeof(prefix));
	if (file_stream.gcount() != static_cast<std::streamsize>(sizeof(prefix))) return false;

	const uint16_t variant = ReadU16(prefix + ResourceCipher::kHeaderTagLen);
	if (!ResourceCipher::GetLayoutInfo(variant, &layout)) return false;
	if (layout.packKeyPartLen > ResourceCipher::kMaxKeyPartLen) return false;
	if (layout.indexKeyPartLen > ResourceCipher::kMaxKeyPartLen) return false;
	if (layout.entryKeyPartLen == 0 || layout.entryKeyPartLen > ResourceCipher::kMaxKeyPartLen) return false;
	if (layout.headTailSampleLen > ResourceCipher::kMaxHeadTailSample) return false;
	if (layout.fixedHeaderBytes < sizeof(prefix) + 2 * ResourceCipher::kMacLen) return false;

	memcpy(pack_id.data(), prefix + ResourceCipher::kHeaderTagLen + 2, ResourceCipher::kPackIdLen);
	memcpy(pack_nonce.data(), prefix + ResourceCipher::kHeaderTagLen + 2 + ResourceCipher::kPackIdLen,
		ResourceCipher::kPackNonceLen);

	std::vector<uint8_t> header(layout.fixedHeaderBytes);
	memcpy(header.data(), prefix, sizeof(prefix));
	const size_t rest = layout.fixedHeaderBytes - sizeof(prefix);
	file_stream.read(reinterpret_cast<char*>(header.data() + sizeof(prefix)), rest);
	if (file_stream.gcount() != static_cast<std::streamsize>(rest)) return false;

	size_t off = sizeof(prefix);
	pack_key_part.assign(header.begin() + off, header.begin() + off + layout.packKeyPartLen);
	off += layout.packKeyPartLen;
	index_key_part.assign(header.begin() + off, header.begin() + off + layout.indexKeyPartLen);
	off += layout.indexKeyPartLen;

	const uint32_t indexEncSize = ReadU32(&header[off]); off += 4;
	const uint32_t indexPlainSize = ReadU32(&header[off]); off += 4;
	data_region_size = ReadU64(&header[off]); off += 8;
	memcpy(index_mac.data(), &header[off], ResourceCipher::kMacLen); off += ResourceCipher::kMacLen;
	const uint8_t* headerMac = &header[off]; off += ResourceCipher::kMacLen;
	if (off != layout.fixedHeaderBytes) return false;

	if (indexEncSize == 0 || indexEncSize > ResourceCipher::kMaxIndexSize) return false;
	if ((indexEncSize % ResourceCipher::kAesBlockLen) != 0) return false;
	if (indexPlainSize < sizeof(ResourceCipher::PackIndexEntryHeader) || indexPlainSize > indexEncSize)
		return false;

	data_region_offset = layout.fixedHeaderBytes + indexEncSize;

	// Read the index before probing the file size: probing moves the read position.
	std::vector<uint8_t> indexBuf(indexEncSize);
	file_stream.read(reinterpret_cast<char*>(indexBuf.data()), indexEncSize);
	if (file_stream.gcount() != static_cast<std::streamsize>(indexEncSize)) return false;

	file_stream.clear();
	file_stream.seekg(0, std::ios::end);
	const std::streamoff total = file_stream.tellg();
	if (total < 0) return false;
	if (data_region_offset + data_region_size > static_cast<uint64_t>(total)) return false;

	uint32_t bindingIndex = 0;
	if (!ResourceCipher::LookupPackBinding(pack_id.data(), &bindingIndex)) return false;

	pack_ctx.layoutVariant = variant;
	pack_ctx.bindingIndex = bindingIndex;
	pack_ctx.packId = pack_id.data();
	pack_ctx.packNonce = pack_nonce.data();
	pack_ctx.packKeyPart = pack_key_part.data();
	pack_ctx.packKeyPartLen = pack_key_part.size();
	pack_ctx.indexKeyPart = index_key_part.data();
	pack_ctx.indexKeyPartLen = index_key_part.size();
	pack_ctx.indexMac = index_mac.data();
	pack_ctx.macLen = index_mac.size();
	pack_ctx.indexEncSize = indexEncSize;
	pack_ctx.indexPlainSize = indexPlainSize;
	pack_ctx.dataRegionSize = data_region_size;

	if (!ResourceCipher::VerifyHeaderMac(header.data(),
		layout.fixedHeaderBytes - ResourceCipher::kMacLen, headerMac, pack_ctx))
		return false;

	size_t plainLen = 0;
	if (!ResourceCipher::DecryptIndex(indexBuf.data(), indexEncSize, &plainLen, pack_ctx)) return false;
	if (plainLen != indexPlainSize) return false;

	index_plain.swap(indexBuf);
	return parseIndex(index_plain.data(), plainLen);
}

bool ResourcePack::parseIndex(const uint8_t* data, size_t len)
{
	// The decrypted index opens with a name table of its own: a byte length, an entry count,
	// then that many (ordinal, length, name) records, in an order only the packer knows. The
	// entries follow it, and the index carries no value derived from a name anywhere, so a
	// name can only be resolved by getting this far.
	if (len < 8) return false;
	const uint32_t tableBytes = ReadU32(data);
	const uint32_t entryCount = ReadU32(data + 4);
	if (static_cast<size_t>(tableBytes) > len - 8) return false;
	const uint8_t* table = data + 8;
	const uint8_t* entries = table + tableBytes;
	const size_t entriesLen = len - 8 - tableBytes;
	// Every entry costs at least one prefix, so a count above that is a corrupt index rather
	// than something to reserve memory for.
	if (entryCount == 0
		|| static_cast<size_t>(entryCount) > entriesLen / sizeof(ResourceCipher::PackIndexEntryHeader))
		return false;

	std::vector<PackIndexEntry> parsed;
	parsed.reserve(entryCount);

	size_t pos = 0;
	for (uint32_t ordinal = 0; ordinal < entryCount; ++ordinal)
	{
		if (entriesLen - pos < sizeof(ResourceCipher::PackIndexEntryHeader)) return false;

		ResourceCipher::PackIndexEntryHeader h{};
		memcpy(&h, entries + pos, sizeof(h));
		pos += sizeof(h);

		const size_t tailLen = static_cast<size_t>(h.saltLen) + h.prevChainLen
			+ h.paramsLen + h.macLen;
		if (entriesLen - pos < tailLen) return false;
		if (h.macLen != ResourceCipher::kMacLen) return false;
		if (h.saltLen == 0 || h.saltLen > ResourceCipher::kMaxKeyPartLen) return false;
		if (h.prevChainLen == 0 || h.prevChainLen > ResourceCipher::kMaxKeyPartLen) return false;
		if (h.entryKeyPartLen != layout.entryKeyPartLen) return false;
		if (h.slot >= ResourceCipher::SlotCount()) return false;
		if (h.encSize == 0 || h.encSize > ResourceCipher::kMaxEntrySize) return false;
		if ((h.encSize % ResourceCipher::kAesBlockLen) != 0) return false;
		if (h.rawSize > h.encSize) return false;
		if (static_cast<uint64_t>(h.offset) + layout.entryKeyPartLen + h.encSize > data_region_size)
			return false;

		PackIndexEntry entry{};
		entry.tailOffset = static_cast<uint32_t>(entries - data + pos);
		entry.dataOffset = h.offset;
		entry.entryIndex = ordinal;
		entry.encSize = h.encSize;
		entry.rawSize = h.rawSize;
		entry.parentInfo = h.parentInfo;
		entry.slot = h.slot;
		entry.saltLen = h.saltLen;
		entry.prevChainLen = h.prevChainLen;
		entry.paramsLen = h.paramsLen;
		entry.macLen = h.macLen;
		parsed.push_back(entry);

		pos += tailLen;
	}

	// Bind the names. The table is the only place a name appears, so a duplicate would make
	// two entries unreachable; the packer refuses those, and emplace keeps the first here.
	size_t tpos = 0;
	for (uint32_t i = 0; i < entryCount; ++i)
	{
		if (tableBytes - tpos < 6) return false;
		const uint32_t ordinal = ReadU32(table + tpos);
		tpos += 4;
		const uint16_t nameLen = ReadU16(table + tpos);
		tpos += 2;
		if (nameLen == 0 || nameLen > ResourceCipher::kMaxNameLen) return false;
		if (tableBytes - tpos < nameLen) return false;
		if (ordinal >= entryCount) return false;
		index_map.emplace(std::string(reinterpret_cast<const char*>(table + tpos), nameLen),
			parsed[ordinal]);
		tpos += nameLen;
	}

	return entryCount > 0;
}

std::unique_ptr<uint8_t[]> ResourcePack::getFileData(const char* name, size_t nameLen,
	size_t* out_size, bool debugLog)
{
	if (!file_stream.is_open() || !name || nameLen == 0) return nullptr;

	auto it = index_map.find(std::string(name, nameLen));
	if (it == index_map.end()) return nullptr;

	const PackIndexEntry& entry = it->second;
	const uint8_t* tail = tailPtr(entry);
	const uint8_t* salt = tail;
	const uint8_t* prevChain = salt + entry.saltLen;
	const uint8_t* params = prevChain + entry.prevChainLen;
	const uint8_t* mac = params + entry.paramsLen;

	const size_t region = layout.entryKeyPartLen + entry.encSize;
	std::vector<uint8_t> buffer(region);

	const uint64_t dataOffset = data_region_offset + entry.dataOffset;
	file_stream.clear();
	file_stream.seekg(static_cast<std::streamoff>(dataOffset));
	if (!file_stream) return nullptr;
	file_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(region));
	if (file_stream.gcount() != static_cast<std::streamsize>(region)) return nullptr;

	ResourceCipher::FileContext file{};
	file.slot = entry.slot;
	file.entryIndex = entry.entryIndex;
	file.parentInfo = entry.parentInfo;
	file.offset = entry.dataOffset;
	file.encSize = entry.encSize;
	file.rawSize = entry.rawSize;
	file.entryKeyPart = buffer.data();
	file.entryKeyPartLen = layout.entryKeyPartLen;
	file.salt = salt;
	file.saltLen = entry.saltLen;
	file.prevChain = prevChain;
	file.prevChainLen = entry.prevChainLen;
	file.params = params;
	file.paramsLen = entry.paramsLen;
	file.entryMac = mac;
	file.entryMacLen = entry.macLen;
	file.cipher = buffer.data() + layout.entryKeyPartLen;
	file.cipherLen = entry.encSize;

	size_t plainLen = 0;
	if (!ResourceCipher::DecryptPackFile(name, nameLen, pack_ctx, file,
		buffer.data() + layout.entryKeyPartLen, entry.encSize, &plainLen))
		return nullptr;
	if (plainLen < entry.rawSize) return nullptr;

	std::unique_ptr<uint8_t[]> result(new uint8_t[entry.rawSize ? entry.rawSize : 1]);
	if (entry.rawSize)
		memcpy(result.get(), buffer.data() + layout.entryKeyPartLen, entry.rawSize);
	if (out_size)
		*out_size = entry.rawSize;
#ifndef NDEBUG
	if (debugLog)
		Logger::Raw("Loaded from ResourcePack, file_path = [%s], data_offset = [%d]. ",
			file_path, static_cast<int>(dataOffset));
#endif
	return result;
}

bool ResourcePack::hasFile(const char* name, size_t nameLen) const
{
	if (!name || nameLen == 0) return false;
	return index_map.find(std::string(name, nameLen)) != index_map.end();
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
	const std::string name = nameOf(filename);
	if (name.empty()) return nullptr;

	for (size_t i = 0; i < packs.size(); ++i) {
		auto data = packs[i]->getFileData(name.data(), name.size(), out_size, true);
		if (data)
		{
			return data;
		}
	}
	return nullptr;
}

bool ResourcePackManager::hasFile(const FString& filename)
{
	const std::string name = nameOf(filename);
	if (name.empty()) return false;

	for (size_t i = 0; i < packs.size(); ++i) {
		if (packs[i]->hasFile(name.data(), name.size()))
		{
			return true;
		}
	}
	return false;
}

std::string ResourcePackManager::nameOf(const FString& filename)
{
	// The packer lower cases a name before it writes it into the table, so match that here.
	// Nothing else is needed to look a name up now: the index table is the lookup, and it
	// arrives decrypted with the index.
	FString lower = filename;
	lower.MakeLower();
	return std::string(lower.c_str());
}

void ResourcePackManager::clear() 
{
	packs.clear();
}
