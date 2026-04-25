#include "Body.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/FString.h"

//DEFINE_HOOK(449470, CHouses_UpdateComboboxContents, 6)
//{
//    GET(CHousesExt*, pThis, ECX);
//    pThis->UpdateComboboxContents();
//    return 0x44A308;
//}

DEFINE_HOOK(44E191, CHouses_NoHouseExists, 9)
{
    GET(CHousesExt*, pThis, EBP);

    pThis->MessageBoxA(
        Translations::TranslateOrDefault("HouseDialog.NoHousesExist",
            "No houses do exist, if you want to use houses, you should use Prepare houses before doing anything else."
            ));

    return 0x44E1A1;
}

DEFINE_HOOK(44BAB9, CHouses_AlreadyHouses, 9)
{
    GET(CHousesExt*, pThis, EBP);

    pThis->MessageBoxA(
        Translations::TranslateOrDefault("HouseDialog.AlreadyHouses",
            "There are already houses in your map. You need to delete these first."
            ));

    return 0x44BBE3;
}

DEFINE_HOOK_AGAIN(44BDAC, CHouses_AddHouse_NotAvailable_1, 5)
DEFINE_HOOK(44BC9A, CHouses_AddHouse_NotAvailable_1, 5)
{
    GET(CHousesExt*, pThis, EBX);
    GET_STACK(const char*, lpHouse, STACK_OFFS(0x1F8, -0x4));

    FString message = Translations::TranslateOrDefault("HouseDialog.NotAvailable", 
        "Sorry this name is not available. %s is already used in the map file. You need to use another name.");
    message.Format(message, lpHouse);

    pThis->MessageBoxA(message);

    return 0x44E05E;
   
}