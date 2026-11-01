#include "../vendor/api_hidden.h"
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cwchar>
#include <string>
#include <thread>
#include <atomic>
#include "native_memory.h"
#include "process_locator.h"
#include "vendor/xor.h"

struct FpStructPreset
{
    float f14, f24, f28, f2c, f30;
};

namespace fp_err
{
inline const char *ConnectFailServer()
{
    return "เชื่อมต่อเซิร์ฟเวอร์ล้มเหลว";
}
// xorstr_ decrypts into a temporary: it must be copied into storage that
// outlives the expression before the pointer is handed out.
inline const char *ConnectFail()
{
    static const std::string kMsg = xorstr_("เชื่อมต่อ FiveM ล้มเหลว");
    return kMsg.c_str();
}
inline const char *OperationFail()
{
    return "การทำงานล้มเหลว";
}
inline const char *AdminRequired()
{
    return "โปรดเปิดด้วยสิทธิ์ Administrator";
}
inline const char *ConfigMissing()
{
    return "การตั้งค่าไม่ถูกต้อง";
}
inline const char *BuildUnsupported()
{
    return "เวอร์ชันเกมไม่รองรับ";
}
inline const char *Hook3Mismatch()
{
    return "ฮุกไม่ตรงกัน";
}
inline const char *GlobalInvalid()
{
    return "กรุณาเข้าเกมก่อน";
}
} // namespace fp_err

namespace pm
{
inline bool Match(const wchar_t *name, uint32_t *outBuild = nullptr)
{
    if (wcsncmp(name, xorstr_(L"FiveM_"), 6) != 0)
        return false;
    const wchar_t *s = name + 6;
    uint32_t build = 0;
    if (*s == L'b')
    {
        const wchar_t *q = s + 1;
        if (*q < L'0' || *q > L'9')
            return false;
        while (*q >= L'0' && *q <= L'9')
        {
            build = build * 10 + (*q - L'0');
            ++q;
        }
        if (*q != L'_')
            return false;
        s = q + 1;
    }
    const bool gta = wcsncmp(s, xorstr_(L"GTAProcess"), 10) == 0;
    const bool game = wcsncmp(s, xorstr_(L"GameProcess"), 11) == 0;
    if (!gta && !game)
        return false;
    s += gta ? 10 : 11;
    if (wcscmp(s, xorstr_(L".exe")) != 0)
        return false;
    if (outBuild)
        *outBuild = build;
    return true;
}
} // namespace pm

// --- runtime config loaded through LoadFromFlatGlobals ---

namespace fp_vars
{

struct BuildEntry
{
    uint32_t build;
    uintptr_t offset_p1, offset_global;
};
struct Rbp38Entry
{
    uint32_t build;
    uintptr_t hook_rva;
};
struct AllocLayout
{
    uintptr_t cave1_off, val1_off, cave2_off, val2_off, ctrl_off, cave3_off;
    uintptr_t ctrl_magic_off, ctrl_ready_off;
    uintptr_t ctrl_hook1_off, ctrl_hook2_off, ctrl_hook3_off;
    uintptr_t ctrl_cave1_off, ctrl_cave2_off, ctrl_cave3_off;
    uintptr_t ctrl_global_abs_off, ctrl_val1_off, ctrl_val2_off;
    uintptr_t ctrl_hook3_en_off, ctrl_orig1_off, ctrl_orig2_off, ctrl_orig3_tail_off;
    size_t alloc_size;
};
struct ProcessInfo
{
    wchar_t mod_ntdll[32], mod_kernel32[32], mod_game[64];
};
struct SessionAddrs
{
    uintptr_t mod_base, hook_p1, hook_p2, hook_p3, global_rva, global_abs;
    uint32_t build;
    bool resolved;
};

static constexpr size_t kBuildPayloadMax = 192;
static constexpr size_t kMaxBuildPayloads = 16;

struct BuildPayloadEntry
{
    uint32_t build;
    char value[kBuildPayloadMax];
};

struct BuildKeyedStore
{
    BuildPayloadEntry entries[kMaxBuildPayloads];
    size_t count = 0;
    char global[kBuildPayloadMax]{};
    bool has_global = false;
};

struct Vars
{
    BuildEntry *builds;
    size_t build_count;
    Rbp38Entry *rbp38_builds;
    size_t rbp38_count;
    uintptr_t delta, hook3_delta, global_anchor_p1, global_anchor_off;
    uint32_t magic;
    uintptr_t struct_off_14, struct_off_24, struct_off_28, struct_off_2c, struct_off_30, struct_off_38;
    BYTE hook3_bytes[4];
    bool hook3_enable;
    float fp_low, fp_medium, fp_high;
    float mode_f14_low, mode_f14_medium, mode_f14_high;
    FpStructPreset struct_low, struct_medium, struct_high;
    AllocLayout layout;
    ProcessInfo proc;
    SessionAddrs session;
    BuildKeyedStore anchor_map, layout_map, struct_offsets_map, misc_map;
};

Vars &Get();
bool Init();
using GlobalLookupFn = const char *(*)(const char *key, void *user);
bool LoadFromFlatGlobals(GlobalLookupFn fn, void *user);
bool IsConfigured();
// "" when the last load succeeded, otherwise "<stage>:<panel global>"
const char *LastLoadError();
const BuildEntry *FindBuild(uint32_t build);
const Rbp38Entry *FindRbp38(uint32_t build);
bool ResolveSession(uintptr_t mod_base, uint32_t build);

} // namespace fp_vars

#define FP_V (::fp_vars::Get())
#define CTRL_OFF (FP_V.layout.ctrl_off)
#define CAVE3_OFF (FP_V.layout.cave3_off)
#define CTRL_MAGIC_OFF (FP_V.layout.ctrl_magic_off)
#define CTRL_READY_OFF (FP_V.layout.ctrl_ready_off)
#define CTRL_HOOK1_OFF (FP_V.layout.ctrl_hook1_off)
#define CTRL_HOOK2_OFF (FP_V.layout.ctrl_hook2_off)
#define CTRL_HOOK3_OFF (FP_V.layout.ctrl_hook3_off)
#define CTRL_CAVE3_OFF (FP_V.layout.ctrl_cave3_off)
#define CTRL_GLOBAL_ABS_OFF (FP_V.layout.ctrl_global_abs_off)
#define CTRL_HOOK3_EN_OFF (FP_V.layout.ctrl_hook3_en_off)
#define CTRL_ORIG3_TAIL_OFF (FP_V.layout.ctrl_orig3_tail_off)
#define ALLOC_SIZE (FP_V.layout.alloc_size)

