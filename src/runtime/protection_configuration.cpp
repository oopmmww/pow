#include "protection_builds.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "vendor/xor.h"

namespace ac_vars
{

namespace
{

static constexpr size_t kMaxBuilds = 64;
static constexpr size_t kMaxChainRoots = 16;
static constexpr size_t kCaveTemplateMax = 512;

static Vars g_v{};
static AntiCritBuildEntry g_build_storage[kMaxBuilds];
static ChainRootEntry g_chain_storage[kMaxChainRoots];
static std::uint8_t g_cave_template_storage[kCaveTemplateMax];
static bool g_inited = false;
static bool g_configured = false;
static char g_load_err[192] = {};

// stage = "missing" / "parse" / "validate", key = panel global name (may be null)
static void SetLoadErr(const char *stage, const char *key)
{
    if (key && *key)
        std::snprintf(g_load_err, sizeof(g_load_err), "%s:%s", stage, key);
    else
        std::snprintf(g_load_err, sizeof(g_load_err), "%s", stage);
}

static const char *HexDigits(const char *s)
{
    if (s && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return s + 2;
    return s;
}

static std::uintptr_t ParseHexStr(const char *s)
{
    if (!s || !*s)
        return 0;
    const char *digits = HexDigits(s);
    if (!digits || !*digits)
        return 0;
    char *end = nullptr;
    const unsigned long long v = std::strtoull(digits, &end, 16);
    return end != digits ? static_cast<std::uintptr_t>(v) : 0;
}

static std::vector<std::string> SplitStr(const std::string &s, char delim)
{
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i)
    {
        if (i == s.size() || s[i] == delim)
        {
            if (i > start)
                out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}

static bool ParseByteToken(const char *tok, std::uint8_t &out)
{
    if (!tok || !*tok)
        return false;
    const char *digits = HexDigits(tok);
    if (!digits || !*digits)
        return false;
    char *end = nullptr;
    const unsigned long v = std::strtoul(digits, &end, 16);
    if (end == digits || v > 255)
        return false;
    out = static_cast<std::uint8_t>(v);
    return true;
}

static bool ParseHookBytes(const char *raw, std::uint8_t *out, size_t n)
{
    if (!raw || !*raw)
        return false;
    const std::string s(raw);
    size_t idx = 0;
    size_t pos = 0;
    while (pos <= s.size() && idx < n)
    {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == ','))
            ++pos;
        if (pos >= s.size())
            break;
        const size_t start = pos;
        while (pos < s.size() && s[pos] != ' ' && s[pos] != ',')
            ++pos;
        if (!ParseByteToken(s.substr(start, pos - start).c_str(), out[idx++]))
            return false;
    }
    return idx >= n;
}

static void ApplyBuildTable(Vars &v, const char *raw)
{
    size_t n = 0;
    for (const std::string &entry : SplitStr(raw, ','))
    {
        if (n >= kMaxBuilds)
            break;
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        v.builds[n].buildId = static_cast<std::uint32_t>(ParseHexStr(parts[0].c_str()));
        v.builds[n].pointerRva = ParseHexStr(parts[1].c_str());
        ++n;
    }
    if (n > 0)
        v.build_count = n;
}

static void ApplyChainRoots(Vars &v, const char *raw)
{
    size_t n = 0;
    for (const std::string &entry : SplitStr(raw, ','))
    {
        if (n >= kMaxChainRoots)
            break;
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        v.chain_roots[n].buildId = static_cast<std::uint32_t>(ParseHexStr(parts[0].c_str()));
        v.chain_roots[n].chainRootRva = ParseHexStr(parts[1].c_str());
        ++n;
    }
    if (n > 0)
        v.chain_root_count = n;
}

static bool ApplyCaveTemplate(Vars &v, const char *raw)
{
    if (!raw || !*raw)
        return false;

    std::string compact;
    compact.reserve(std::strlen(raw));
    for (const char *p = raw; *p; ++p)
    {
        const char c = *p;
        if (c == ' ' || c == ',' || c == '-' || c == ':')
            continue;
        compact.push_back(c);
    }
    if (compact.empty())
        return false;

    if (compact.empty() || (compact.size() % 2) != 0 || compact.size() / 2 > kCaveTemplateMax)
        return false;

    std::vector<std::uint8_t> bytes;
    bytes.reserve(compact.size() / 2);
    for (size_t i = 0; i < compact.size(); i += 2)
    {
        char pair[3] = {compact[i], compact[i + 1], '\0'};
        char *end = nullptr;
        const unsigned long value = std::strtoul(pair, &end, 16);
        if (end != pair + 2 || value > 0xff)
            return false;
        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    std::memcpy(g_cave_template_storage, bytes.data(), bytes.size());
    v.cave_template = g_cave_template_storage;
    v.cave_template_size = bytes.size();
    return true;
}

static bool ApplyConfig(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 5)
        return false;
    HhConfig &c = v.cfg;
    c.patch_offset = ParseHexStr(p[0].c_str());
    c.install_byte = static_cast<std::uint8_t>(ParseHexStr(p[1].c_str()));
    c.patch_jmp_byte = static_cast<std::uint8_t>(ParseHexStr(p[2].c_str()));
    c.prologue_save_bytes = static_cast<std::size_t>(ParseHexStr(p[3].c_str()));
    c.trampoline_size = static_cast<std::size_t>(ParseHexStr(p[4].c_str()));
    return c.patch_offset != 0 && c.prologue_save_bytes != 0 && c.trampoline_size != 0;
}

static bool ApplyCave(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 6)
        return false;
    HhConfig &c = v.cfg;
    c.cave_alloc_size = static_cast<std::size_t>(ParseHexStr(p[0].c_str()));
    c.cave_exec_off = ParseHexStr(p[1].c_str());
    c.cave_template_size = static_cast<std::size_t>(ParseHexStr(p[2].c_str()));
    c.cave_enable_off = ParseHexStr(p[3].c_str());
    c.cave_ring_cap = static_cast<std::uint32_t>(ParseHexStr(p[4].c_str()));
    c.cave_ring_cap_off = ParseHexStr(p[5].c_str());
    return c.cave_alloc_size != 0 && c.cave_exec_off != 0 && c.cave_template_size != 0;
}

static bool ApplyTplPatch(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 4)
        return false;
    HhConfig &c = v.cfg;
    c.tpl_off_counter = ParseHexStr(p[0].c_str());
    c.tpl_off_chain = ParseHexStr(p[1].c_str());
    c.tpl_off_prologue = ParseHexStr(p[2].c_str());
    c.tpl_off_resume = ParseHexStr(p[3].c_str());
    return true;
}

