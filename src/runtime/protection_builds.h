#pragma once
// ============================================================================
// protection_builds.h
// Anti-Crit (Hard Head) — runtime config only (no hardcoded game offsets)
// ============================================================================
//
// hh_builds       buildId:pointerRva (hex, comma-separated)
// hh_chain_roots  buildId:chainRootRva (hex, comma-separated)
// hh_config       patchOff-installByte-jmpByte-prologueSave-trampolineSize
// hh_cave         caveAlloc-caveExecOff-templateSize-enableOff-ringCap-ringCapOff
// hh_tpl_patch    counterOff-chainOff-prologueOff-resumeOff
// hh_probe        slotScan-sigPatternLen-prologueBytes-trampolineBytes
// hh_slots        pointerSlotRva-patchRva-critJccOff
// hh_b3258        min FiveM build number (e.g. 3258)

#include <cstdint>
#include <cstddef>

struct AntiCritBuildEntry
{
    std::uint32_t buildId;
    std::uintptr_t pointerRva;
};

namespace ac_vars
{

struct ChainRootEntry
{
    std::uint32_t buildId;
    std::uintptr_t chainRootRva;
};

struct HhConfig
{
    std::uintptr_t patch_offset;
    std::uint8_t install_byte;
    std::uint8_t patch_jmp_byte;
    std::size_t prologue_save_bytes;
    std::size_t trampoline_size;

    std::size_t cave_alloc_size;
    std::uintptr_t cave_exec_off;
    std::size_t cave_template_size;
    std::uintptr_t cave_enable_off;
    std::uint32_t cave_ring_cap;
    std::uintptr_t cave_ring_cap_off;

    std::uintptr_t tpl_off_counter;
    std::uintptr_t tpl_off_chain;
    std::uintptr_t tpl_off_prologue;
    std::uintptr_t tpl_off_resume;

    std::size_t slot_scan_bytes;
    std::size_t sig_pattern_len;
    std::uint8_t probe_prologue[3];
    std::uint8_t probe_trampoline[2];

    std::uintptr_t pointer_slot_rva;
    std::uintptr_t patch_rva;
    std::uintptr_t crit_jcc_offset_in_slot;

    int fivem_build_min;
};

struct Vars
{
    AntiCritBuildEntry *builds;
    size_t build_count;
    ChainRootEntry *chain_roots;
    size_t chain_root_count;
    std::uintptr_t hook_base_rva;
    std::uintptr_t chain_root_default;
    const std::uint8_t *cave_template;
    size_t cave_template_size;
    HhConfig cfg;
};

using GlobalLookupFn = const char *(*)(const char *key, void *user);

Vars &Get();
bool Init();
bool LoadFromFlatGlobals(GlobalLookupFn fn, void *user);
bool IsConfigured();
// "" when the last load succeeded, otherwise "<stage>:<panel global>"
// stage is one of missing / parse / validate.
const char *LastLoadError();

const AntiCritBuildEntry *FindBuild(std::uint32_t buildId);
const AntiCritBuildEntry *GetLatestBuild();
std::uintptr_t ResolveChainRootRva(std::uint32_t buildId);
std::uintptr_t GetHookBaseRva();
std::uintptr_t GetChainRootDefault();
const std::uint8_t *GetCaveTemplate();
size_t GetCaveTemplateSize();
const HhConfig &GetConfig();

} // namespace ac_vars