namespace fp_vars
{

static Vars g_v;
static BuildEntry g_build_storage[32];
static Rbp38Entry g_rbp38_storage[32];
static bool g_inited = false;
static bool g_configured = false;
static char g_load_err[192] = {};

// stage = "missing" / "parse", key = panel global name (may be null)
static void SetLoadErr(const char *stage, const char *key)
{
    if (key && *key)
        snprintf(g_load_err, sizeof(g_load_err), "%s:%s", stage, key);
    else
        snprintf(g_load_err, sizeof(g_load_err), "%s", stage);
}

static const char *HexDigits(const char *s)
{
    if (s && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return s + 2;
    return s;
}

static bool ParseByteToken(const char *tok, BYTE &out)
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
    out = static_cast<BYTE>(v);
    return true;
}

static bool ParseHookBytes(const char *raw, BYTE *out, size_t n)
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

static void InitDefaults()
{
    g_v = {};
    g_v.builds = g_build_storage;
    g_v.rbp38_builds = g_rbp38_storage;

    wcscpy_s(g_v.proc.mod_ntdll, xorstr_(L"ntdll.dll"));
    wcscpy_s(g_v.proc.mod_kernel32, xorstr_(L"kernel32.dll"));
    g_v.proc.mod_game[0] = 0;
    g_v.session = {};

    g_v.struct_low = {0.45f, 0.21f, 0.21f, 0.21f, 0.15f};
    g_v.struct_medium = {0.55f, 0.21f, 0.21f, 0.21f, 0.15f};
    g_v.struct_high = {0.80f, 0.21f, 0.21f, 0.21f, 0.15f};
    g_v.mode_f14_low = 0.45f;
    g_v.mode_f14_medium = 0.55f;
    g_v.mode_f14_high = 0.80f;
    g_v.fp_low = 0.45f;
    g_v.fp_medium = 0.55f;
    g_v.fp_high = 0.80f;
}

Vars &Get()
{
    if (!g_inited)
    {
        InitDefaults();
        g_inited = true;
    }
    return g_v;
}

bool IsConfigured()
{
    return g_configured;
}

bool Init()
{
    if (!g_inited)
    {
        InitDefaults();
        g_inited = true;
    }
    g_configured = false;
    return true;
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

static bool IsBuildMapFormat(const char *raw)
{
    if (!raw || !*raw)
        return false;
    const std::string s(raw);
    const size_t colon = s.find(':');
    if (colon == std::string::npos || colon == 0)
        return false;
    char *end = nullptr;
    std::strtoul(s.substr(0, colon).c_str(), &end, 10);
    return end && *end == '\0';
}

static std::string JoinFromIndex(const std::vector<std::string> &parts, size_t startIdx)
{
    if (startIdx >= parts.size())
        return {};
    std::string out = parts[startIdx];
    for (size_t i = startIdx + 1; i < parts.size(); ++i)
    {
        out += ':';
        out += parts[i];
    }
    return out;
}

static bool StoreBuildKeyed(BuildKeyedStore &store, const char *raw)
{
    store = {};
    if (!raw || !*raw)
        return false;

    if (!IsBuildMapFormat(raw))
    {
        strncpy_s(store.global, raw, _TRUNCATE);
        store.has_global = true;
        return true;
    }

    for (const std::string &entry : SplitStr(raw, ','))
    {
        if (store.count >= kMaxBuildPayloads)
            break;
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        char *end = nullptr;
        const unsigned long build = std::strtoul(parts[0].c_str(), &end, 10);
        if (end == parts[0].c_str())
            continue;
        const std::string payload = JoinFromIndex(parts, 1);
        if (payload.empty())
            continue;
        store.entries[store.count].build = static_cast<uint32_t>(build);
        strncpy_s(store.entries[store.count].value, payload.c_str(), _TRUNCATE);
        ++store.count;
    }
    return store.count > 0;
}

static const char *ResolveBuildKeyed(const BuildKeyedStore &store, uint32_t build)
{
    for (size_t i = 0; i < store.count; ++i)
    {
        if (store.entries[i].build == build)
            return store.entries[i].value;
    }
    return store.has_global ? store.global : nullptr;
}

static bool ApplyAnchor(Vars &v, const char *raw);
static bool ApplyLayout(Vars &v, const char *raw);
static bool ApplyStructOffsets(Vars &v, const char *raw);
static bool ApplyMiscFields(Vars &v, const char *raw);

static void ActivateBuildKeyedGlobals(Vars &v, uint32_t build)
{
    if (const char *p = ResolveBuildKeyed(v.anchor_map, build))
        ApplyAnchor(v, p);
    if (const char *p = ResolveBuildKeyed(v.layout_map, build))
        ApplyLayout(v, p);
    if (const char *p = ResolveBuildKeyed(v.struct_offsets_map, build))
        ApplyStructOffsets(v, p);
    if (const char *p = ResolveBuildKeyed(v.misc_map, build))
        ApplyMiscFields(v, p);
}

static uintptr_t ParseHexStr(const char *s)
{
    if (!s || !*s)
        return 0;
    const char *digits = HexDigits(s);
    if (!digits || !*digits)
        return 0;
    char *end = nullptr;
    const unsigned long long v = std::strtoull(digits, &end, 16);
    return end != digits ? static_cast<uintptr_t>(v) : 0;
}

static void ApplyHookTable(Vars &v, const char *raw)
{
    size_t n = 0;
    for (const std::string &entry : SplitStr(raw, ','))
    {
        if (n >= 32)
            break;
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        v.builds[n].build = static_cast<uint32_t>(std::strtoul(parts[0].c_str(), nullptr, 10));
        v.builds[n].offset_p1 = ParseHexStr(parts[1].c_str());
        v.builds[n].offset_global = parts.size() >= 3 ? ParseHexStr(parts[2].c_str()) : 0;
        ++n;
    }
    if (n > 0)
        v.build_count = n;
}

static void ApplyBuildRvaTable(Vars &v, const char *raw)
{
    size_t n = 0;
    for (const std::string &entry : SplitStr(raw, ','))
    {
        if (n >= 32)
            break;
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        v.rbp38_builds[n].build = static_cast<uint32_t>(std::strtoul(parts[0].c_str(), nullptr, 10));
        v.rbp38_builds[n].hook_rva = ParseHexStr(parts[1].c_str());
        ++n;
    }
    if (n > 0)
        v.rbp38_count = n;
}

static void ApplyWorldRva(Vars &v, const char *raw)
{
    for (const std::string &entry : SplitStr(raw, ','))
    {
        const auto parts = SplitStr(entry, ':');
        if (parts.size() < 2)
            continue;
        const uint32_t build = static_cast<uint32_t>(std::strtoul(parts[0].c_str(), nullptr, 10));
        const uintptr_t global = ParseHexStr(parts[1].c_str());
        for (size_t i = 0; i < v.build_count; ++i)
        {
            if (v.builds[i].build == build)
                v.builds[i].offset_global = global;
        }
    }
}

static bool ApplyAnchor(Vars &v, const char *raw)
{
    const auto parts = SplitStr(raw, '-');
    if (parts.size() < 4)
        return false;
    v.delta = ParseHexStr(parts[0].c_str());
    v.hook3_delta = ParseHexStr(parts[1].c_str());
    v.global_anchor_p1 = ParseHexStr(parts[2].c_str());
    v.global_anchor_off = ParseHexStr(parts[3].c_str());
    return true;
}

static bool ApplyLayout(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 21)
        return false;
    AllocLayout &l = v.layout;
    l.cave1_off = ParseHexStr(p[0].c_str());
    l.val1_off = ParseHexStr(p[1].c_str());
    l.cave2_off = ParseHexStr(p[2].c_str());
    l.val2_off = ParseHexStr(p[3].c_str());
    l.ctrl_off = ParseHexStr(p[4].c_str());
    l.cave3_off = ParseHexStr(p[5].c_str());
    l.ctrl_magic_off = ParseHexStr(p[6].c_str());
    l.ctrl_ready_off = ParseHexStr(p[7].c_str());
    l.ctrl_hook1_off = ParseHexStr(p[8].c_str());
    l.ctrl_hook2_off = ParseHexStr(p[9].c_str());
    l.ctrl_hook3_off = ParseHexStr(p[10].c_str());
    l.ctrl_cave1_off = ParseHexStr(p[11].c_str());
    l.ctrl_cave2_off = ParseHexStr(p[12].c_str());
    l.ctrl_cave3_off = ParseHexStr(p[13].c_str());
    l.ctrl_global_abs_off = ParseHexStr(p[14].c_str());
    l.ctrl_val1_off = ParseHexStr(p[15].c_str());
    l.ctrl_val2_off = ParseHexStr(p[16].c_str());
    l.ctrl_hook3_en_off = ParseHexStr(p[17].c_str());
    l.ctrl_orig1_off = ParseHexStr(p[18].c_str());
    l.ctrl_orig2_off = ParseHexStr(p[19].c_str());
    l.ctrl_orig3_tail_off = ParseHexStr(p[20].c_str());
    if (p.size() >= 22)
        l.alloc_size = static_cast<size_t>(ParseHexStr(p[21].c_str()));
    return l.ctrl_off != 0 && l.cave3_off != 0 && l.alloc_size != 0;
}

static bool ApplyStructOffsets(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 6)
        return false;
    v.struct_off_14 = ParseHexStr(p[0].c_str());
    v.struct_off_24 = ParseHexStr(p[1].c_str());
    v.struct_off_28 = ParseHexStr(p[2].c_str());
    v.struct_off_2c = ParseHexStr(p[3].c_str());
    v.struct_off_30 = ParseHexStr(p[4].c_str());
    v.struct_off_38 = ParseHexStr(p[5].c_str());
    return true;
}

static bool ApplyMiscFields(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, '-');
    if (p.size() < 3)
        return false;
    v.magic = static_cast<uint32_t>(ParseHexStr(p[0].c_str()));
    v.hook3_enable = (p[2] != "0" && p[2] != "false" && p[2] != "False");
    return v.magic != 0;
}

static bool LoadBuildKeyedVar(Vars &v, BuildKeyedStore &store, const char *raw,
                              bool (*apply)(Vars &, const char *))
{
    if (!StoreBuildKeyed(store, raw))
        return false;
    if (store.has_global)
        return apply(v, store.global);
    return store.count > 0;
}

static bool ApplyHookBytes(BYTE *out, size_t n, const char *raw)
{
    return raw && *raw && ParseHookBytes(raw, out, n);
}

