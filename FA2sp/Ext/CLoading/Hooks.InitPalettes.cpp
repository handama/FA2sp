#include "Body.h"

#include <CPalette.h>
#include <FAMemory.h>

DEFINE_HOOK(48B020, CLoading_InitPalettes, 7)
{
    GET(CLoadingExt*, pThis, ECX);

    BytePalette::LoadedPaletteCount() = 0;

    auto loadPalette = [&pThis](const char* pName, int& to) -> bool
    {
        if (BytePalette::LoadedPaletteCount() > 0xFF)
            return false;

        if (auto pBuffer = (BytePalette*)pThis->ReadWholeFile(pName))
        {
            for (int i = 0; i < 256; ++i)
            {
                pBuffer->Data[i].red <<= 2;
                pBuffer->Data[i].green <<= 2;
                pBuffer->Data[i].blue <<= 2;
            }
            to = ++BytePalette::LoadedPaletteCount();
            BytePalette::LoadedPalettes[BytePalette::LoadedPaletteCount()] = *pBuffer;
            BytePalette::LoadedPaletteFlag[BytePalette::LoadedPaletteCount()] = TRUE;
            GameDelete(pBuffer);
            return true;
        }

        return false;
    };

    auto& suffixes = TheaterHelpers::GetFileTheaterSuffix();
    ppmfc::CString isoPal = "iso~~~.pal";
    isoPal.Replace("~~~", suffixes['T']);
    loadPalette(isoPal, pThis->PAL_ISOTEM);
    isoPal.Replace("~~~", suffixes['A']);
    loadPalette(isoPal, pThis->PAL_ISOSNO);
    isoPal.Replace("~~~", suffixes['U']);
    loadPalette(isoPal, pThis->PAL_ISOURB);
    isoPal.Replace("~~~", suffixes['N']);
    loadPalette(isoPal, pThis->PAL_ISOUBN);
    isoPal.Replace("~~~", suffixes['L']);
    loadPalette(isoPal, pThis->PAL_ISOLUN);
    isoPal.Replace("~~~", suffixes['D']);
    loadPalette(isoPal, pThis->PAL_ISODES);

    ppmfc::CString unitPal = "unit~~~.pal";
    unitPal.Replace("~~~", suffixes['T']);
    loadPalette(unitPal, pThis->PAL_UNITTEM);
    unitPal.Replace("~~~", suffixes['A']);
    loadPalette(unitPal, pThis->PAL_UNITSNO);
    unitPal.Replace("~~~", suffixes['U']);
    loadPalette(unitPal, pThis->PAL_UNITURB);
    unitPal.Replace("~~~", suffixes['N']);
    loadPalette(unitPal, pThis->PAL_UNITUBN);
    unitPal.Replace("~~~", suffixes['L']);
    loadPalette(unitPal, pThis->PAL_UNITLUN);
    unitPal.Replace("~~~", suffixes['D']);
    loadPalette(unitPal, pThis->PAL_UNITDES);

    loadPalette("temperat.pal", pThis->PAL_TEMPERAT);
    loadPalette("snow.pal", pThis->PAL_SNOW);
    loadPalette("urban.pal", pThis->PAL_URBAN);
    loadPalette("urbann.pal", pThis->PAL_URBANN);
    loadPalette("lunar.pal", pThis->PAL_LUNAR);
    loadPalette("desert.pal", pThis->PAL_DESERT);

    ppmfc::CString libPal = "lib~~~.pal";
    libPal.Replace("~~~", suffixes['T']);
    loadPalette(libPal, pThis->PAL_LIBTEM);

    return 0x48C3CD;
}