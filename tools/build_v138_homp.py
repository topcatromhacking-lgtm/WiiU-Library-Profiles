#!/usr/bin/env python3
"""Apply WULP v1.3.8 on pinned homebrew_on_menu_plugin source.

Build-time source patcher: does not write SD or console files.
A verified immutable candidate activates only if marker, Barista and WLPA match.
Missing inputs use the original redirect and original title enumeration.
"""
import pathlib
import shutil
import subprocess
import argparse

PIN = "c3b6d06301ba97f479da6227c8dd95bbda01d29f"

def replace_once(source, old, new, purpose):
    count = source.count(old)
    if count != 1:
        raise RuntimeError(f"{purpose}: expected one exact patch anchor, found {count}")
    return source.replace(old, new, 1)

SAVE_HELPER = r"""
// WULP138: read and validate an opt-in private profile before SaveRedirection_AddFSLayer.
// No writes, no account/serial information in logs. Legacy is always the default.
bool gWulp138Extended = false;
uint64_t gWulp138AllowedIds[284] = {};
static bool wulp138ReadBounded(const std::string &path, size_t maxBytes, std::vector<uint8_t> &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    const long length = ftell(f);
    if (length <= 0 || static_cast<unsigned long>(length) > maxBytes || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f); return false;
    }
    out.resize(static_cast<size_t>(length));
    const bool ok = fread(out.data(), 1u, out.size(), f) == out.size();
    fclose(f);
    if (!ok) out.clear();
    return ok;
}
static bool wulp138TryExtended(const std::string &originalSave, uint32_t account) {
    gWulp138Extended = false;
    if (account != 0x80000001u) return false;
    constexpr const char *root = "fs:/vol/external01/wiiu/wulp";
    constexpr const char *candidateRoot = "fs:/vol/external01/wiiu/wulp/profiles/extended/save/80000001";
    std::vector<uint8_t> marker, original, candidate, allowlist;
    if (!wulp138ReadBounded(std::string(root) + "/ACTIVE_PROFILE.txt", 220u, marker)) return false;
    static const char legacy[] = "WULP137:1\nprofile=legacy\n";
    if (marker.size() == sizeof(legacy) - 1u && memcmp(marker.data(), legacy, marker.size()) == 0) return false;
    if (!wulp138ReadBounded(originalSave, W138_BARISTA, original) ||
        !wulp138ReadBounded(std::string(candidateRoot) + "/BaristaAccountSaveFile.dat", W138_BARISTA, candidate) ||
        !wulp138ReadBounded(std::string(candidateRoot) + "/WLPA.paired", W138_WLPA, allowlist)) return false;
    const int decision = w138_gate(reinterpret_cast<const char *>(marker.data()), marker.size(),
                                   original.data(), original.size(), candidate.data(), candidate.size(),
                                   allowlist.data(), allowlist.size(), gWulp138AllowedIds);
    if (decision != 2) return false;
    gWulp138Extended = true;
    return true;
}
"""

def main():
    p = argparse.ArgumentParser()
    p.add_argument("upstream", type=pathlib.Path)
    a = p.parse_args()
    root = a.upstream.resolve()
    got = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    if got != PIN:
        raise SystemExit(f"Wrong pinned upstream revision: {got}, required {PIN}")
    savepath = root / "src/SaveRedirection.cpp"
    mainpath = root / "src/main.cpp"
    if not (savepath.exists() and mainpath.exists()):
        raise SystemExit("Missing upstream sources")
    save = savepath.read_text()
    menu = mainpath.read_text()
    save = replace_once(save, '#include "SaveRedirection.h"',
                        '#include "SaveRedirection.h"\n#include "wulp_v138_gate.h"\n#include <vector>\n#include <cstdio>\n#include <cstring>', "save headers")
    save = replace_once(save, 'void initSaveData() {', SAVE_HELPER + '\nvoid initSaveData() {', "save helper")
    redirect_line = '    std::string replaceDir = string_format("%s/%08X", getBaseSavePathFS().c_str(), 0x80000000 | persistentId);'
    new_redirect = redirect_line + r"""
    const auto account = static_cast<uint32_t>(0x80000000u | persistentId);
    const std::string oldBarista = replaceDir + "/BaristaAccountSaveFile.dat";
    if (wulp138TryExtended(oldBarista, account)) {
        replaceDir = "fs:/vol/external01/wiiu/wulp/profiles/extended/save/80000001";
        DEBUG_FUNCTION_LINE("WULP138 selection=EXTENDED_VERIFIED");
    } else {
        DEBUG_FUNCTION_LINE("WULP138 selection=LEGACY");
    }"""
    save = replace_once(save, redirect_line, new_redirect, "save redirect selection")
    menu = replace_once(menu, '#include "SaveRedirection.h"',
                        '#include "SaveRedirection.h"\nextern bool gWulp138Extended;\nextern uint64_t gWulp138AllowedIds[284];',
                        "main extern state")
    old_tail = '    *outTitleCount = titleCount;\n\n    return result;\n}\n\nDECL_FUNCTION(int32_t, ACPCheckTitleLaunchByTitleListTypeEx'
    new_tail = r"""    // WULP138: Filter both real Wii U games and synthetic homebrew using the
    // SAME WLPA whose paired Barista was activated earlier in this boot.
    // System / other title kinds are preserved exactly as upstream returned them.
    if (gWulp138Extended && result >= 0 && titleList && titleCount <= 1024u) {
        uint32_t writePos = 0;
        for (uint32_t readPos = 0; readPos < titleCount; ++readPos) {
            const uint64_t id = titleList[readPos].titleId;
            const uint32_t kind = static_cast<uint32_t>(id >> 32);
            if (kind != 0x00050000u && kind != 0x0005000Fu ||
                std::binary_search(gWulp138AllowedIds, gWulp138AllowedIds + 284u, id)) {
                if (writePos != readPos) titleList[writePos] = titleList[readPos];
                ++writePos;
            }
        }
        titleCount = writePos;
    }
    *outTitleCount = titleCount;

    return result;
}

DECL_FUNCTION(int32_t, ACPCheckTitleLaunchByTitleListTypeEx"""
    menu = replace_once(menu, old_tail, new_tail, "MCP title-list filter")
    header = pathlib.Path(__file__).resolve().parent.parent / "src" / "wulp_v138_gate.h"
    if not header.exists():
        raise SystemExit("Missing portable gate")
    # Reject all patch failures before writing any modified upstream source.
    savepath.write_text(save)
    mainpath.write_text(menu)
    shutil.copyfile(header, root / "src/wulp_v138_gate.h")
    print("WULP138 patched pinned Homebrew-on-Menu: save redirect + MCP filter")
    print("No additional SAVEInit hook; missing/mismatched profile falls back to original.")

if __name__ == "__main__":
    main()
