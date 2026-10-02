#include <Helpers/Macro.h>

#include <cstring>

#include "Hooks.PreviewFix.h"
#include "../RunTime.h"

// FinalAlert 2 used a MapPreview buffer in CMapData::InitMinimap(0x4C3D40):
// memset(this->MapPreview, 0xFFu, previewHeight * stride);
// If we create a big map, it will probably corrupt all members following it, like all the INI datas
//
// However, the buffer sizes only 0x40000
// As it is a bitmap and as how FA2 paint the minimap, the size a map takes is 2 * W * H * 3 bytes.
// 256 * 256 * 2 * 3 = 0x60000
// This is the preview buffer size we need instead of stupid FA2's 0x40000,
// which limited the map size to 209 * 209 at max
// (209 * 209 * 2 * 3 = 262086, just under 0x40000)
//
// Therefore we just need to replace this buffer, and it will work fine.
// Its size is decided by the size of the map that is currently loaded, see UpdateBuffer().
//
// The instructions of the original exe refer to that buffer in one of these three forms
// (0x80248 is the offset of CMapData::MapPreviewData, and 7ACE40 is its absolute address):
//     lea  reg, [this + 0x80248]
//     lea  reg, [eax + this + 0x80248]
//     mov  reg, 7ACE40
// so we simply rewrite them to point to our own buffer instead.

namespace MapPreviewFix
{
	namespace
	{
		// Register number as how it is encoded in a ModRM byte
		enum RegCode : byte
		{
			RegEAX = 0, RegECX, RegEDX, RegEBX, RegESP, RegEBP, RegESI, RegEDI
		};

		// These op codes are verified against both what the original exe uses and
		// what MSVC generates for the equivalents:
		//     lea reg, [address]        -> 8D <05 | reg << 3> <address>   (6 bytes)
		//     lea reg, [eax + address]  -> 8D <80 | reg << 3> <address>   (6 bytes)
		//     mov reg, offset address   -> <B8 | reg> <address>           (5 bytes)

		byte* PreviewBuffer = nullptr;
		size_t PreviewBufferSize = 0;

		inline void WriteAddress(byte* dest)
		{
			const DWORD address = reinterpret_cast<DWORD>(PreviewBuffer);
			std::memcpy(dest, &address, sizeof(address));
		}

		// lea reg, [PreviewBuffer]
		inline void DoZero(unsigned long addr, RegCode reg)
		{
			byte code[6] = { 0x8D, static_cast<byte>(0x05 | (reg << 3)) };
			WriteAddress(code + 2);
			RunTime::ResetMemoryContentAt(addr - 2, code, sizeof(code));
		}

		// lea reg, [eax + PreviewBuffer]
		// The original instruction takes 7 bytes, so the byte left unused is filled with a nop
		inline void DoReg(unsigned long addr, RegCode reg)
		{
			byte code[7] = { 0x8D, static_cast<byte>(0x80 | (reg << 3)) };
			WriteAddress(code + 2);
			code[6] = 0x90;
			RunTime::ResetMemoryContentAt(addr - 3, code, sizeof(code));
		}

		// mov reg, offset PreviewBuffer
		inline void DoMove(unsigned long addr, RegCode reg)
		{
			byte code[5] = { static_cast<byte>(0xB8 | reg) };
			WriteAddress(code + 1);
			RunTime::ResetMemoryContentAt(addr, code, sizeof(code));
		}