static void ApplyThresholds(Vars &v, const char *raw)
{
    const auto p = SplitStr(raw, ':');
    if (p.size() < 3)
        return;
    char *end = nullptr;
    v.fp_low = std::strtof(p[0].c_str(), &end);
    v.fp_medium = std::strtof(p[1].c_str(), &end);
    v.fp_high = std::strtof(p[2].c_str(), &end);
}

static bool ApplyFloatPreset(const char *raw, FpStructPreset &out)
{
    const auto p = SplitStr(raw, ':');
    if (p.size() < 5)
        return false;
    char *end = nullptr;
    out.f14 = std::strtof(p[0].c_str(), &end);
    out.f24 = std::strtof(p[1].c_str(), &end);
    out.f28 = std::strtof(p[2].c_str(), &end);
    out.f2c = std::strtof(p[3].c_str(), &end);
    out.f30 = std::strtof(p[4].c_str(), &end);
    return true;
}

static void ApplyMode(Vars &v, const char *raw, float &modeF14, FpStructPreset &preset)
{
    if (!ApplyFloatPreset(raw, preset))
        return;
    modeF14 = preset.f14;
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
    InitDefaults();
    g_configured = false;

    // Values returned by fn() may share a scratch buffer:
    // copy immediately, never hold two picked pointers at once.
    auto pick = [&](const char *key) -> std::string
    {
        const char *val = fn(key, user);
        return (val && *val) ? std::string(val) : std::string();
    };

    static const char *const kRequired[] = {
        "anchor",      "layout",   "struct_offsets", "misc",       "hook_table",
        "rbp38",       "hook3",    "thresholds",     "struct_low", "struct_medium",
        "struct_high", "mode_low", "mode_medium",    "mode_high",  nullptr,
    };

    for (const char *const *key = kRequired; *key; ++key)
    {
        if (pick(*key).empty())
        {
            SetLoadErr("missing", *key);
            return false;
        }
    }

    struct KeyedStep
    {
        const char *key;
        BuildKeyedStore &store;
        bool (*apply)(Vars &, const char *);
    };
    const KeyedStep kKeyed[] = {
        {"anchor", g_v.anchor_map, ApplyAnchor},
        {"layout", g_v.layout_map, ApplyLayout},
        {"struct_offsets", g_v.struct_offsets_map, ApplyStructOffsets},
        {"misc", g_v.misc_map, ApplyMiscFields},
    };
    for (const KeyedStep &step : kKeyed)
    {
        if (!LoadBuildKeyedVar(g_v, step.store, pick(step.key).c_str(), step.apply))
        {
            SetLoadErr("parse", step.key);
            return false;
        }
    }

    ApplyHookTable(g_v, pick("hook_table").c_str());
    ApplyBuildRvaTable(g_v, pick("rbp38").c_str());
    const std::string worldRva = pick("world_rva");
    if (!worldRva.empty())
        ApplyWorldRva(g_v, worldRva.c_str());

    if (!ApplyHookBytes(g_v.hook3_bytes, 4, pick("hook3").c_str()))
    {
        SetLoadErr("parse", "hook3");
        return false;
    }
    ApplyThresholds(g_v, pick("thresholds").c_str());
    ApplyFloatPreset(pick("struct_low").c_str(), g_v.struct_low);
    ApplyFloatPreset(pick("struct_medium").c_str(), g_v.struct_medium);
    ApplyFloatPreset(pick("struct_high").c_str(), g_v.struct_high);
    ApplyMode(g_v, pick("mode_low").c_str(), g_v.mode_f14_low, g_v.struct_low);
    ApplyMode(g_v, pick("mode_medium").c_str(), g_v.mode_f14_medium, g_v.struct_medium);
    ApplyMode(g_v, pick("mode_high").c_str(), g_v.mode_f14_high, g_v.struct_high);

    if (g_v.build_count == 0)
    {
        SetLoadErr("parse", "hook_table");
        return false;
    }
    if (g_v.rbp38_count == 0)
    {
        SetLoadErr("parse", "rbp38");
        return false;
    }

    g_configured = true;
    return true;
}

const char *LastLoadError()
{
    return g_load_err[0] ? g_load_err : "";
}

const BuildEntry *FindBuild(uint32_t build)
{
    if (!g_configured)
        return nullptr;
    Vars &v = Get();
    for (size_t i = 0; i < v.build_count; ++i)
    {
        if (v.builds[i].build == build)
            return &v.builds[i];
    }
    return nullptr;
}

const Rbp38Entry *FindRbp38(uint32_t build)
{
    Vars &v = Get();
    for (size_t i = 0; i < v.rbp38_count; ++i)
    {
        if (v.rbp38_builds[i].build == build)
            return &v.rbp38_builds[i];
    }
    return nullptr;
}

uintptr_t GlobalRva(const BuildEntry *e)
{
    if (!e)
        return 0;
    if (e->offset_global)
        return e->offset_global;
    Vars &v = Get();
    return v.global_anchor_off + (e->offset_p1 - v.global_anchor_p1);
}

bool ResolveSession(uintptr_t mod_base, uint32_t build)
{
    if (!g_configured)
        return false;

    Vars &v = Get();
    const BuildEntry *e = FindBuild(build);
    if (!e)
    {
        v.session = {};
        return false;
    }

    ActivateBuildKeyedGlobals(v, build);

    v.session.mod_base = mod_base;
    v.session.build = build;
    v.session.hook_p1 = mod_base + e->offset_p1;
    v.session.hook_p2 = v.session.hook_p1 + v.delta;
    v.session.hook_p3 = v.session.hook_p1 + v.hook3_delta;
    v.session.global_rva = GlobalRva(e);
    v.session.global_abs = mod_base + v.session.global_rva;
    v.session.resolved = true;
    return true;
}

} // namespace fp_vars
// --- rbp38 ---

namespace rbp38
{

void Shutdown();

namespace
{

static HANDLE s_hProc = nullptr;
static DWORD s_pid = 0;
static LPVOID s_cave = nullptr;
static uintptr_t s_hook = 0;
static uintptr_t s_ret = 0;
static uintptr_t s_callee = 0;
static uintptr_t s_valAddr = 0;
static uint8_t s_orig[5] = {};
static uint8_t s_rbpDisp = 0x38;
static bool s_active = false;

static uintptr_t LookupHookRva(uint32_t build)
{
    const fp_vars::Rbp38Entry *e = fp_vars::FindRbp38(build);
    return e ? e->hook_rva : 0;
}

static LPVOID AllocNear(HANDLE hProc, uintptr_t target, SIZE_T size)
{
    constexpr uintptr_t step = 0x10000;
    constexpr uintptr_t kMinApp = 0x10000;
    uintptr_t maxAddr = target + 0x7FFF0000;
    uintptr_t minAddr = (target > 0x7FFF0000) ? target - 0x7FFF0000 : kMinApp;

    for (uintptr_t addr = (target & ~(step - 1)); addr < maxAddr;)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!ds::VirtualQueryExNt(hProc, (LPCVOID)addr, &mbi, sizeof(mbi)))
            break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= size)
        {
            uintptr_t a = ((uintptr_t)mbi.BaseAddress + step - 1) & ~(step - 1);
            LPVOID p = ds::VirtualAllocExNt(hProc, (LPVOID)a, size, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE);
            if (p)
                return p;
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    for (uintptr_t addr = (target & ~(step - 1)); addr > minAddr;)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!ds::VirtualQueryExNt(hProc, (LPCVOID)(addr - step), &mbi, sizeof(mbi)))
            break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= size)
        {
            LPVOID p = ds::VirtualAllocExNt(hProc, (LPVOID)mbi.BaseAddress, size, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE);
            if (p)
                return p;
        }
        if ((uintptr_t)mbi.BaseAddress < step)
            break;
        addr = (uintptr_t)mbi.BaseAddress;
    }
    return nullptr;
}

static bool FindLeaRbpDisp(HANDLE hProc, uintptr_t hook, uint8_t &disp)
{
    BYTE buf[64];
    const uintptr_t start = hook > 48 ? hook - 48 : hook;
    const size_t len = (size_t)(hook - start);
    if (!ds::ReadProcessMemoryNt(hProc, (LPCVOID)start, buf, len, nullptr))
        return false;
    for (size_t i = 0; i + 4 <= len; i++)
    {
        if (buf[i] == 0x48 && buf[i + 1] == 0x8D && buf[i + 2] == 0x55)
        {
            disp = buf[i + 3];
            return true;
        }
    }
    return false;
}

