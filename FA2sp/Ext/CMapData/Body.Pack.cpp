#include "Body.h"

#include <Helpers/Macro.h>

#include "../../Logger.h"

#include "../../Algorithms/base64.h"
#include "../../Algorithms/lcw.h"
#include "../../Algorithms/lzo.h"

std::string CMapDataExt::convertToExtendedOverlayPack(const std::string& input) {
	std::string output(input.size() * 2, 0);

	for (size_t i = 0; i < input.size(); ++i) {
		uint16_t value = static_cast<uint16_t>(static_cast<unsigned char>(input[i])); 
		if (value == 0xFF)
			value = 0xFFFF;
		output[i * 2] = static_cast<char>(value & 0xFF); 
		output[i * 2 + 1] = static_cast<char>((value >> 8) & 0xFF); 
	}

	return output;
}

std::string CMapDataExt::convertFromExtendedOverlayPack(const std::string& input) {
	if (input.size() % 2 != 0) {
		throw std::invalid_argument("Input size must be even for 2-byte format");
	}

	std::string output(input.size() / 2, 0); 

	for (size_t i = 0; i < output.size(); ++i) {
		uint16_t value = static_cast<uint16_t>(static_cast<unsigned char>(input[i * 2])) |
			(static_cast<uint16_t>(static_cast<unsigned char>(input[i * 2 + 1])) << 8);
		if (value > 255) {
			value = 255; 
			// throw std::out_of_range("Value exceeds 1-byte range: " + std::to_string(value));
		}
		output[i] = static_cast<char>(value);
	}

	return output;
}

void CMapDataExt::PackExt(bool UpdatePreview, bool Description)
{
	UNREFERENCED_PARAMETER(Description);

	if (FieldDataAllocated)
	{
		Logger::Raw(
			"CMapDataExt::PackExt called!\n"
			"UpdatePreview = %d, Description = %d\n"
			"Erasing sections\n",
			UpdatePreview, Description
		);

		INI.DeleteSection("OverlayPack");
		INI.DeleteSection("OverlayDataPack");
		INI.DeleteSection("IsoMapPack5");
		INI.DeleteSection("Digest");

		bool needNewIniFormat = INI.GetInteger("Basic", "NewINIFormat", 4) > 4;
		if (!needNewIniFormat)
		{
			for (const auto& cellExt : CellDataExts)
			{
				if (cellExt.NewOverlay != 0xffff && cellExt.NewOverlay >= 0xff)
				{
					needNewIniFormat = true;
				}
			}
		}
		if (needNewIniFormat)
			NewINIFormat = 5;
		else
			NewINIFormat = 4;
		INI.WriteString("Basic", "NewINIFormat", STDHelpers::IntToString(NewINIFormat));

		{
			Logger::Raw("Packing overlay\n");
			auto data = std::string(reinterpret_cast<const char*>(NewOverlay), 0x40000 * 2);
			if (needNewIniFormat)
			{
				data = lcw::compress(data.data(), 0x40000 * 2);
			}
			else
			{
				data = convertFromExtendedOverlayPack(data);
				data = lcw::compress(data.data(), 0x40000);
			}
			data = base64::encode(data);
			Logger::Raw("Saving overlay...");
			INI.WriteBase64String("OverlayPack", data.data(), data.length());
			Logger::Raw(" done\n");
		}

		{
			Logger::Raw("Packing overlaydata\n");
			auto data = lcw::compress(OverlayData, 0x40000);
			data = base64::encode(data);
			Logger::Raw("Saving overlaydata...");
			INI.WriteBase64String("OverlayDataPack", data.data(), data.length());
			Logger::Raw(" done\n");
		}

		{
			Logger::Raw("Packing isomappack\n");
			auto data = lzo::compressIsoMapPack5(IsoPackData, IsoPackDataCount * sizeof(IsoMapPack5Entry));
			data = base64::encode(data);
			Logger::Raw("Saving isomappack...");
			INI.WriteBase64String("IsoMapPack5", data.data(), data.length());
			Logger::Raw(" done\n");
		}

		if (UpdatePreview)
		{
			Logger::Raw("Removing old previewpack\n");
			INI.DeleteSection("PreviewPack");
			INI.DeleteSection("Preview");

			Logger::Raw("Packing previewpack\n");
			
			BITMAPINFO info; unsigned char* buffer; int stride;
			GetMapPreview(buffer, &info, stride);

			const int width = info.bmiHeader.biWidth;
			const int height = info.bmiHeader.biHeight;
			auto rawdata = new unsigned char[3 * width * height];
			auto p = buffer;
			for (int x = 0; x < width; ++x)
			{
				for (int y = 0; y < height; ++y)
				{
					const int idx = 3 * (x + width * y);
					auto clr = p + stride * (height - y - 1);
					// RGB <- BGR
					rawdata[idx + 0] = clr[2];
					rawdata[idx + 1] = clr[1];
					rawdata[idx + 2] = clr[0];
				}
				p += 3;
			}
			auto data = lzo::compress(rawdata, 3 * width * height);
			delete[] rawdata;
			data = base64::encode(data);
			Logger::Raw("Saving previewpack...");
			INI.WriteBase64String("PreviewPack", data.data(), data.length());
			
			ppmfc::CString size;
			size.Format("0,0,%d,%d", width, height);
			INI.WriteString("Preview", "Size", size);
			Logger::Raw(" done\n");
		}
	}
}