static bool ApplyProbe(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 4)
        return false;
    HhConfig &c = v.cfg;
    c.slot_scan_bytes = static_cast<std::size_t>(ParseHexStr(p[0].c_str()));
    c.sig_pattern_len = static_cast<std::size_t>(ParseHexStr(p[1].c_str()));
    if (!ParseHookBytes(p[2].c_str(), c.probe_prologue, 3))
        return false;
    if (!ParseHookBytes(p[3].c_str(), c.probe_trampoline, 2))
        return false;
    return c.slot_scan_bytes != 0;
}

static bool ApplySlots(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 3)
        return false;
    HhConfig &c = v.cfg;
    c.pointer_slot_rva = ParseHexStr(p[0].c_str());
    c.patch_rva = ParseHexStr(p[1].c_str());
    c.crit_jcc_offset_in_slot = ParseHexStr(p[2].c_str());
    return c.pointer_slot_rva != 0 && c.patch_rva != 0;
}

static bool ApplyB3258(Vars &v, const char *raw)
{
    if (!raw || !*raw)
        return false;
    char *end = nullptr;
    const long minBuild = std::strtol(raw, &end, 10);
    if (end == raw || minBuild <= 0)
        return false;
    v.cfg.fivem_build_min = static_cast<int>(minBuild);
    return true;
}

static void ResetStorage(Vars &v)
{
    g_v = {};
    g_v.builds = g_build_storage;
    g_v.chain_roots = g_chain_storage;
    g_v.cfg = {};
}

// Returns nullptr when valid, otherwise the panel global that is bad.
static const char *ValidateLoaded(const Vars &v)
{
    if (v.build_count == 0)
        return "hh_builds";
    if (v.chain_root_count == 0)
        return "hh_chain_roots";
    if (v.hook_base_rva == 0)
        return "hh_hook_base";
    if (v.chain_root_default == 0)
        return "hh_chain_root";
    if (!v.cave_template || v.cave_template_size == 0)
        return "hh_cave_template";
    if (v.cfg.cave_alloc_size == 0)
        return "hh_cave";
    if (v.cfg.cave_template_size == 0)
        return "hh_cave";
    if (v.cfg.fivem_build_min <= 0)
        return "hh_b3258";
    if (v.cave_template_size > v.cfg.cave_template_size)
        return "hh_cave_template";
    return nullptr;
}

} // namespace