static bool WriteJmp(HANDLE hProc, uintptr_t at, uintptr_t dest)
{
    BYTE patch[5] = {0xE9};
    const int32_t rel = (int32_t)((int64_t)dest - (int64_t)(at + 5));
    memcpy(patch + 1, &rel, 4);
    DWORD oldProt = 0;
    if (!ds::VirtualProtectExNt(hProc, (LPVOID)at, 5, PAGE_EXECUTE_READWRITE, &oldProt))
        return false;
    const bool ok = ds::WriteProcessMemoryNt(hProc, (LPVOID)at, patch, 5, nullptr);
    ds::VirtualProtectExNt(hProc, (LPVOID)at, 5, oldProt, &oldProt);
    if (ok)
        ds::FlushInstructionCacheNt(hProc, (LPCVOID)at, 5);
    return ok;
}

static bool ReconstructOrigCall()
{
    if (s_orig[0] == 0xE8 || !s_callee || !s_hook)
        return s_orig[0] == 0xE8;
    s_orig[0] = 0xE8;
    const int32_t rel = (int32_t)(s_callee - (s_hook + 5));
    memcpy(s_orig + 1, &rel, 4);
    return true;
}

static bool AdoptExistingJmp(HANDLE hProc, uintptr_t hook, const BYTE *jmp5, DWORD pid)
{
    int32_t rel = 0;
    memcpy(&rel, jmp5 + 1, 4);
    const uintptr_t cave = hook + 5 + (uintptr_t)(int64_t)rel;

    BYTE c[19] = {};
    if (!ds::ReadProcessMemoryNt(hProc, (LPCVOID)cave, c, sizeof(c), nullptr))
        return false;
    if (c[0] != 0x8B || c[1] != 0x05)
        return false;
    if (c[6] != 0x89 || c[7] != 0x45)
        return false;
    if (c[9] != 0xE8 || c[14] != 0xE9)
        return false;

    s_rbpDisp = c[8];
    int32_t callRel = 0, jmpRel = 0;
    memcpy(&callRel, c + 10, 4);
    memcpy(&jmpRel, c + 15, 4);
    s_callee = (cave + 9) + 5 + (uintptr_t)(int64_t)callRel;
    s_ret = (cave + 14) + 5 + (uintptr_t)(int64_t)jmpRel;
    if (s_ret != hook + 5)
        return false;

    s_hook = hook;
    s_cave = (LPVOID)cave;
    s_valAddr = cave + 0x7A;
    s_hProc = hProc;
    s_pid = pid;
    ReconstructOrigCall();
    s_active = true;
    return true;
}

static bool BuildCaveCode(std::vector<BYTE> &out)
{
    const uintptr_t cave = (uintptr_t)s_cave;
    const int32_t dispVal = (int32_t)(0x7A - 6); // mov eax,[cave+0x7A] ends at cave+6

    const int32_t relCall = (int32_t)(s_callee - (cave + 9 + 5));
    const int32_t relJ = (int32_t)(s_ret - (cave + 14 + 5));

    out = {
        0x8B,
        0x05,
        (BYTE)(dispVal),
        (BYTE)(dispVal >> 8),
        (BYTE)(dispVal >> 16),
        (BYTE)(dispVal >> 24),
        0x89,
        0x45,
        s_rbpDisp,
        0xE8,
        (BYTE)(relCall),
        (BYTE)(relCall >> 8),
        (BYTE)(relCall >> 16),
        (BYTE)(relCall >> 24),
        0xE9,
        (BYTE)(relJ),
        (BYTE)(relJ >> 8),
        (BYTE)(relJ >> 16),
        (BYTE)(relJ >> 24),
    };
    return out.size() == 19;
}

static bool RestoreAt(HANDLE hProc, uintptr_t hook)
{
    if (!hProc || !hook)
    {
        Shutdown();
        return true;
    }

    BYTE head[5] = {};
    if (!ds::ReadProcessMemoryNt(hProc, (LPCVOID)hook, head, 5, nullptr))
        return false;

    if (head[0] == 0xE8)
    {
        Shutdown();
        return true;
    }
    if (head[0] != 0xE9)
        return false;

    if (!s_active || s_hook != hook)
    {
        const DWORD pid = ds::GetProcessPidNt(hProc);
        if (!AdoptExistingJmp(hProc, hook, head, pid))
            return false;
    }
    else
    {
        s_hProc = hProc;
    }

    if (s_orig[0] != 0xE8)
        ReconstructOrigCall();
    if (s_orig[0] != 0xE8)
        return false;

    DWORD oldProt = 0;
    ds::VirtualProtectExNt(hProc, (LPVOID)hook, 5, PAGE_EXECUTE_READWRITE, &oldProt);
    ds::WriteProcessMemoryNt(hProc, (LPVOID)hook, s_orig, 5, nullptr);
    ds::VirtualProtectExNt(hProc, (LPVOID)hook, 5, oldProt, &oldProt);
    ds::FlushInstructionCacheNt(hProc, (LPCVOID)hook, 5);
    if (s_cave)
        ds::VirtualFreeExNt(hProc, s_cave, 0, MEM_RELEASE);
    Shutdown();
    return true;
}

} // namespace

bool Install(HANDLE hProc, uintptr_t modBase, uint32_t build)
{
    const uintptr_t hookRva = LookupHookRva(build);
    if (!hookRva)
        return false;

    const uintptr_t hook = modBase + hookRva;
    const DWORD pid = ds::GetProcessPidNt(hProc);

    if (s_active && s_hook == hook && s_pid == pid)
    {
        s_hProc = hProc;
        return true;
    }

    BYTE head[5] = {};
    if (!ds::ReadProcessMemoryNt(hProc, (LPCVOID)hook, head, 5, nullptr))
        return false;

    if (head[0] == 0xE9)
        return AdoptExistingJmp(hProc, hook, head, pid);

    if (s_active && s_hook == hook && s_hProc)
        RestoreAt(s_hProc, hook);

    s_hook = hook;
    s_ret = s_hook + 5;
    memcpy(s_orig, head, 5);
    if (s_orig[0] != 0xE8)
        return false;

    int32_t rel = 0;
    memcpy(&rel, s_orig + 1, 4);
    s_callee = s_hook + 5 + (uintptr_t)(int64_t)rel;

    if (!FindLeaRbpDisp(hProc, s_hook, s_rbpDisp))
        s_rbpDisp = 0x38;

    s_cave = AllocNear(hProc, s_hook, 0x80);
    if (!s_cave)
        return false;

    s_valAddr = (uintptr_t)s_cave + 0x7A;
    int32_t zero = 0;
    ds::WriteProcessMemoryNt(hProc, (LPVOID)s_valAddr, &zero, 4, nullptr);

    std::vector<BYTE> code;
    if (!BuildCaveCode(code))
    {
        ds::VirtualFreeExNt(hProc, s_cave, 0, MEM_RELEASE);
        s_cave = nullptr;
        return false;
    }

    if (!ds::WriteProcessMemoryNt(hProc, s_cave, code.data(), code.size(), nullptr))
    {
        ds::VirtualFreeExNt(hProc, s_cave, 0, MEM_RELEASE);
        s_cave = nullptr;
        return false;
    }
    ds::FlushInstructionCacheNt(hProc, s_cave, code.size());

    if (!WriteJmp(hProc, s_hook, (uintptr_t)s_cave))
    {
        ds::VirtualFreeExNt(hProc, s_cave, 0, MEM_RELEASE);
        s_cave = nullptr;
        return false;
    }

    s_hProc = hProc;
    s_pid = pid;
    s_active = true;
    return true;
}

bool SetValue(int32_t v)
{
    if (!s_active || !s_hProc || !s_valAddr)
        return false;
    return ds::WriteProcessMemoryNt(s_hProc, (LPVOID)s_valAddr, &v, 4, nullptr) != 0;
}

bool RestoreForGame(HANDLE hProc, uintptr_t modBase, uint32_t build)
{
    const uintptr_t hookRva = LookupHookRva(build);
    if (!hookRva)
        return false;
    return RestoreAt(hProc, modBase + hookRva);
}

bool Restore()
{
    if (s_hook && s_hProc)
        return RestoreAt(s_hProc, s_hook);
    Shutdown();
    return true;
}

void Shutdown()
{
    s_hProc = nullptr;
    s_pid = 0;
    s_cave = nullptr;
    s_hook = 0;
    s_ret = 0;
    s_callee = 0;
    s_valAddr = 0;
    memset(s_orig, 0, sizeof(s_orig));
    s_rbpDisp = 0x38;
    s_active = false;
}