void CMapDataExt::UnPackExt(CINI& ini, std::vector<IsoMapPack5Entry>& entry)
{
	std::string pack;
	if (auto pSection = ini.GetSection("IsoMapPack5"))
	{
		for (const auto& [k, v] : pSection->GetEntities())
		{
			pack += v.m_pchData;
		}
		pack = base64::decode(pack.data());
		pack = lzo::decompress(pack.data(), pack.size());

		size_t count = pack.size() / sizeof(IsoMapPack5Entry);
		entry.resize(count);
		memcpy(entry.data(), pack.data(), count * sizeof(IsoMapPack5Entry));
	}
}

DEFINE_HOOK(49F7A0, CMapData_Pack, 7)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(bool, UpdatePreview, 0x4);
	GET_STACK(bool, Description, 0x4);

	pThis->PackExt(UpdatePreview, Description);

	return 0x4A1674;
}

DEFINE_HOOK(49EF81, CMapData_UnPack_OverlayData, 8)
{
	auto pThis = CMapDataExt::GetExtension();
	std::memset(pThis->Overlay, 0xff, 0x40000);
	std::memset(pThis->OverlayData, 0x0, 0x40000);
	std::fill(std::begin(CMapDataExt::NewOverlay), std::end(CMapDataExt::NewOverlay), 0xFFFF);

	int mapINIformat = pThis->INI.GetInteger("Basic", "NewINIFormat", 4);
	bool needNewIniFormat = mapINIformat > 4;
	if (needNewIniFormat)
		CMapDataExt::NewINIFormat = mapINIformat;

	std::string ovr = "";
	if (auto pSection = pThis->INI.GetSection("OverlayPack"))
	{
		for (const auto& [k, v] : pSection->GetEntities())
		{
			ovr += v.m_pchData;
		}
		ovr = base64::decode(ovr.data());
		ovr = lcw::decompress(ovr.data(), ovr.size());
		if (needNewIniFormat)
		{
			memcpy(pThis->NewOverlay, ovr.data(), std::min(sizeof(pThis->NewOverlay) * 2, ovr.size()));
		}
		else
		{
			for (size_t i = 0; i < std::min(sizeof(pThis->NewOverlay), ovr.size()); ++i) 
			{
				BYTE overlay = static_cast<BYTE>(ovr[i]);
				pThis->NewOverlay[i] = static_cast<WORD>(overlay == 0xFF ? 0xFFFF : overlay);
			}
		}
	}
	ovr = "";
	if (auto pSection = pThis->INI.GetSection("OverlayDataPack"))
	{
		for (const auto& [k, v] : pSection->GetEntities())
		{
			ovr += v.m_pchData;
		}
		ovr = base64::decode(ovr.data());
		ovr = lcw::decompress(ovr.data(), ovr.size());
		memcpy(pThis->OverlayData, ovr.data(), std::min(sizeof(pThis->OverlayData), ovr.size()));
	}

	return 0x49F440;
}
