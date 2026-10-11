"""Parse title inventory and produce deterministic, non-mutating profile previews.

The Wii U Menu hook does NOT ship in v0.1. This module never modifies a console.
"""
from __future__ import annotations

from dataclasses import asdict, dataclass
from pathlib import Path
import json
import re
from typing import Any

TITLE_ID_RE = re.compile(r'^[0-9A-Fa-f]{16}$')
VALID_PLATFORMS = frozenset({
    'wiiu', 'wii', 'nes', 'snes', 'n64', 'nds', 'gba', 'gb', 'gbc',
    'tg16', 'msx', 'other', 'unclassified', 'system', 'homebrew'
})
VALID_ORIGINS = frozenset({'native', 'official_vc', 'injection', 'homebrew', 'system', 'unknown'})
DEFAULT_BUDGET = 280  # Conservative, not an official hardware capacity.

class CatalogError(ValueError):
    """Inventory, override, or profile has invalid or unsafe metadata."""


def normalized_title_id(value: Any) -> str:
    if not isinstance(value, str):
        raise CatalogError('title_id must be a 16-character hexadecimal string')
    value = value.strip().removeprefix('0x').removeprefix('0X')
    if not TITLE_ID_RE.fullmatch(value):
        raise CatalogError(f'Invalid 64-bit title ID: {value!r}')
    return value.upper()


# Wii U ACP product-code *family hints*. These designate the installed
# Wii U wrapper/emulator, NOT necessarily the actual game in an injection.
# An injected ROM can inherit all of the donor's ACP metadata.
def product_code_platform_hint(product_code: str) -> str | None:
    if not isinstance(product_code,str):
        return None
    code = product_code.upper()
    if re.fullmatch(r'WUP-[PU]-[A-Z0-9]{4}',code):
        return 'wiiu'
    match = re.fullmatch(r'WUP-N-([A-Z0-9]{4})',code)
    if not match:
        return None
    family = match.group(1)
    if family[0] == 'F':
        return 'nes'
    if family[0] == 'J':
        return 'snes'
    if family[0] == 'N':
        return 'n64'
    if family[0] == 'D':
        return 'nds'
    if family[0] == 'V':
        return 'wii'
    if family[:2] in ('PA','PB','PC','PD'):
        return 'gba'
    if family[:2] in ('PN','PP'):
        return 'tg16'
    if family[:2] == 'MN':
        return 'msx'
    return None


def known_base_category(title_id: str, app_type: str = '', product_code: str = '') -> tuple[str, str, str]:
    """Return platform/origin/status, conservatively; injections are unknowable by ID alone."""
    upper = int(title_id[:8], 16)
    if upper in (0x00050010, 0x0005001B, 0x00050030):
        # OS applications, system data and overlay applets are never profile targets.
        return 'system', 'system', 'system'
    if upper == 0x0005000F:  # Synthetic Homebrew-on-Menu titles
        return 'homebrew', 'homebrew', 'application'
    if upper in (0x0005000C, 0x0005000E):
        return 'unclassified', 'unknown', 'addon'
    # Wii mode/forwarder identification is provisional and may be donor-dependent.
    if app_type.upper() in ('0X8000002E', '8000002E'):
        return 'wii', 'unknown', 'game'
    if upper == 0x00050000:
        return product_code_platform_hint(product_code) or 'unclassified', 'unknown', 'game'
    return 'unclassified', 'unknown', 'other'


@dataclass(frozen=True)
class Title:
    title_id: str
    device: str
    app_type: str
    title_version: int
    name: str
    platform: str
    origin: str
    kind: str
    tags: tuple[str, ...] = ()
    # Diagnostic ACP fields, not grounds to declare an injection "official".
    meta_status: str = 'unknown'
    product_code: str = ''
    content_platform: str = ''
    region: int | None = None

    def as_dict(self) -> dict:
        obj = asdict(self)
        obj['tags'] = list(self.tags)
        return obj


