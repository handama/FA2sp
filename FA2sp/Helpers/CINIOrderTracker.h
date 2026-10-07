#pragma once

#include <MFC/ppmfc_cstring.h>
#include <unordered_map>
#include "SequencedKeyList.h"

class CINI;

class CINIOrderTracker
{
public:
    static void RecordSection(CINI* ini, const ppmfc::CString& section);
    static void RecordKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key);
    static void RemoveKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key);
    static void RemoveSection(CINI* ini, const ppmfc::CString& section);
    static void Clear(CINI* ini);
    static const SequencedKeyList* GetSectionOrder(CINI* ini);
    static const SequencedKeyList* GetKeyOrder(CINI* ini, const ppmfc::CString& section);
    static SequencedKeyList* GetOrCreateKeyOrder(CINI* ini, const ppmfc::CString& section);
};
