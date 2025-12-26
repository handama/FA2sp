#include <CMixFile.h>
#include <CShpFile.h>
#include <CLoading.h>
#include <FAMemory.h>
#include <CFinalSunApp.h>

#include <fstream>

#include <Helpers/Macro.h>
#include "../RunTime.h"
#include "../Ext/CLoading/Body.h"
#include "../Helpers/STDHelpers.h"

namespace hooks_files_detail
{
	struct shape_file
	{
		bool is_open() const
		{
			return m_data != nullptr;
		}

		void close()
		{
			if (m_data)
			{
				GameDeleteArray(m_data, size);
				m_data = nullptr;
			}
		}
		
		template <typename T>
		T* ptr(size_t offset) const
		{
			if (offset + sizeof(T) > size)
				return nullptr;

			return reinterpret_cast<T*>(m_data + offset);
		}

		unsigned char* m_data;
		size_t size;
	};
	static shape_file current_shape_file;

	unsigned char empty_shape[32] = {
		0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

}
using namespace hooks_files_detail;

static void Decode3(const byte* s, byte* d, int cx, int cy)
{
	const byte* r = s;
	const byte* dataBegin = current_shape_file.m_data;
	const byte* dataEnd = current_shape_file.m_data + current_shape_file.size;
	byte* w = d;
	int total = cx * cy;
	int written = 0;

	for (int y = 0; y < cy; y++) {
		if (r + 2 > dataEnd) break;

		int count = *reinterpret_cast<const unsigned __int16*>(r) - 2;
		r += 2;
		int x = 0;

		while (count-- > 0) {
			if (r >= dataEnd) break;
			int v = *r++;

			if (v) {
				if (written < total) {
					*w++ = v;
					written++;
				}
				x++;
			}
			else {
				if (--count < 0) break;
				if (r >= dataEnd) break;
				v = *r++;

				if (x + v > cx) v = cx - x;
				x += v;

				int remain = total - written;
				if (v > remain) v = remain;
				for (int i = 0; i < v; i++) {
					*w++ = 0;
					written++;
				}
			}
		}
	}
}

DEFINE_HOOK(52D280, CSHPFile_Decode3, 5)
{
	GET_STACK(const byte*, s, 0x4);
	GET_STACK(byte*, d, 0x8);
	GET_STACK(int, cx, 0xC);
	GET_STACK(int, cy, 0x10);

	Decode3(s, d, cx, cy);

	return 0x52D330;
}

DEFINE_HOOK(526020, CMixFile_LoadFrame, 8)
{
	GET_STACK(int, iImageIndex, 0x4);
	GET_STACK(int, iCount, 0x8);
	GET_STACK(BYTE**, lpPics, 0xC);

	if (!current_shape_file.is_open())
	{
		R->EAX(FALSE);
		return 0x5261D9;
	}

	const byte* dataEnd = current_shape_file.m_data + current_shape_file.size;

	ShapeImageHeader imghead{};
	BYTE* image = NULL;

	ShapeHeader head{};
	CShpFile::GetSHPHeader(&head);

	if (head.Width == 0 || head.Height == 0)
	{
		R->EAX(FALSE);
		return 0x5261D9;
	}

	auto get_image = [&imghead, &dataEnd](int) -> byte*
	{
		auto addr = current_shape_file.ptr<byte>(imghead.Offset);
		size_t need = size_t(imghead.Width) * imghead.Height;

		if (!(imghead.Compression & 2) && addr + need > dataEnd)
			return nullptr;

		return addr;
	};

	int pic;
	std::vector<byte> decode_image_buffer;
	for (pic = 0; pic < iCount; pic++)
	{
		lpPics[pic] = GameCreateArray<byte>(head.Width * head.Height);
		if (CShpFile::GetSHPImageHeader(iImageIndex + pic, &imghead))
		{
			if (auto data = get_image(iImageIndex + pic))
			{
				if (imghead.Compression & 2)
				{
					decode_image_buffer.resize(imghead.Width * imghead.Height);
					image = decode_image_buffer.data();
					Decode3(data, image, imghead.Width, imghead.Height);
				}
				else
					image = data;

				int i, e;
				for (i = 0; i < head.Width; i++)
				{
					for (e = 0; e < head.Height; e++)
					{
						DWORD dwRead = 0xFFFFFFFF;
						DWORD dwWrite = i + e * head.Width;

						if (i >= imghead.X && e >= imghead.Y && i < imghead.X + imghead.Width && e < imghead.Y + imghead.Height)
							dwRead = (i - imghead.X) + (e - imghead.Y) * imghead.Width;

						size_t imageSize = static_cast<size_t>(dataEnd - image);

						if (dwRead < imageSize)
						{
							lpPics[pic][dwWrite] = image[dwRead];
						}
						else
							lpPics[pic][dwWrite] = 0;
					}
				}
			}
		}
	}

	R->EAX(TRUE);
	return 0x5261D9;
}

DEFINE_HOOK(525C50, CMixFile_LoadSHP, 5)
{
    GET_STACK(const char*, filename, 0x4);
    GET_STACK(int, nMix, 0x8);

    if (current_shape_file.is_open())
		current_shape_file.close();

	DWORD size = 0;
	if (auto pData = CLoadingExt::GetExtension()->ReadWholeFile(filename, &size))
	{
		current_shape_file.size = size;
		current_shape_file.m_data = (unsigned char*)pData;
		R->EAX(true);
		return 0x525CFC;
	}
	
	R->EAX(false);
    return 0x525CFC;
}

DEFINE_HOOK(525A30, CSHPFile_GetHeader, 6)
{
	GET_STACK(ShapeHeader*, pHeader, 0x4);

	if (pHeader && current_shape_file.is_open())
	{
		auto header = current_shape_file.ptr<ShapeHeader>(0);
		if (header)
			*pHeader = *header;
		else
			memset(pHeader, 0, sizeof(ShapeHeader));
		R->EAX(true);
	}
	else
	{
		R->EAX(false);
	}

	return 0x525A5B;
}

DEFINE_HOOK(525A60, CSHPFile_GetImageHeader, 7)
{
	GET_STACK(int, nFrame, 0x4);
	GET_STACK(ShapeImageHeader*, pImageHeader, 0x8);

	if (pImageHeader && nFrame >= 0 && current_shape_file.is_open() && nFrame < *current_shape_file.ptr<short>(0x6))
	{
		auto header = current_shape_file.ptr<ShapeImageHeader>(nFrame * sizeof(ShapeImageHeader) + sizeof(ShapeHeader));
		if (header)
			*pImageHeader = *header;
		else
			memset(pImageHeader, 0, sizeof(ShapeImageHeader));
		R->EAX(true);
	}
	else
	{
		R->EAX(false);
	}

	return 0x525ACB;
}