bool IsActive()
{
    return s_active;
}

} // namespace rbp38
// --- float_patch ---
//
// Alloc layout: CTRL_OFF control block + CAVE3_OFF hook3 cave

namespace fpatch
{

static HANDLE s_hProc = nullptr;
static DWORD s_pid = 0;
static LPVOID s_alloc = nullptr;
static uintptr_t s_ctrl = 0;
static uintptr_t s_hookAddr3 = 0;
static bool s_hook3On = false;
static uintptr_t s_hook3JmpBack = 0;
static uintptr_t s_globalAbs = 0;
static const char *s_lastErr = nullptr;

static void SetErr(const char *msg)
{
    s_lastErr = msg;
}
static void Shutdown();

// ---------------------------------------------------------------------------
struct CodeBuf
{
    std::vector<BYTE> b;
    size_t size() const
    {
        return b.size();
    }
    void emit(const void *p, size_t n)
    {
        auto *bytes = static_cast<const BYTE *>(p);
        b.insert(b.end(), bytes, bytes + n);
    }
    void u8(BYTE v)
    {
        b.push_back(v);
    }
    void u32(uint32_t v)
    {
        emit(&v, 4);
    }
    void patch_u32(size_t at, uint32_t v)
    {
        memcpy(b.data() + at, &v, 4);
    }
    void patch_rel32_from(size_t at, uint64_t target)
    {
        int32_t rel = (int32_t)((int64_t)target - (int64_t)((uintptr_t)at + 4));
        patch_u32(at, (uint32_t)rel);
    }
};

// ---------------------------------------------------------------------------
static LPVOID AllocNear(HANDLE hProc, uintptr_t target, SIZE_T size)
{
    constexpr uintptr_t step = 0x10000;
    constexpr uintptr_t kMinApp = 0x10000;
    uintptr_t maxAddr = target + 0x7FFF0000;
    uintptr_t minAddr = (target > 0x7FFF0000) ? target - 0x7FFF0000 : kMinApp;

    for (uintptr_t addr = (target & ~(step - 1)); addr < maxAddr;)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!ds::VirtualQueryExNt(hProc, (LPCVOID)addr, &mbi, sizeof(mbi)))
            break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= size)
        {
            uintptr_t a = ((uintptr_t)mbi.BaseAddress + step - 1) & ~(step - 1);
            LPVOID p = ds::VirtualAllocExNt(hProc, (LPVOID)a, size, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE);
            if (p)
                return p;
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    for (uintptr_t addr = (target & ~(step - 1)); addr > minAddr;)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!ds::VirtualQueryExNt(hProc, (LPCVOID)(addr - step), &mbi, sizeof(mbi)))
            break;
        if (mbi.State == MEM_FREE && mbi.RegionSize >= size)
        {
            LPVOID p = ds::VirtualAllocExNt(hProc, (LPVOID)mbi.BaseAddress, size, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_EXECUTE_READWRITE);
            if (p)
                return p;
        }
        if ((uintptr_t)mbi.BaseAddress < step)
            break;
        addr = (uintptr_t)mbi.BaseAddress;
    }
    return nullptr;
}

// cave3 covers 8 bytes (4+4) at P3 then jumps back to hook3+8.
static uintptr_t Hook3JmpBack(uintptr_t hook3)
{
    return hook3 + 8;
}

static bool RestoreHook3(HANDLE hProc, uintptr_t h3)
{
    if (!h3)
        return true;
    BYTE orig8[8] = {
        FP_V.hook3_bytes[0],
        FP_V.hook3_bytes[1],
        FP_V.hook3_bytes[2],
        FP_V.hook3_bytes[3],
        0x48,
        0x89,
        0x45,
        0x00,
    };
    DWORD oldProt = 0;
    if (!ds::VirtualProtectExNt(hProc, (LPVOID)h3, 8, PAGE_EXECUTE_READWRITE, &oldProt))
        return false;
    const bool w = ds::WriteProcessMemoryNt(hProc, (LPVOID)h3, orig8, 8, nullptr);
    ds::VirtualProtectExNt(hProc, (LPVOID)h3, 8, oldProt, &oldProt);
    if (w)
        ds::FlushInstructionCacheNt(hProc, (LPCVOID)h3, 8);
    return w;
}

static const FpStructPreset &StructPresetForMovss(float movssVal)
{
    if (movssVal >= FP_V.fp_high - 0.001f)
        return FP_V.struct_high;
    if (movssVal >= FP_V.fp_medium - 0.001f)
        return FP_V.struct_medium;
    return FP_V.struct_low;
}

// caveBase = absolute address where cave3 will live (needed for correct E9 rel32).
// fields   = 5 floats {f14,f24,f28,f2c,f30}; nullptr â†’ trampoline only.
// r15 filter: 2, 4, 5  (matches CE script).
static std::vector<BYTE> BuildCave3(uintptr_t hook3, const float *fields, uintptr_t jmpBack,
                                    uintptr_t caveBase)
{
    CodeBuf c;
    if (!jmpBack)
        jmpBack = hook3 + 8;

    auto emit_jmp = [&](uintptr_t target)
    {
        size_t pos = c.size();
        c.u8(0xE9);
        c.u32(0);
        int32_t rel = (int32_t)((int64_t)target - (int64_t)(caveBase + pos + 5));
        memcpy(c.b.data() + pos + 1, &rel, 4);
    };

    // mov rax,[rax+r15*8]   (original insn #1 â€” 4 bytes)
    c.u8(0x4A);
    c.u8(0x8B);
    c.u8(0x04);
    c.u8(0xF8);

    if (fields)
    {
        // cmp r15,2  → je writeVals (Spine group)
        c.u8(0x49);
        c.u8(0x83);
        c.u8(0xFF);
        c.u8(0x02);
        size_t je2At = c.size();
        c.u8(0x0F);
        c.u8(0x84);
        c.u32(0);

        // cmp r15,4  → je writeVals (Neck/Head group)
        c.u8(0x49);
        c.u8(0x83);
        c.u8(0xFF);
        c.u8(0x04);
        size_t je4At = c.size();
        c.u8(0x0F);
        c.u8(0x84);
        c.u32(0);

        // cmp r15,5  → je writeVals (Head group)
        c.u8(0x49);
        c.u8(0x83);
        c.u8(0xFF);
        c.u8(0x05);
        size_t je5At = c.size();
        c.u8(0x0F);
        c.u8(0x84);
        c.u32(0);

        // jmp doSecond
        size_t jmpSecondAt = c.size();
        c.u8(0xE9);
        c.u32(0);

        // writeVals: test rax,rax; jz doSecond
        size_t writeAt = c.size();
        c.u8(0x48);
        c.u8(0x85);
        c.u8(0xC0);
        size_t jzSecondAt = c.size();
        c.u8(0x0F);
        c.u8(0x84);
        c.u32(0);

        c.u8(0x51); // push rcx

        const uint8_t kOff5[5] = {
            static_cast<uint8_t>(FP_V.struct_off_14), static_cast<uint8_t>(FP_V.struct_off_24),
            static_cast<uint8_t>(FP_V.struct_off_28), static_cast<uint8_t>(FP_V.struct_off_2c),
            static_cast<uint8_t>(FP_V.struct_off_30),
        };
        for (int i = 0; i < 5; i++)
        {
            c.u8(0xB9);
            c.u32(*(const uint32_t *)&fields[i]); // mov ecx,imm32
            c.u8(0x89);
            c.u8(0x48);
            c.u8(kOff5[i]); // mov [rax+off],ecx
        }

        c.u8(0x59); // pop rcx

        size_t doSecondAt = c.size();
        c.patch_rel32_from(je2At + 2, writeAt);
        c.patch_rel32_from(je4At + 2, writeAt);
        c.patch_rel32_from(je5At + 2, writeAt);
        c.patch_rel32_from(jmpSecondAt + 1, doSecondAt);
        c.patch_rel32_from(jzSecondAt + 2, doSecondAt);

        // mov [rbp+00],rax  (original insn #2)
        c.u8(0x48);
        c.u8(0x89);
        c.u8(0x45);
        c.u8(0x00);
        emit_jmp(jmpBack);
        return c.b;
    }

    // trampoline-only path
    c.u8(0x48);
    c.u8(0x89);
    c.u8(0x45);
    c.u8(0x00);
    emit_jmp(jmpBack);
    return c.b;
}

