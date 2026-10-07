# Implementation Plan - Align INI Section & Key Sorting Behavior with YR_RN_Mission_Editor

## Goal Description
In the current repository (`FA2Copy`, branch `enhance/rn-version-alignment`), INI sections and keys use `std::map` (`FAMap<ppmfc::CString, INISection>` for sections and `FAMap<ppmfc::CString, ppmfc::CString, ..., INISectionEntriesComparator>` for key-value pairs). Because `std::map` naturally orders keys using `INISectionEntriesComparator` (shorter strings first, then alphabetical via `strcmp`), saving an INI or map file forces both sections and keys within each section to be reordered.

In the target editor [`YR_RN_Mission_Editor`](file:///D:/Developments/RevengeNow/Codes/YR_RN_Mission_Editor), INI management was refactored (`CIniFileSection` uses sequenced `Container value_pairs` / `std::vector<std::pair<CString, CString>>` and `map value_pos`), preserving the original file key order and appending newly created keys to the end.

Our objective is to:
1. Enable `FA2Copy` to **preserve both INI section sequence and key sequence** when saving, matching `YR_RN_Mission_Editor`.
2. Use an **$O(1)$ Multi-Index design (`std::list` + `std::unordered_map<Key, list::iterator>`)** for both section order and key order to ensure top performance on massive maps (avoiding $O(N^2)$ lookups or $O(N)$ index shifts on deletions).
3. Introduce a configuration option (`SaveMap.PreserveINIKeySorting` / `ExtConfigs::SaveMap_PreserveINIKeySorting`) so that vanilla behavior remains intact for upstream compatibility when disabled.
4. Adapt a dedicated configuration suite for **Revenge Now (RN)** (similar to `Supplementary/MentalOmega`) with this feature enabled, incorporating RN-specific configurations from `YR_RN_Mission_Editor/dist/FinalRevenge`.

---

## Technical Design: Dual-Level `SequencedKeyList`

### 1. `SequencedKeyList` Container
A reusable multi-index ordered set:
```cpp
class SequencedKeyList
{
public:
    using ListType = std::list<ppmfc::CString>;
    using MapType = std::unordered_map<ppmfc::CString, ListType::iterator>;

    void Add(const ppmfc::CString& key);        // O(1)
    bool Remove(const ppmfc::CString& key);     // O(1)
    bool Contains(const ppmfc::CString& key) const; // O(1)
    void Clear();                               // O(1)
    const ListType& GetOrder() const;
private:
    ListType m_order;
    MapType m_lookup;
};
```

### 2. Dual-Level Sidecar Structure in `CINIManager`
For each `CINI*` instance:
- **`SequencedKeyList SectionOrder;`**: Tracks the sequence of sections (`[Header]`, `[Basic]`, `[Triggers]`, etc.).
- **`std::unordered_map<ppmfc::CString, SequencedKeyList> KeyOrder;`**: Tracks the sequence of keys within each section.

### 3. Lifecycle Integration
1. **On Load (`CINIExt::LoadINIExt`)**:
   - Section headers: `SectionOrder.Add(CurrentSectionName)`
   - Key-value pairs: `KeyOrder[CurrentSectionName].Add(key)`
2. **On Modification (`WriteString`)**:
   - `SectionOrder.Add(section)`
   - `KeyOrder[section].Add(key)` (if key already existed, position is unchanged!)
3. **On Deletion (`DeleteKey`, `DeleteSection`)**:
   - `DeleteKey`: `KeyOrder[section].Remove(key)`
   - `DeleteSection`: `KeyOrder.erase(section)`, `SectionOrder.Remove(section)`
4. **On Save (`SaveMapExt::SaveMap` & `CINI_WriteToFile`)**:
   When `SaveMap_PreserveINIKeySorting` is true:
   - Iterate sections in `SectionOrder.GetOrder()`, followed by any untracked sections in `Dict`.
   - For each section, iterate keys in `KeyOrder[section].GetOrder()`, followed by any untracked keys in `EntitiesDictionary`.
   - When false, maintain existing behavior.

---

## Execution Steps

1. **Implement `SequencedKeyList`**:
   - Add helper class in `FA2sp/Helpers/SequencedKeyList.h`.
2. **Integrate with `CINIManager` & `CINIExt`**:
   - Add `SectionOrder` and `KeyOrder` tracking to `CINIManager`.
   - Record order during `CINIExt::LoadINIExt`.
   - Track additions/deletions in `CINI::WriteString`, `DeleteKey`, `DeleteSection`.
3. **Update Map & INI Saving**:
   - Update `SaveMapExt::SaveMap` in `FA2sp/Miscs/SaveMap.cpp`.
   - Update `CINI_WriteToFile` in `FA2sp/Miscs/Hooks.INI.cpp`.
4. **Configuration**:
   - Add `SaveMap_PreserveINIKeySorting` to `FA2sp.h` / `FA2sp.cpp`.
   - Add option translation strings to language INIs.
   - Document in `Supplementary/FAData.ini`.
5. **Revenge Now (RN) Config Preset**:
   - Create `Supplementary/RevengeNow/FAData.ini` adapted from `YR_RN_Mission_Editor/dist/FinalRevenge/FAData.ini`.
   - Enable both `SaveMap.PreserveINISorting` and `SaveMap.PreserveINIKeySorting`.
6. **Build & Verification**:
   - MSBuild compile check.
   - Test case verification of section and key sequencing.
