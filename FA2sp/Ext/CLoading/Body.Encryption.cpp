// Resource pack cipher implementation.
//
// This repository keeps an empty implementation: encrypted packages will not load
// until the file produced by generator.exe is copied here and the project rebuilt.
//
// Do not commit the generated version.
//
// RESOURCE_CIPHER_PLACEHOLDER
#include "Body.Cipher.h"

#include <cstring>

namespace ResourceCipher
{
	bool Initialize()
	{
		return false;
	}

	uint32_t SlotCount()
	{
		return 0;
	}

	bool GetLayoutInfo(uint16_t variant, LayoutInfo* out)
	{
		(void)variant;
		if (out)
			std::memset(out, 0, sizeof(*out));
		return false;
	}

	bool LookupPackBinding(const uint8_t packId[kPackIdLen], uint32_t* outBindingIndex)
	{
		(void)packId;
		if (outBindingIndex)
			*outBindingIndex = 0;
		return false;
	}

	bool VerifyHeaderMac(const uint8_t* header, size_t headerLen,
		const uint8_t* storedMac, const PackContext& ctx)
	{
		(void)header;
		(void)headerLen;
		(void)storedMac;
		(void)ctx;
		return false;
	}

	bool DecryptIndex(uint8_t* buf, size_t len, size_t* outPlainLen, const PackContext& ctx)
	{
		(void)buf;
		(void)len;
		(void)ctx;
		if (outPlainLen)
			*outPlainLen = 0;
		return false;
	}

	bool DecryptPackFile(const char* name, size_t nameLen,
		const PackContext& pack, const FileContext& file,
		uint8_t* buf, size_t len, size_t* outPlainLen)
	{
		(void)name;
		(void)nameLen;
		(void)pack;
		(void)file;
		(void)buf;
		(void)len;
		if (outPlainLen)
			*outPlainLen = 0;
		return false;
	}
}