def _load_json(path: str | Path) -> Any:
    try:
        return json.loads(Path(path).read_text(encoding='utf-8'))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise CatalogError(f'Cannot read valid JSON from {path}: {exc}') from exc


def parse_catalog(data: Any, *, overrides: dict | None = None) -> list[Title]:
    """Parse MCP title records, safely applying explicit full-title-ID overrides.

    A title may appear twice if the same Title ID is listed on separate devices.
    """
    if not isinstance(data, dict) or not isinstance(data.get('titles'), list):
        raise CatalogError('Catalog must be an object with a titles array')
    entries = data['titles']
    if len(entries) > 10000:
        raise CatalogError('Inventory exceeds 10,000 records')
    cleaned_overrides: dict[str, dict] = {}
    for key, value in (overrides or {}).items():
        tid = normalized_title_id(key)
        if not isinstance(value, dict):
            raise CatalogError('Overrides must contain objects keyed by full title ID')
        if tid in cleaned_overrides:
            raise CatalogError(f'Duplicate title ID override: {tid}')
        if not set(value).issubset({'platform', 'origin', 'name', 'tags'}):
            raise CatalogError('Unknown override fields for '+tid)
        cleaned_overrides[tid] = value
    titles: list[Title] = []
    seen: set[tuple[str,str]] = set()
    for raw in entries:
        if not isinstance(raw,dict):
            raise CatalogError('Each title must be an object')
        title_id = normalized_title_id(raw.get('title_id'))
        device = raw.get('device', 'unknown')
        app_type = raw.get('app_type', 'unknown')
        version = raw.get('title_version', 0)
        if not isinstance(device,str) or len(device)>64:
            raise CatalogError('Invalid storage device')
        if not isinstance(app_type,str) or len(app_type)>16:
            raise CatalogError('Invalid app_type')
        if type(version) is not int or not 0 <= version <= 65535:
            raise CatalogError('Invalid title version')
        key=(title_id,device)
        if key in seen:
            raise CatalogError(f'Duplicate (title ID, device): {title_id}, {device}')
        seen.add(key)
        platform, origin, kind = known_base_category(title_id,app_type,raw.get('product_code',''))
        ov = cleaned_overrides.get(title_id,{})
        # System titles are always protected; a data file may not reclassify them.
        if kind=='system' and (('platform' in ov and ov['platform'] != 'system') or
                               ('origin' in ov and ov['origin'] != 'system')):
            raise CatalogError(f'Cannot override system title class: {title_id}')
        platform=ov.get('platform',platform)
        origin=ov.get('origin',origin)
        # ACP metadata is an optional, untrusted *label*, never a platform decision.
        name=ov.get('name')
        if name is None:
            candidates=(raw.get('name'),raw.get('shortname_en'),raw.get('shortname_es'),
                        raw.get('shortname_ja'),raw.get('longname_en'),raw.get('longname_es'))
            name=next((n for n in candidates if isinstance(n,str) and n.strip()),'')
        tags=ov.get('tags',raw.get('tags',[]))
        meta_status=raw.get('meta_status','unknown')
        product_code=raw.get('product_code','')
        content_platform=raw.get('content_platform','')
        region=raw.get('region',None)
        if meta_status not in ('ok','unavailable','skipped','unknown') or not isinstance(meta_status,str):
            raise CatalogError(f'Invalid ACP metadata status: {title_id}')
        if not isinstance(product_code,str) or len(product_code)>32 or not isinstance(content_platform,str) or len(content_platform)>32:
            raise CatalogError(f'Invalid ACP product/platform strings: {title_id}')
        if region is not None and (type(region) is not int or not 0 <= region <= 0xFFFFFFFF):
            raise CatalogError(f'Invalid ACP region: {title_id}')
        if platform not in VALID_PLATFORMS or origin not in VALID_ORIGINS:
            raise CatalogError(f'Invalid platform or origin classification: {title_id}')
        if not isinstance(name,str) or len(name)>256:
            raise CatalogError('Invalid game name')
        if not isinstance(tags,list) or len(tags)>32 or any(not isinstance(t,str) or len(t)>60 for t in tags):
            raise CatalogError('Invalid tags')
        titles.append(Title(title_id,device,app_type,version,name,platform,origin,kind,tuple(tags),
                            meta_status,product_code,content_platform,region))
    return titles


