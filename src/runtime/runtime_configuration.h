#pragma once
#include <cstring>
#include <cstddef>

// User-supplied configuration. Loaded locally; no login or server lookup.
namespace local_config
{
struct Entry
{
    const char *key;
    const char *value;
};
static const Entry entries[] = {
    {"anchor", "36-2CD-9EDD8C-25B14B0"},
    {"layout", "0-D-80-8D-100-200-0-4-10-18-20-28-30-38-40-48-4C-50-54-59-5F-1000"},
    {"struct_offsets", "14-24-28-2C-30-38"},
    {"misc", "B12CAFE1-0-1"},
    {"hook3", "4A,8B,04,F8"},
    {"thresholds", "0.45:0.55:0.80"},
    {"struct_low", "0.45:0.21:0.21:0.21:0.15"},
    {"struct_medium", "0.55:0.21:0.21:0.21:0.15"},
    {"struct_high", "0.80:0.21:0.21:0.21:0.15"},
    {"mode_low", "0.45:0.21:0.21:0.21:0.15"},
    {"mode_medium", "0.55:0.21:0.21:0.21:0.15"},
    {"mode_high", "0.80:0.21:0.21:0.21:0.15"},
    {"hook_table", "0:9EDD8C:25B14B0,2699:9DB214:0,2802:9E1C32:0,2944:9E317E:0,3095:9EA144:0,3258:9EDD8C:"
                   "25B14B0,3323:9EE31A:0,3407:9F6132:0,3570:9F7932:0"},
    {"rbp38", "0:102EA94,2699:100D7F9,2802:100B22A,2944:1018215,3095:10236CD,3258:102EA94,3323:103B0F8,3407:"
              "104442C,3570:1041AC8"},
    // Partial Hard Head configuration. Missing keys intentionally return nullptr.
    // The existing loader keeps Hard Head unconfigured until all keys are supplied.
    {"hh_hook_base", "11546DC"},
    {"hh_chain_root", "25B14B0"},
    {"hh_chain_roots", "A8B:26684D8,C17:2593320,D4F:25D7108"},
    {"hh_config", "E-90-EB-F-E"},
    {"hh_slots", "CDAD0-11546EA-18"},
};

// These partial cave values cannot form a complete hh_cave entry yet.
static constexpr std::size_t hard_head_cave_alloc_size = 0x42000;
static constexpr std::size_t hard_head_cave_exec_offset = 0x41000;
// Reference signatures supplied by the user (not substituted into runtime code).
static constexpr unsigned char hook1_signature[] = {0xF3, 0x0F, 0x10, 0x42, 0x28};
static constexpr unsigned char hook2_signature[] = {0xF3, 0x0F, 0x10, 0x4A, 0x38};
// RunSpeed's build table, 1.07f default, 30ms interval and PlayerInfo offsets
// already match memory_controller.cpp. Keep hook_table's auto-anchor zeros unchanged.

inline const char *Lookup(const char *key, void *)
{
    if (!key)
        return nullptr;
    for (const Entry &entry : entries)
        if (std::strcmp(entry.key, key) == 0)
            return entry.value;
    return nullptr;
}
} // namespace local_config
