#!/usr/bin/env python3
"""WULP146: plugin configuration UI, one boolean per console class.

The actual v145 title filter / profile selector is intentionally unchanged.
This builds an experimental WUPS plugin so checkbox UI can be exercised first.
"""
from __future__ import annotations
import argparse
import re
from pathlib import Path
import subprocess
import sys

PIN = "c3b6d06301ba97f479da6227c8dd95bbda01d29f"
CLASSES = [
    ("WIIU", "wulp_ui_wiiu", "Wii U (140)", 140),
    ("WII", "wulp_ui_wii", "Wii (32)", 32),
    ("NDS", "wulp_ui_nds", "Nintendo DS (31)", 31),
    ("GBA", "wulp_ui_gba", "Game Boy Advance (74)", 74),
]

def once(src: str, needle: str, replacement: str, label: str) -> str:
    n = src.count(needle)
    if n != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {n}")
    return src.replace(needle, replacement, 1)

def apply(upstream: Path) -> None:
    root = upstream.resolve()
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"],
                                      cwd=root, text=True).strip()
    if revision != PIN:
        raise RuntimeError(f"Wrong pinned upstream revision: {revision}")
    here = Path(__file__).resolve().parent
    subprocess.run([sys.executable, str(here / "build_v145_homp.py"), str(root)],
                   check=True)
    path = root / "src/main.cpp"
    code = path.read_text(encoding="utf-8")

    # Storage keys are deliberately separated from WULP marker/Barista/WLPA.
    decl = [
        "\n// WULP146 UI milestone: these switches only store future platform choices.",
        "// No Barista, WLPA, marker, active title list or Menu restart is altered.",
    ]
    for platform, key, display, count in CLASSES:
        decl.extend([
            f'#define WULP146_UI_{platform}_KEY "{key}"',
            f"bool gWulp146Ui{platform} = true;",
        ])
    code = once(code, "bool prevHideValue", "\n".join(decl) + "\n\nbool prevHideValue", "UI state")

    callbacks = [
        "// WULP146 platform-class selectors, NOT individual game selectors.",
        "// This is a UI-only hardware milestone; callbacks persist intent only.",
    ]
    for platform, key, display, count in CLASSES:
        callbacks.extend([
            f"void wulp146{platform}Changed(ConfigItemBoolean *item, bool newValue) {{",
            "    (void) item;",
            f"    gWulp146Ui{platform} = newValue;",
            f"    const auto err = WUPSStorageAPI::Store(WULP146_UI_{platform}_KEY, gWulp146Ui{platform});",
            "    if (err != WUPS_STORAGE_ERROR_SUCCESS) {",
            f'        DEBUG_FUNCTION_LINE_ERR("[WULP146] unable to save {platform} class choice: %s",',
            "                                WUPSStorageAPI::GetStatusStr(err).data());",
            "    }",
            "}",
        ])
    code = once(code,
        "WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {",
        "\n".join(callbacks) + "\n\nWUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {",
        "category callback")

    ui = [
        '        // Native Aroma Plugin Configuration menu: one toggle per PLATFORM.',
        '        auto platforms = WUPSConfigCategory::Create("WULP - Console Classes");',
    ]
    for platform, key, display, count in CLASSES:
        ui.extend([
            f'        platforms.add(WUPSConfigItemBoolean::Create(WULP146_UI_{platform}_KEY,',
            f'            "{display}", true, gWulp146Ui{platform}, &wulp146{platform}Changed));'
        ])
    ui.append("        root.add(std::move(platforms));")
    code = once(code,
                "    } catch (std::exception &e) {",
                "\n".join(ui) + "\n    } catch (std::exception &e) {",
                "Aroma category")

    storage = ["    // Read platform checkboxes; all four ON reproduces 277 games."]
    for platform, key, display, count in CLASSES:
        storage.extend([
            f"    storageError = WUPSStorageAPI::GetOrStoreDefault(WULP146_UI_{platform}_KEY,",
            f"                                       gWulp146Ui{platform}, true);",
            "    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {",
            f'        DEBUG_FUNCTION_LINE_ERR("[WULP146] unable to load {platform} checkbox: %s",',
            "                                WUPSStorageAPI::GetStatusStr(storageError).data());",
            "    }",
        ])
    code = once(code,
                "    // save storage\n    storageError = WUPSStorageAPI::SaveStorage();",
                "\n".join(storage) +
                "\n\n    // save storage\n    storageError = WUPSStorageAPI::SaveStorage();",
                "configuration storage")
    # Make the C++ use of std::move explicitly supported.
    code = once(code, "#include <optional>",
                     "#include <optional>\n#include <utility>", "utility include")

    # For UI-only experiment there is *no* use of gWulp146Ui in the MCP or
    # Barista paths; hooking it now risks breaking proven v144/v145 booting.
    for platform, _, _, _ in CLASSES:
        if len(re.findall(r"(?<![A-Za-z0-9_])gWulp146Ui" + platform + r"(?![A-Za-z0-9_])", code)) != 5:
            raise RuntimeError(f"Unexpected UI field usage: {platform}")
    path.write_text(code, encoding="utf-8")
    print("WULP146 four-class configuration UI patched; "
          "WULP145 MCP filter and Barista selector unchanged.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=Path)
    args = parser.parse_args()
    apply(args.upstream)
