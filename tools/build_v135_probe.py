#!/usr/bin/env python3
"""WULP v1.3.5: patch a PINNED upstream ContentRedirectionModule checkout.
The generated module only adds filtered logging of real Barista opens/reads/closes.
No Barista/allowlist write, SD-copy, account-changing, or FS-layer mutation is added.
"""
import argparse
import pathlib
import shutil
import subprocess

UPSTREAM_SHA = "31b319b6b4370caf4ab173f0db91cf838c6819d0"

def replace_once(source: str, old: str, new: str, description: str) -> str:
    occurrences = source.count(old)
    if occurrences != 1:
        raise RuntimeError(f"{description}: expected one anchor, found {occurrences}")
    return source.replace(old, new, 1)

HELPERS = r'''
// WULP135: filtered diagnostic of real redirected Barista access.
// The account/serial and file bytes are NEVER logged.
// A successful OPEN alone is not sufficient: the matching READ fingerprint
// must be checked against the owner's independent Barista backup.
static std::atomic<uint32_t> wulp135Sequence{0};
static uint32_t wulp135Next() {
    return wulp135Sequence.fetch_add(1, std::memory_order_relaxed) + 1u;
}
static bool wulp135IsBaristaName(const char *s) {
    return s && strstr(s, "BaristaAccountSaveFile.dat") != nullptr;
}
static bool wulp135SourceMatch(const char *logical, const char *physical) {
    constexpr const char prefix[] = "/vol/save/";
    if (!logical || !physical ||
        strncmp(logical, prefix, sizeof(prefix) - 1u) != 0 ||
        strlen(logical) < sizeof(prefix) - 1u + 8u) return false;
    char account[9] = {};
    memcpy(account, logical + sizeof(prefix) - 1u, 8u);
    return wlp_v134_matches_redirect(logical, physical, account) != 0;
}
static uint64_t wulp135Fnv64(const void *p, size_t n) {
    const auto *b = static_cast<const uint8_t *>(p);
    uint64_t h = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < n; ++i) {
        h ^= b[i];
        h *= UINT64_C(1099511628211);
    }
    return h;
}
// End WULP135 helpers

'''
OPEN_OLD = '''            openFiles.push_back(fileHandle);

            OSMemoryBarrier();'''
OPEN_NEW = '''            fileHandle->wulpBarista =
                getName() == "homp_save_redirection" &&
                wulp135SourceMatch(path, newPath.c_str());

            openFiles.push_back(fileHandle);

            OSMemoryBarrier();
            if (getName() == "homp_save_redirection" && wulp135IsBaristaName(path)) {
                WHBLogPrintf("[WULP135] seq=%u event=OPEN handle=%08X source=%s\\n",
                             wulp135Next(), static_cast<unsigned>(fileHandle->handle),
                             fileHandle->wulpBarista ? "SD_REDIRECT_ACCOUNT_SAVE" : "UNVERIFIED");
            }'''
READ_OLD = '''    } else {
        result = static_cast<FSError>(((uint32_t) read) / size);
    }

    return result;
}

FSError FSWrapper::FSReadFileWithPosWrapper'''
READ_NEW = '''    } else {
        result = static_cast<FSError>(((uint32_t) read) / size);
        if (read > 0 && fileHandle->wulpBarista) {
            const off_t end = lseek(real_fd, 0, SEEK_CUR);
            uint32_t start = 0;
            if (wlp_v134_read_offset(static_cast<int64_t>(end),
                                      static_cast<size_t>(read), &start)) {
                const auto fingerprint =
                    static_cast<unsigned long long>(wulp135Fnv64(buffer, static_cast<size_t>(read)));
                WHBLogPrintf("[WULP135] seq=%u event=READ handle=%08X offset=%u length=%u fnv64=%016llX\\n",
                             wulp135Next(), static_cast<unsigned>(handle),
                             static_cast<unsigned>(start), static_cast<unsigned>(read),
                             fingerprint);
            } else {
                WHBLogPrintf("[WULP135] seq=%u event=READ_OFFSET_UNAVAILABLE handle=%08X\\n",
                             wulp135Next(), static_cast<unsigned>(handle));
            }
        }
    }

    return result;
}

FSError FSWrapper::FSReadFileWithPosWrapper'''
CLOSE_OLD = '''    fileHandle->fd = -1;
    return result;
}

bool FSWrapper::CheckFileShouldBeIgnored'''
CLOSE_NEW = '''    if (fileHandle->wulpBarista) {
        WHBLogPrintf("[WULP135] seq=%u event=CLOSE handle=%08X status=%s\\n",
                     wulp135Next(), static_cast<unsigned>(handle),
                     result == FS_ERROR_OK ? "OK" : "ERROR");
    }
    fileHandle->fd = -1;
    return result;
}

bool FSWrapper::CheckFileShouldBeIgnored'''

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=pathlib.Path,
                        help="Git checkout of pinned ContentRedirectionModule")
    args = parser.parse_args()
    root = args.upstream.resolve()
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    if sha != UPSTREAM_SHA:
        raise SystemExit(f"Wrong upstream commit: {sha}; required {UPSTREAM_SHA}")

    wulp_root = pathlib.Path(__file__).resolve().parent.parent
    helper = wulp_root / "src" / "v134_physical_source.h"
    if not helper.is_file():
        raise SystemExit(f"Missing WULP classifier: {helper}")
    cpp_file = root / "src" / "FSWrapper.cpp"
    info_file = root / "src" / "FileInfo.h"
    if not (cpp_file.is_file() and info_file.is_file()):
        raise SystemExit("Upstream files missing")
    cpp = cpp_file.read_text()
    info = info_file.read_text()
    cpp = replace_once(cpp, "#include <algorithm>\n#include <filesystem>",
                       '#include "wulp_v134_physical_source.h"\n#include <atomic>\n#include <algorithm>\n#include <filesystem>',
                       "C++ include")
    cpp = replace_once(cpp, "FSError FSWrapper::FSOpenDirWrapper", HELPERS +
                       "FSError FSWrapper::FSOpenDirWrapper", "helper insertion")
    cpp = replace_once(cpp, OPEN_OLD, OPEN_NEW, "open instrumentation")
    cpp = replace_once(cpp, READ_OLD, READ_NEW, "read instrumentation")
    cpp = replace_once(cpp, CLOSE_OLD, CLOSE_NEW, "close instrumentation")
    info = replace_once(info, "    int fd;\n", "    int fd;\n    bool wulpBarista = false;\n",
                        "FileInfo tracking")
    # Write only after ALL original-source anchors have been validated.
    cpp_file.write_text(cpp)
    info_file.write_text(info)
    shutil.copyfile(helper, root / "src" / "wulp_v134_physical_source.h")
    print(f"WULP v1.3.5 probe applied to pinned upstream {sha}")
    print("Modified: FSWrapper.cpp, FileInfo.h; added physical classifier.")
    print("Build with DEBUG=1. Never publish raw Wii U console logs.")

if __name__ == "__main__":
    main()
