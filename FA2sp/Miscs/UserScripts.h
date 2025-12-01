#pragma once
#include <CINI.h>
#include <CFinalSunApp.h>
#include <vector>
#include <unordered_map>

class UserScriptExt
{
public:
    static int ParamCount;
    static bool EditVaribale;
    static bool EditParams;
    static bool NeedRedraw;
    static std::unordered_map<ppmfc::CString, ppmfc::CString> VariablePool;
    static std::vector<ppmfc::CString> Temps;
    static ppmfc::CString ParamsTemp[10];

    static bool IsValSet(ppmfc::CString val)
    {
        val.MakeLower();
        if (val == "false" || val == "no") return false;
        if (val == "true" || val == "yes") return true;
        if (atoi(val)) return true;
        return false;
    }

    static ppmfc::CString GetParam(ppmfc::CString* params, int index) {
        if (index < 0 || index >= ParamCount) return "";
        return *(params + index);
    }
};
