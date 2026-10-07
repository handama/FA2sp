#pragma once

#include <list>
#include <unordered_map>
#include <string_view>
#include <functional>
#include <MFC/ppmfc_cstring.h>

#ifndef PPMFC_CSTRING_HASH_DEFINED
#define PPMFC_CSTRING_HASH_DEFINED
namespace std
{
    template <>
    struct hash<ppmfc::CString>
    {
        size_t operator()(const ppmfc::CString& str) const noexcept
        {
            return hash<string_view>()(string_view(str.m_pchData, str.GetLength()));
        }
    };
}
#endif

class SequencedKeyList
{
public:
    using ListType = std::list<ppmfc::CString>;
    using MapType = std::unordered_map<ppmfc::CString, ListType::iterator>;

    bool Add(const ppmfc::CString& key)
    {
        if (m_lookup.find(key) == m_lookup.end())
        {
            m_order.push_back(key);
            m_lookup[key] = std::prev(m_order.end());
            return true;
        }
        return false;
    }

    bool Remove(const ppmfc::CString& key)
    {
        auto it = m_lookup.find(key);
        if (it != m_lookup.end())
        {
            m_order.erase(it->second);
            m_lookup.erase(it);
            return true;
        }
        return false;
    }

    bool Contains(const ppmfc::CString& key) const
    {
        return m_lookup.find(key) != m_lookup.end();
    }

    void Clear()
    {
        m_order.clear();
        m_lookup.clear();
    }

    size_t Size() const
    {
        return m_order.size();
    }

    size_t size() const noexcept
    {
        return m_order.size();
    }

    bool Empty() const
    {
        return m_order.empty();
    }

    bool empty() const noexcept
    {
        return m_order.empty();
    }

    const ListType& GetOrder() const
    {
        return m_order;
    }

    auto begin() const noexcept { return m_order.begin(); }
    auto end() const noexcept { return m_order.end(); }

private:
    ListType m_order;
    MapType m_lookup;
};