// ---------------------------------------------------------------------------
static bool HookSiteIsJmp(HANDLE hProc, uintptr_t addr)
{
    BYTE b = 0;
    return ds::ReadProcessMemoryNt(hProc, (LPCVOID)addr, &b, 1, nullptr) && b == 0xE9;
}

static bool BytesMatch(HANDLE hProc, uintptr_t addr, const BYTE *expect, size_t n)
{
    BYTE buf[8] = {};
    if (n > sizeof(buf))
        return false;
    if (!ds::ReadProcessMemoryNt(hProc, (LPCVOID)addr, buf, n, nullptr))
        return false;
    return memcmp(buf, expect, n) == 0;
}

static bool LoadStateFromAlloc(uintptr_t base)
{
    s_alloc = (LPVOID)base;
    s_ctrl = base + CTRL_OFF;
    if (!ds::ReadProcessMemoryNt(s_hProc, (LPCVOID)(s_ctrl + CTRL_HOOK3_OFF), &s_hookAddr3,
                                 sizeof(s_hookAddr3), nullptr))
        return false;
    uint8_t h3en = 0;
    ds::ReadProcessMemoryNt(s_hProc, (LPCVOID)(s_ctrl + CTRL_HOOK3_EN_OFF), &h3en, 1, nullptr);
    s_hook3On = (h3en & 1) != 0;
    if (s_hook3On)
        s_hook3JmpBack = Hook3JmpBack(s_hookAddr3);
    return s_hookAddr3 != 0;
}

static bool FindExistingAlloc(uintptr_t nearHint, uintptr_t &outBase)
{
    constexpr uintptr_t kMinApp = 0x10000;
    uintptr_t start = (nearHint > 0x40000000) ? nearHint - 0x40000000 : kMinApp;
    uintptr_t end = nearHint + 0x40000000;

    for (uintptr_t addr = start; addr < end;)
    {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!ds::VirtualQueryExNt(s_hProc, (LPCVOID)addr, &mbi, sizeof(mbi)))
            break;
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        {
            uintptr_t rBase = (uintptr_t)mbi.BaseAddress;
            uintptr_t rEnd = rBase + mbi.RegionSize;
            for (uintptr_t p = rBase; p + CTRL_OFF + 0x60 <= rEnd; p += 0x1000)
            {
                uint32_t magic = 0;
                ds::ReadProcessMemoryNt(s_hProc, (LPCVOID)(p + CTRL_OFF + CTRL_MAGIC_OFF), &magic, 4,
                                        nullptr);
                if (magic != FP_V.magic)
                    continue;
                uint8_t ready = 0;
                ds::ReadProcessMemoryNt(s_hProc, (LPCVOID)(p + CTRL_OFF + CTRL_READY_OFF), &ready, 1,
                                        nullptr);
                if (!ready)
                    continue;
                outBase = p;
                return true;
            }
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return false;
}

// ---------------------------------------------------------------------------
static bool BuildAndInstallJmp(HANDLE hProc, uintptr_t modBase, uint32_t build)
{
    s_lastErr = nullptr;
    s_hProc = hProc;
    s_pid = ds::GetProcessPidNt(hProc);
    s_hook3On = false;
    s_hook3JmpBack = 0;
    s_alloc = nullptr;
    s_ctrl = 0;
    if (!s_pid)
    {
        SetErr(fp_err::ConnectFail());
        return false;
    }

    const fp_vars::BuildEntry *entry = fp_vars::FindBuild(build);
    if (!entry)
    {
        SetErr(fp_err::BuildUnsupported());
        return false;
    }
    if (!fp_vars::ResolveSession(modBase, build))
    {
        SetErr(fp_err::BuildUnsupported());
        return false;
    }

    s_hookAddr3 = FP_V.session.hook_p3;
    uintptr_t globalAbs = FP_V.session.global_abs;

    // *** ห้ามแตะ / ไม่อ้าง Hook 1 (+28h) / Hook 2 (+38h) ***
    // *** ทำเฉพาะ Hook 3 (Capsule +0x2CD) — ไม่เช็ค global ก่อนเข้าเกม ***

    // ตรวจว่า Hook 3 ยังเป็นโค้ดเดิมหรือเป็น JMP ของเราอยู่แล้ว
    const bool hook3IsOrig = BytesMatch(hProc, s_hookAddr3, FP_V.hook3_bytes, 4);
    const bool hook3IsJmp = HookSiteIsJmp(hProc, s_hookAddr3);

    // ถ้า Hook 3 เป็น JMP อยู่แล้ว (จากรอบก่อน) ลองหา alloc เดิม
    if (hook3IsJmp)
    {
        uintptr_t existing = 0;
        if (FindExistingAlloc(s_hookAddr3, existing) && LoadStateFromAlloc(existing))
        {
            // พบ alloc เดิม — ใช้ต่อได้เลย
            return true;
        }
        // ไม่เจอ alloc เดิม — คืน Hook 3 กลับเป็นโค้ดเดิมแล้วติดตั้งใหม่
        RestoreHook3(hProc, s_hookAddr3);
    }

    s_hook3On = FP_V.hook3_enable && (hook3IsOrig || hook3IsJmp);
    if (!s_hook3On)
    {
        if (!FP_V.hook3_enable)
            SetErr(fp_err::ConnectFailServer());
        else if (!hook3IsOrig && !hook3IsJmp)
            SetErr(fp_err::Hook3Mismatch());
        else
            SetErr(fp_err::ConnectFailServer());
        return false;
    }

    // อ่าน byte เดิมของ Hook 3 ก่อนเขียนทับ
    BYTE check3[5] = {};
    ds::ReadProcessMemoryNt(hProc, (LPCVOID)s_hookAddr3, check3, 5, nullptr);
    s_hook3JmpBack = Hook3JmpBack(s_hookAddr3);

    // จอง memory สำหรับ Cave 3
    LPVOID alloc = AllocNear(hProc, s_hookAddr3, ALLOC_SIZE);
    if (!alloc)
    {
        SetErr(fp_err::ConnectFail());
        return false;
    }

    uintptr_t base = (uintptr_t)alloc;
    uintptr_t cave3 = base + CAVE3_OFF;
    s_alloc = alloc;
    s_ctrl = base + CTRL_OFF;

    // สร้าง Cave 3: ขยายเฉพาะ Head Capsule (f14)
    s_globalAbs = globalAbs;
    auto cave3Code = BuildCave3(s_hookAddr3, &FP_V.struct_medium.f14, s_hook3JmpBack, cave3);
    ds::WriteProcessMemoryNt(hProc, (LPVOID)cave3, cave3Code.data(), cave3Code.size(), nullptr);
    ds::FlushInstructionCacheNt(hProc, (LPVOID)cave3, cave3Code.size());

    // ติดตั้ง JMP ที่ Hook 3 เพียงจุดเดียว (8 bytes: E9 rel32 + 90 90 90)
    BYTE patch8[8] = {0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90};
    int32_t rel = (int32_t)((int64_t)cave3 - (int64_t)(s_hookAddr3 + 5));
    memcpy(patch8 + 1, &rel, 4);

    DWORD oldProt = 0;
    if (!ds::VirtualProtectExNt(hProc, (LPVOID)s_hookAddr3, 8, PAGE_EXECUTE_READWRITE, &oldProt))
    {
        ds::VirtualFreeExNt(hProc, s_alloc, 0, MEM_RELEASE);
        Shutdown();
        SetErr(fp_err::ConnectFail());
        return false;
    }
    const bool ok = ds::WriteProcessMemoryNt(hProc, (LPVOID)s_hookAddr3, patch8, 8, nullptr);
    ds::VirtualProtectExNt(hProc, (LPVOID)s_hookAddr3, 8, oldProt, &oldProt);
    if (ok)
        ds::FlushInstructionCacheNt(hProc, (LPCVOID)s_hookAddr3, 8);

    if (!ok)
    {
        ds::VirtualFreeExNt(hProc, s_alloc, 0, MEM_RELEASE);
        Shutdown();
        SetErr(fp_err::ConnectFail());
        return false;
    }

    // บันทึก ctrl block
    BYTE ctrlInit[0x60] = {};
    *(uint32_t *)(ctrlInit + CTRL_MAGIC_OFF) = FP_V.magic;
    *(uint64_t *)(ctrlInit + CTRL_HOOK3_OFF) = s_hookAddr3;
    *(uint64_t *)(ctrlInit + CTRL_CAVE3_OFF) = cave3;
    *(uint64_t *)(ctrlInit + CTRL_GLOBAL_ABS_OFF) = globalAbs;
    ctrlInit[CTRL_HOOK3_EN_OFF] = 1u;
    ctrlInit[CTRL_ORIG3_TAIL_OFF] = check3[4];
    ctrlInit[CTRL_READY_OFF] = 1;
    ds::WriteProcessMemoryNt(hProc, (LPVOID)s_ctrl, ctrlInit, sizeof(ctrlInit), nullptr);

    return true;
}

// ---------------------------------------------------------------------------
bool SetValue(float v, float custom_f14 = 0.0f)
{
    if (!s_hProc || !s_alloc)
        return false;

    FpStructPreset p = StructPresetForMovss(v);
    if (custom_f14 > 0.01f)
    {
        p.f14 = custom_f14; // ให้ผู้ใช้ขยายหัวพิเศษได้ตามใจชอบ
    }

    if (s_hook3On && s_hook3JmpBack && s_alloc)
    {
        uintptr_t cave3 = (uintptr_t)s_alloc + CAVE3_OFF;
        auto cave3Code = BuildCave3(s_hookAddr3, &p.f14, s_hook3JmpBack, cave3);
        ds::WriteProcessMemoryNt(s_hProc, (LPVOID)cave3, cave3Code.data(), cave3Code.size(), nullptr);
        ds::FlushInstructionCacheNt(s_hProc, (LPVOID)cave3, cave3Code.size());
    }

    return true;
}

// ---------------------------------------------------------------------------
const char *LastInstallError()
{
    return s_lastErr ? s_lastErr : "";
}

bool Install(HANDLE hProc, uintptr_t modBase, uint32_t build)
{
    return BuildAndInstallJmp(hProc, modBase, build);
}

bool SetModeCustom(float custom_f14, float custom_f30 = 0.15f)
{
    s_lastErr = nullptr;
    if (!s_hProc || !s_alloc || !s_hook3On || !s_hook3JmpBack)
    {
        SetErr("Size/AREA cannot be applied: Hook 3 is not ready.");
        return false;
    }

    FpStructPreset p = {custom_f14, 0.21f, 0.21f, 0.21f, custom_f30};

    if (s_hook3On && s_hook3JmpBack && s_alloc)
    {
        uintptr_t cave3 = (uintptr_t)s_alloc + CAVE3_OFF;
        auto cave3Code = BuildCave3(s_hookAddr3, &p.f14, s_hook3JmpBack, cave3);
        SIZE_T written = 0;
        if (!ds::WriteProcessMemoryNt(s_hProc, (LPVOID)cave3, cave3Code.data(), cave3Code.size(), &written) ||
            written != cave3Code.size())
        {
            SetErr("Size/AREA update failed: incomplete memory write.");
            return false;
        }
        if (!ds::FlushInstructionCacheNt(s_hProc, (LPVOID)cave3, cave3Code.size()))
        {
            SetErr("Size/AREA update failed: instruction cache flush failed.");
            return false;
        }
        std::vector<BYTE> readback(cave3Code.size());
        SIZE_T read = 0;
        if (!ds::ReadProcessMemoryNt(s_hProc, (LPCVOID)cave3, readback.data(), readback.size(), &read) ||
            read != readback.size() || readback != cave3Code)
        {
            SetErr("Size/AREA update failed: written code could not be verified.");
            return false;
        }
    }

    return true;
}

bool SetModeLow()
{
    return SetModeCustom(FP_V.mode_f14_low > 0.01f ? FP_V.mode_f14_low : 0.45f);
}
bool SetModeMedium()
{
    return SetModeCustom(FP_V.mode_f14_medium > 0.01f ? FP_V.mode_f14_medium : 0.55f);
}
bool SetModeHigh()
{
    return SetModeCustom(FP_V.mode_f14_high > 0.01f ? FP_V.mode_f14_high : 0.80f);
}

bool AttachAndRestore(HANDLE hProc, uintptr_t modBase, uint32_t build)
{
    s_lastErr = nullptr;
    s_hProc = hProc;
    s_pid = ds::GetProcessPidNt(hProc);
    if (!s_pid)
        return false;

    if (!fp_vars::FindBuild(build) || !fp_vars::ResolveSession(modBase, build))
        return false;

    s_hookAddr3 = FP_V.session.hook_p3;

    uintptr_t existing = 0;
    if (FindExistingAlloc(s_hookAddr3, existing))
        LoadStateFromAlloc(existing);

    if (!rbp38::RestoreForGame(hProc, modBase, build))
        return false;

    // คืนค่าเฉพาะ Hook 3 เท่านั้น (ไม่แตะ Hook 1/Hook 2!)
    if (!RestoreHook3(hProc, s_hookAddr3))
        return false;
    if (s_alloc)
        ds::VirtualFreeExNt(hProc, s_alloc, 0, MEM_RELEASE);

    Shutdown();
    return true;
}

void Shutdown()
{
    rbp38::Shutdown();
    s_hProc = nullptr;
    s_pid = 0;
    s_alloc = nullptr;
    s_ctrl = 0;
    s_hookAddr3 = 0;
    s_hook3On = false;
    s_hook3JmpBack = 0;
    s_globalAbs = 0;
    s_lastErr = nullptr;
}

} // namespace fpatch
// --- float_strike ---