Vars &Get()
{
    if (!g_inited)
    {
        ResetStorage(g_v);
        g_inited = true;
    }
    return g_v;
}

bool Init()
{
    if (!g_inited)
    {
        ResetStorage(g_v);
        g_inited = true;
    }
    g_configured = false;
    return true;
}

bool IsConfigured()
{
    return g_configured;
}

bool LoadFromFlatGlobals(GlobalLookupFn fn, void *user)
{
    g_load_err[0] = 0;
    if (!fn)
    {
        SetLoadErr("nolookup", nullptr);
        return false;
    }
    Init();
    ResetStorage(g_v);
    g_configured = false;

    // Values returned by fn() may share a scratch buffer:
    // copy immediately, never hold two picked pointers at once.
    auto pick = [&](const char *key) -> std::string
    {
        const char *val = fn(key, user);
        return (val && *val) ? std::string(val) : std::string();
    };

    // NOTE: these must NOT be xorstr_ pointers stored in static storage — the
    // decrypted buffer lives in a temporary that dies at the end of the
    // initialising statement, leaving dangling keys on every later call.
    const std::string kRequired[] = {
        xorstr_("hh_builds"),      xorstr_("hh_hook_base"),     xorstr_("hh_chain_root"),
        xorstr_("hh_chain_roots"), xorstr_("hh_cave_template"), xorstr_("hh_config"),
        xorstr_("hh_cave"),        xorstr_("hh_tpl_patch"),     xorstr_("hh_probe"),
        xorstr_("hh_slots"),       xorstr_("hh_b3258"),
    };

    for (const std::string &key : kRequired)
    {
        if (pick(key.c_str()).empty())
        {
            SetLoadErr("missing", key.c_str());
            return false;
        }
    }

    ApplyBuildTable(g_v, pick(kRequired[0].c_str()).c_str());
    g_v.hook_base_rva = ParseHexStr(pick(kRequired[1].c_str()).c_str());
    g_v.chain_root_default = ParseHexStr(pick(kRequired[2].c_str()).c_str());
    ApplyChainRoots(g_v, pick(kRequired[3].c_str()).c_str());

    struct Step
    {
        const std::string &key;
        bool (*apply)(Vars &, const char *);
    };
    const Step kSteps[] = {
        {kRequired[4], ApplyCaveTemplate}, {kRequired[5], ApplyConfig}, {kRequired[6], ApplyCave},
        {kRequired[7], ApplyTplPatch},     {kRequired[8], ApplyProbe},  {kRequired[9], ApplySlots},
        {kRequired[10], ApplyB3258},
    };

    for (const Step &step : kSteps)
    {
        if (!step.apply(g_v, pick(step.key.c_str()).c_str()))
        {
            SetLoadErr("parse", step.key.c_str());
            return false;
        }
    }

    if (const char *bad = ValidateLoaded(g_v))
    {
        SetLoadErr("validate", bad);
        return false;
    }

    g_configured = true;
    return true;
}

const char *LastLoadError()
{
    return g_load_err[0] ? g_load_err : "";
}

const AntiCritBuildEntry *FindBuild(std::uint32_t buildId)
{
    if (!g_configured)
        return nullptr;
    Vars &v = Get();
    for (size_t i = 0; i < v.build_count; ++i)
    {
        if (v.builds[i].buildId == buildId)
            return &v.builds[i];
    }
    return nullptr;
}

const AntiCritBuildEntry *GetLatestBuild()
{
    if (!g_configured)
        return nullptr;
    Vars &v = Get();
    if (v.build_count == 0)
        return nullptr;
    return &v.builds[v.build_count - 1];
}

std::uintptr_t ResolveChainRootRva(std::uint32_t buildId)
{
    Vars &v = Get();
    for (size_t i = 0; i < v.chain_root_count; ++i)
    {
        if (v.chain_roots[i].buildId == buildId)
            return v.chain_roots[i].chainRootRva;
    }
    return v.chain_root_default;
}

std::uintptr_t GetHookBaseRva()
{
    return Get().hook_base_rva;
}

std::uintptr_t GetChainRootDefault()
{
    return Get().chain_root_default;
}

const std::uint8_t *GetCaveTemplate()
{
    return Get().cave_template;
}

size_t GetCaveTemplateSize()
{
    return Get().cave_template_size;
}

const HhConfig &GetConfig()
{
    return Get().cfg;
}

} // namespace ac_vars