def read_catalog(catalog_path: str | Path, overrides_path: str | Path | None = None) -> list[Title]:
    overrides = _load_json(overrides_path) if overrides_path else None
    if overrides is not None and not isinstance(overrides,dict):
        raise CatalogError('Override file must be a title-ID keyed JSON object')
    return parse_catalog(_load_json(catalog_path),overrides=overrides)


def preview(titles: list[Title], profile: dict, *, budget: int = DEFAULT_BUDGET) -> dict:
    """Simulate a filtered Wii U Menu view. Never truncate silently.

    A real Aroma plugin will require additional safety validation before deploy.
    """
    if type(budget) is not int or not 1 <= budget <= 300:
        raise CatalogError('Preview budget must be between 1 and 300')
    if not isinstance(profile,dict) or set(profile)-{'name','platforms','origins','tags','include_ids','exclude_ids'}:
        raise CatalogError('Unknown or invalid profile fields')
    name = profile.get('name','Unnamed')
    if not isinstance(name,str) or not name or len(name)>80:
        raise CatalogError('Invalid profile name')
    platforms = profile.get('platforms',[])
    origins = profile.get('origins',[])
    tags = profile.get('tags',[])
    include = profile.get('include_ids',[])
    exclude = profile.get('exclude_ids',[])
    if any(not isinstance(x,list) for x in (platforms,origins,tags,include,exclude)):
        raise CatalogError('Profile selectors must be arrays')
    if any(x not in VALID_PLATFORMS for x in platforms) or any(x not in VALID_ORIGINS for x in origins):
        raise CatalogError('Invalid platform or origin selector')
    if any(not isinstance(x,str) for x in tags):
        raise CatalogError('Tags must be strings')
    allow_ids={normalized_title_id(x) for x in include}
    deny_ids={normalized_title_id(x) for x in exclude}
    if allow_ids & deny_ids:
        raise CatalogError('A title cannot be both included and excluded')
    selected=[]
    pass_through=0
    launchable=0
    for entry in titles:
        # Preserve updates, DLC, OS apps, shared data and overlay applets in
        # any future MCP hook. These records do NOT consume Wii U Menu slots.
        if entry.kind not in ('game', 'application'):
            pass_through += 1
            continue
        launchable += 1
        protected = entry.platform == 'homebrew'
        # Homebrew remains selectable regardless of custom exclusions.
        # Platform and origin are conjunctive facets; tags and explicit IDs
        # are additive shortcuts for personal collections (e.g. Favorites).
        facet_match = bool(platforms or origins) and (not platforms or entry.platform in platforms) and (not origins or entry.origin in origins)
        qualifies = (facet_match or bool(set(entry.tags).intersection(tags)) or entry.title_id in allow_ids)
        if protected or (qualifies and entry.title_id not in deny_ids):
            selected.append(entry)
    selected.sort(key=lambda t:(t.platform,t.name.casefold(),t.title_id,t.device))
    if len(selected)>budget:
        raise CatalogError(f'Profile {name!r} has {len(selected)} entries, exceeds budget {budget}. '
                           'Split the view into smaller subcategories; nothing was discarded.')
    return {'name':name,'total_installed_records':len(titles),
            'launchable_record_count':launchable, 'pass_through_record_count':pass_through,
            'visible_count':len(selected), 'hidden_count':launchable-len(selected),
            'budget':budget,'remaining':budget-len(selected),
            'titles':[t.as_dict() for t in selected],
            'note':'Offline preview only. Nonlaunchable MCP records are pass-through, not Menu icons; no console modifications.'}