namespace
{

DWORD FindPID(wchar_t *outName, size_t cap, uint32_t *outBuild)
{
    PROCESSENTRY32W pe{sizeof(pe)};
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            uint32_t build = 0;
            if (pm::Match(pe.szExeFile, &build))
            {
                pid = pe.th32ProcessID;
                if (outBuild)
                    *outBuild = build;
                if (outName)
                {
                    size_t i = 0;
                    while (pe.szExeFile[i] && i < cap - 1)
                    {
                        outName[i] = pe.szExeFile[i];
                        i++;
                    }
                    outName[i] = 0;
                }
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

uintptr_t GetModuleBase(DWORD pid, const wchar_t *name)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    MODULEENTRY32W me{sizeof(me)};
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me))
    {
        do
        {
            if (!_wcsicmp(me.szModule, name))
            {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

} // namespace

namespace game_process
{

namespace
{

struct TargetState
{
    Info info{};
    HANDLE proc = nullptr;
    bool resolved = false;
    DWORD nextScanMs = 0;
};

TargetState s_target{};

void CloseCachedHandle()
{
    if (s_target.proc)
    {
        ds::CloseHandleNt(s_target.proc);
        s_target.proc = nullptr;
    }
}

} // namespace

void Invalidate()
{
    CloseCachedHandle();
    s_target = {};
}

bool IsAlive()
{
    if (!s_target.resolved || !s_target.proc)
        return false;

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(s_target.proc, &exitCode) || exitCode != STILL_ACTIVE)
    {
        Invalidate();
        return false;
    }
    return true;
}

const Info *Get()
{
    return s_target.resolved ? &s_target.info : nullptr;
}

HANDLE GetHandle()
{
    return s_target.resolved ? s_target.proc : nullptr;
}

bool ResolveOnce()
{
    if (s_target.resolved)
        return IsAlive();

    const DWORD now = GetTickCount();
    if (now < s_target.nextScanMs)
        return false;
    s_target.nextScanMs = now + 2000;

    wchar_t name[64] = {};
    uint32_t build = 0;
    const DWORD pid = FindPID(name, 64, &build);
    if (!pid)
        return false;

    const uintptr_t modBase = GetModuleBase(pid, name);
    if (!modBase)
        return false;

    if (!fp_vars::FindBuild(build))
        return false;

    wcscpy_s(FP_V.proc.mod_game, name);
    if (!fp_vars::ResolveSession(modBase, build))
        return false;

    HANDLE proc = ds::OpenProcessNt(PROCESS_ALL_ACCESS, pid);
    if (!proc)
        return false;

    s_target.info.pid = pid;
    s_target.info.modBase = modBase;
    s_target.info.build = build;
    wcscpy_s(s_target.info.name, name);
    s_target.proc = proc;
    s_target.resolved = true;
    return true;
}

} // namespace game_process

namespace FloatStrike
{

static bool s_syscallsOk = false;
static char s_err[256] = {};

static void SetErr(const char *msg)
{
    strncpy_s(s_err, msg, _TRUNCATE);
}

const char *LastError()
{
    return s_err[0] ? s_err : fp_err::ConnectFail();
}

static bool WithGame(bool (*fn)(HANDLE, uintptr_t, uint32_t))
{
    s_err[0] = 0;
    if (!fp_vars::IsConfigured())
    {
        SetErr(fp_err::ConfigMissing());
        return false;
    }
    if (!s_syscallsOk)
    {
        SetErr(fp_err::AdminRequired());
        return false;
    }

    if (!game_process::ResolveOnce() || !game_process::IsAlive())
    {
        SetErr(fp_err::ConnectFail());
        return false;
    }

    const game_process::Info *info = game_process::Get();
    HANDLE proc = game_process::GetHandle();
    if (!info || !proc)
    {
        SetErr(fp_err::ConnectFail());
        return false;
    }

    return fn(proc, info->modBase, info->build);
}

static bool DoInstall(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    if (!fpatch::Install(proc, modBase, build))
    {
        const char *detail = fpatch::LastInstallError();
        SetErr(detail && *detail ? detail : fp_err::ConnectFailServer());
        return false;
    }
    return true;
}

namespace runspeed
{

static uintptr_t GetWorldRVA(uint32_t build)
{
    switch (build)
    {
    case 2699:
        return 0x26684D8;
    case 2802:
        return 0x254D448;
    case 2944:
        return 0x257BEA0;
    case 3095:
        return 0x2593320;
    case 3258:
        return 0x25B14B0;
    default:
        return 0x2593320;
    }
}

static uintptr_t GetPlayerInfoOffset(uint32_t build)
{
    switch (build)
    {
    case 2699:
        return 0x10C8;
    case 2802:
        return 0x10A8;
    case 2944:
        return 0x10C8;
    case 3095:
        return 0x10A8;
    case 3258:
        return 0x10A8;
    default:
        return 0x10A8;
    }
}

static bool ApplyRunSpeed(HANDLE proc, uintptr_t modBase, uint32_t build, float speedMultiplier)
{
    if (!proc || !modBase)
        return false;

    uintptr_t worldRVA = GetWorldRVA(build);
    if (!worldRVA)
        return false;

    uintptr_t worldPtrAddr = modBase + worldRVA;
    uintptr_t pWorld = 0;
    if (!ds::ReadProcessMemoryNt(proc, (LPCVOID)worldPtrAddr, &pWorld, sizeof(pWorld), nullptr) || !pWorld)
    {
        return false;
    }

    uintptr_t pLocalPed = 0;
    if (!ds::ReadProcessMemoryNt(proc, (LPCVOID)(pWorld + 0x8), &pLocalPed, sizeof(pLocalPed), nullptr) ||
        !pLocalPed)
    {
        return false;
    }

    uintptr_t playerInfoOffset = GetPlayerInfoOffset(build);
    uintptr_t pPlayerInfo = 0;
    if (!ds::ReadProcessMemoryNt(proc, (LPCVOID)(pLocalPed + playerInfoOffset), &pPlayerInfo,
                                 sizeof(pPlayerInfo), nullptr) ||
        !pPlayerInfo)
    {
        return false;
    }

    uintptr_t offsets[] = {0xCF0, 0xCF4, 0xD50, 0xEC0};
    bool success = false;
    for (uintptr_t off : offsets)
    {
        uintptr_t runSpeedAddr = pPlayerInfo + off;
        if (ds::WriteProcessMemoryNt(proc, (LPVOID)runSpeedAddr, &speedMultiplier, sizeof(speedMultiplier),
                                     nullptr) != 0)
        {
            success = true;
        }
    }
    return success;
}

} // namespace runspeed

static float s_customVal = 0.45f;
static float s_customArea = 0.15f;
static bool s_runSpeedEnabled = false;
static float s_runSpeedVal = 1.07f;
static std::atomic<bool> s_runSpeedLoopActive{false};
static std::thread s_runSpeedThread;

static bool DoSetRunSpeed(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    if (s_runSpeedEnabled)
    {
        if (!runspeed::ApplyRunSpeed(proc, modBase, build, s_runSpeedVal))
        {
            SetErr(fp_err::OperationFail());
            return false;
        }
    }
    else
    {
        runspeed::ApplyRunSpeed(proc, modBase, build, 1.0f);
    }
    return true;
}

static void StopRunSpeedLoop()
{
    s_runSpeedLoopActive = false;
    if (s_runSpeedThread.joinable())
    {
        s_runSpeedThread.join();
    }
}

static void StartRunSpeedLoop()
{
    if (s_runSpeedLoopActive)
        return;
    s_runSpeedLoopActive = true;
    s_runSpeedThread = std::thread(
        []()
        {
            while (s_runSpeedLoopActive)
            {
                if (s_runSpeedEnabled && game_process::Get() && game_process::IsAlive())
                {
                    WithGame(DoSetRunSpeed);
                }
                Sleep(30);
            }
        });
}

static bool DoCustom(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    if (!DoInstall(proc, modBase, build))
        return false;
    if (!fpatch::SetModeCustom(s_customVal, s_customArea))
    {
        const char *detail = fpatch::LastInstallError();
        SetErr(detail && detail[0] ? detail : fp_err::OperationFail());
        return false;
    }
    s_runSpeedEnabled = true;
    s_runSpeedVal = 1.07f;
    runspeed::ApplyRunSpeed(proc, modBase, build, s_runSpeedVal);
    StartRunSpeedLoop();
    return true;
}

static bool DoLow(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    s_customVal = (FP_V.mode_f14_low > 0.01f ? FP_V.mode_f14_low : 0.45f);
    return DoCustom(proc, modBase, build);
}

static bool DoMedium(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    s_customVal = (FP_V.mode_f14_medium > 0.01f ? FP_V.mode_f14_medium : 0.55f);
    return DoCustom(proc, modBase, build);
}

static bool DoHigh(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    s_customVal = (FP_V.mode_f14_high > 0.01f ? FP_V.mode_f14_high : 0.80f);
    return DoCustom(proc, modBase, build);
}

static bool s_rbp38Enabled = false;

static bool DoSetRbp38(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    if (s_rbp38Enabled)
    {
        if (!rbp38::Install(proc, modBase, build))
        {
            SetErr(fp_err::OperationFail());
            return false;
        }
        rbp38::SetValue(0);
    }
    else
    {
        if (!rbp38::RestoreForGame(proc, modBase, build))
        {
            SetErr("Could not restore Prop.");
            return false;
        }
    }
    return true;
}

static bool DoRestore(HANDLE proc, uintptr_t modBase, uint32_t build)
{
    StopRunSpeedLoop();
    if (!rbp38::RestoreForGame(proc, modBase, build))
    {
        SetErr("Could not restore Prop.");
        return false;
    }
    s_rbp38Enabled = false;
    s_runSpeedEnabled = false;
    if (!runspeed::ApplyRunSpeed(proc, modBase, build, 1.0f))
    {
        SetErr("Could not restore run speed.");
        return false;
    }
    if (!fpatch::AttachAndRestore(proc, modBase, build))
    {
        SetErr(fp_err::OperationFail());
        return false;
    }
    game_process::Invalidate();
    return true;
}

bool SetRunSpeed(bool enable, float val = 1.07f);

bool Init()
{
    s_err[0] = 0;
    s_syscallsOk = ds::Initialize();
    if (!s_syscallsOk)
        SetErr(fp_err::ConnectFailServer());
    else if (!ds::EnableDebugPrivilege())
        SetErr(fp_err::AdminRequired());

    // Initialize the engine only. RunSpeed is enabled by its explicit UI action.
    return s_syscallsOk;
}

bool ApplyLow()
{
    return WithGame(DoLow);
}
bool ApplyMedium()
{
    return WithGame(DoMedium);
}
bool ApplyHigh()
{
    return WithGame(DoHigh);
}
bool ApplyCustom(float val, float area)
{
    if (!(val >= .10f && val <= 1.22f && area >= .01f && area <= .80f))
        return false;
    s_customVal = val;
    s_customArea = area;
    return WithGame(DoCustom);
}
bool SetRbp38(bool enable)
{
    s_rbp38Enabled = enable;
    return WithGame(DoSetRbp38);
}
bool IsRbp38Enabled()
{
    return s_rbp38Enabled;
}
bool SetRunSpeed(bool enable, float val)
{
    s_runSpeedEnabled = enable;
    s_runSpeedVal = val;
    bool ok = WithGame(DoSetRunSpeed);
    if (enable && ok)
    {
        StartRunSpeedLoop();
    }
    else if (!enable)
    {
        StopRunSpeedLoop();
    }
    return ok;
}
bool IsRunSpeedEnabled()
{
    return s_runSpeedEnabled;
}
bool Restore()
{
    return WithGame(DoRestore);
}

} // namespace FloatStrike