		void ApplyPatches()
		{
			// CMapData::InitMinimap / CMapData::GetMapPreview
			DoZero(0x4C3DC7, RegEDI);
			DoZero(0x4C3DF6, RegEAX);

			DoZero(0x4168B1, RegEAX);
			DoZero(0x45DCC6, RegEAX);
			DoZero(0x45E3C5, RegEAX);
			DoZero(0x4A23C0, RegEAX);
			DoZero(0x4A335F, RegEAX);
			DoZero(0x4A44C2, RegEAX);
			DoZero(0x4A512B, RegEAX);
			DoZero(0x4A6290, RegEAX);
			DoZero(0x4A6A13, RegEAX);
			DoZero(0x4A7FC5, RegEAX);
			DoZero(0x4A8B7A, RegEAX);
			DoZero(0x4A940B, RegEAX);
			DoZero(0x4A9C8A, RegEAX);
			DoZero(0x4B4BB2, RegEAX);
			DoZero(0x4C7843, RegEAX);

			DoReg(0x4169A9, RegEDI);
			DoReg(0x4169B6, RegEBX);
			DoReg(0x45DDC1, RegEBP);
			DoReg(0x45DDCE, RegEBX);
			DoReg(0x45E4BE, RegEBP);
			DoReg(0x45E4CB, RegECX);
			DoReg(0x4A24C0, RegEBP);
			DoReg(0x4A24CD, RegEAX);
			DoReg(0x4A346A, RegEBP);
			DoReg(0x4A3477, RegEDI);
			DoReg(0x4A45B5, RegEBP);
			DoReg(0x4A45C2, RegEAX);
			DoReg(0x4A521E, RegEBP);
			DoReg(0x4A522B, RegEAX);
			DoReg(0x4A6383, RegEBP);
			DoReg(0x4A6390, RegEAX);
			DoReg(0x4A6B0A, RegEBP);
			DoReg(0x4A6B17, RegEAX);
			DoReg(0x4A80B8, RegEBP);
			DoReg(0x4A80C5, RegEBX);
			DoReg(0x4A8C6F, RegEBP);
			DoReg(0x4A8C7C, RegEBX);
			DoReg(0x4A94FB, RegEDI);
			DoReg(0x4A9508, RegEBP);
			DoReg(0x4A9D7F, RegEBP);
			DoReg(0x4A9D8C, RegEBX);
			DoReg(0x4B4CAD, RegEBX);
			DoReg(0x4B4CBA, RegEAX);
			DoReg(0x4BD3C9, RegESI);
			DoReg(0x4BD3DB, RegEDI);
			DoReg(0x4BDACF, RegESI);
			DoReg(0x4BDAE1, RegEDI);
			DoReg(0x4BE61D, RegESI);
			DoReg(0x4BE62F, RegEBP);
			DoReg(0x4BED9F, RegESI);
			DoReg(0x4BEDB1, RegEBP);
			DoReg(0x4BFF91, RegESI);
			DoReg(0x4BFFA3, RegEDI);
			DoReg(0x4C100E, RegESI);
			DoReg(0x4C1020, RegEDI);
			DoReg(0x4C793E, RegESI);
			DoReg(0x4C7952, RegEDI);

			DoZero(0x4BD2AF, RegESI);
			DoZero(0x4BD9B5, RegESI);
			DoZero(0x4BE501, RegEDI);
			DoZero(0x4BEC85, RegEDI);
			DoZero(0x4BFE59, RegESI);
			DoZero(0x4C0EF4, RegESI);

			DoMove(0x4C1DC1, RegECX);
			DoMove(0x4C1CD8, RegEAX);
			DoMove(0x462005, RegECX);
			DoMove(0x461F07, RegEDX);
			DoMove(0x4579F1, RegECX);
			DoMove(0x4578F8, RegEAX);
			DoMove(0x43895D, RegECX);
			DoMove(0x438876, RegEAX);
			DoMove(0x43826B, RegECX);
			DoMove(0x438184, RegEAX);
			DoMove(0x425D37, RegECX);
			DoMove(0x425C4E, RegEAX);
			DoMove(0x425642, RegECX);
			DoMove(0x425559, RegEAX);
		}
	}

	void UpdateBuffer(int width, int height)
	{
		if (width <= 0 || height <= 0)
			return;

		// The preview is a 24bpp bitmap of (2 * width) x height pixels, and FA2 aligns
		// its stride to 4 bytes. This is exactly what CMapData::InitMinimap() will memset.
		size_t stride = static_cast<size_t>(width) * 2 * 3;
		stride = (stride + 3) & ~static_cast<size_t>(3);
		size_t size = stride * static_cast<size_t>(height);

		if (PreviewBuffer && PreviewBufferSize >= size)
			return;

		byte* oldBuffer = PreviewBuffer;
		PreviewBuffer = new byte[size]();
		PreviewBufferSize = size;

		ApplyPatches();

		delete[] oldBuffer;
	}
}
