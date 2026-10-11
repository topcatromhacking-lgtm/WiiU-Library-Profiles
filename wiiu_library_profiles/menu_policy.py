"""Offline-only fixed-tools-page and priority-ordered Wii U Menu planner.

The 300 paired Barista positions are split into a 15-position tools/system
page and 19 x 15 = 285 software positions. This *does not* rewrite Barista
or install a filter: placeholder folders are proposed records only.

The result is a strict allowlist. Games hidden by platform selection or
capacity MUST NOT reappear through MCP overflow refill.
"""
from __future__ import annotations

from typing import Any, Iterable, Mapping
from .core import CatalogError, Title, VALID_PLATFORMS, normalized_title_id

TOOL_PAGE_SIZE = 15
SOFTWARE_PAGE_COUNT = 19
SOFTWARE_PAGE_SIZE = 15
SOFTWARE_SLOTS = SOFTWARE_PAGE_COUNT * SOFTWARE_PAGE_SIZE
MENU_SLOTS = TOOL_PAGE_SIZE + SOFTWARE_SLOTS

_GAME_PLATFORMS = VALID_PLATFORMS - {'homebrew', 'system'}


def _valid_id(value: Any) -> str:
    return normalized_title_id(value)


def plan_fixed_menu(
    installed: Iterable[Title],
    *,
    priority: list[str],
    fixed_system_slots: list[dict],
    tool_candidates: list[dict],
    selected_tool_keys: list[str],
    saved_order: Mapping[str, list[str]] | None = None,
) -> dict:
    """Make a deterministic 300-slot *proposal*, with explicit overflow.

    fixed_system_slots: observed, physically occupied protected Barista slots
        with {'position': 0..14, 'title_id': '...'}, preserved at that position.
        Another console can have a different number of system slots.
    tool_candidates: Menu/SD-discovered items {'key': stable path or forwarder
        identity, 'title_id': full 64-bit ID, 'name': optional display name}.
    selected_tool_keys: explicit user selection; missing keys cause failure.
        For >available slots the caller MUST ask the user to deselect tools.
    saved_order: per-platform lists of full IDs, maintained across refreshes;
        previously positioned titles retain relative order, new games append
        deterministically by name then ID.

    The result does not contain Barista folder-record bytes. Placeholder folder
    creation and relocating existing Homebrew entries need separate validation.
    """
    if not isinstance(priority, list) or any(
        not isinstance(p, str) or p not in _GAME_PLATFORMS for p in priority
    ) or len(priority) != len(set(priority)):
        raise CatalogError('Platform priority must contain distinct game platforms')
    if not isinstance(fixed_system_slots, list) or not isinstance(tool_candidates, list):
        raise CatalogError('Fixed system slots and tool candidates must be arrays')
    if not isinstance(selected_tool_keys, list) or any(
        not isinstance(k, str) or not k for k in selected_tool_keys
    ) or len(selected_tool_keys) != len(set(selected_tool_keys)):
        raise CatalogError('Tool selection must contain distinct, nonempty keys')
    if saved_order is not None and (not isinstance(saved_order, Mapping) or any(
        p not in _GAME_PLATFORMS or not isinstance(ids, list) for p, ids in saved_order.items()
    )):
        raise CatalogError('Saved order must map game platforms to ID lists')

    slot_map: list[dict | None] = [None] * TOOL_PAGE_SIZE
    fixed_ids: set[str] = set()
    for obj in fixed_system_slots:
        if not isinstance(obj, dict):
            raise CatalogError('Malformed fixed slot')
        pos = obj.get('position')
        if type(pos) is not int or not 0 <= pos < TOOL_PAGE_SIZE:
            raise CatalogError('Protected system slot lies outside first page')
        tid = _valid_id(obj.get('title_id'))
        if tid[:8] not in {'00050010', '0005001B', '00050030'}:
            raise CatalogError('Fixed system slot is not a protected system Title ID')
        if slot_map[pos] is not None or tid in fixed_ids:
            raise CatalogError('Duplicate fixed system slot or title')
        fixed_ids.add(tid)
        slot_map[pos] = {'kind': 'fixed_system', 'position': pos, 'title_id': tid}
    candidate_map: dict[str, dict] = {}
    for item in tool_candidates:
        if not isinstance(item, dict):
            raise CatalogError('Malformed tool candidate')
        key = item.get('key')
        if not isinstance(key, str) or not key or key in candidate_map:
            raise CatalogError('Tool candidates need unique stable keys')
        tid = _valid_id(item.get('title_id'))
        if tid[:8] in {'0005000C', '0005000E', '00050010', '0005001B', '00050030'}:
            raise CatalogError('System/update/DLC records cannot be selectable tools')
        if tid in fixed_ids:
            raise CatalogError('A selectable tool duplicates a fixed system item')
        name = item.get('name','')
        if not isinstance(name,str):
            raise CatalogError('Invalid tool display name')
        candidate_map[key] = {'key': key, 'title_id': tid, 'name': name}
    missing = [key for key in selected_tool_keys if key not in candidate_map]
    if missing:
        raise CatalogError('Selected tools missing from current Homebrew inventory: '+repr(missing))

    available_tool_slots = TOOL_PAGE_SIZE - len(fixed_ids)
    if len(selected_tool_keys) > available_tool_slots:
        raise CatalogError(
            f'Only {available_tool_slots} selectable tool slots remain after '
            f'{len(fixed_ids)} fixed system entries; choose fewer tools '
            'and use Homebrew Loader for the rest')
    selected_tool_ids: set[str] = set()
    for key in selected_tool_keys:
        item = candidate_map[key]
        if item['title_id'] in selected_tool_ids:
            raise CatalogError('Two selected tools resolve to the same Title ID')
        selected_tool_ids.add(item['title_id'])
        pos = slot_map.index(None)
        slot_map[pos] = {'kind':'tool','position':pos,**item}
    for pos, obj in enumerate(slot_map):
        if obj is None:
            slot_map[pos] = {
                'kind':'placeholder_folder', 'position':pos,
                'note':'Not yet materialized: requires validated Barista folder structure'
            }

    all_game_records = [t for t in installed if t.kind == 'game']
    by_id: dict[str, Title] = {}
    # One icon per Title ID even if an ID has both mlc and usb copies.
    for entry in sorted(all_game_records, key=lambda t: (
        t.title_id, {'usb':0,'mlc':1}.get(t.device,2),t.device
    )):
        by_id.setdefault(entry.title_id, entry)
    if fixed_ids & set(by_id):
        raise CatalogError('System slot Title ID also appears as a game')
    candidates = [t for t in by_id.values() if t.title_id not in selected_tool_ids]

    order_index: dict[str, dict[str,int]] = {}
    for platform, ids in (saved_order or {}).items():
        norm = [_valid_id(v) for v in ids]
        if len(norm) != len(set(norm)):
            raise CatalogError('Duplicate Title ID within saved order for '+platform)
        order_index[platform] = {tid:i for i,tid in enumerate(norm)}
    chosen: list[Title] = []
    stats: list[dict] = []
    for platform in priority:
        pool = [t for t in candidates if t.platform == platform]
        previous = order_index.get(platform,{})
        pool.sort(key=lambda t: (
            0 if t.title_id in previous else 1,
            previous.get(t.title_id,0),
            t.name.casefold(),t.title_id
        ))
        remaining = SOFTWARE_SLOTS-len(chosen)
        accepted = pool[:remaining]
        chosen.extend(accepted)
        stats.append({
            'platform':platform,'matching_installed':len(pool),
            'displayed':len(accepted),'deferred_by_capacity':len(pool)-len(accepted)
        })
    software_ids = [t.title_id for t in chosen]
    if len(software_ids) != len(set(software_ids)):
        raise CatalogError('Duplicate visible game')
    software = [
        {'kind':'software', 'position':TOOL_PAGE_SIZE+i,
         'title_id':t.title_id, 'platform':t.platform, 'name':t.name}
        for i,t in enumerate(chosen)
    ]
    software += [
        {'kind':'empty_software', 'position':TOOL_PAGE_SIZE+i}
        for i in range(len(chosen), SOFTWARE_SLOTS)
    ]
    visible = fixed_ids | selected_tool_ids | set(software_ids)
    # These IDs are the allowlist for actual launchable icon enumeration.
    # Updates/DLC/system app metadata stay untouched/passed through in MCP.
    hidden = sorted(tid for tid in by_id if tid not in visible)
    chosen_platforms = set(priority)
    return {
        'format':'wlp-menu-policy-v1', 'mode':'offline_proposal_only',
        'tool_slots':TOOL_PAGE_SIZE,'software_slots':SOFTWARE_SLOTS,
        'visible_tool_count':len(selected_tool_keys),
        'fixed_system_count':len(fixed_ids),
        'placeholder_folder_count':TOOL_PAGE_SIZE-len(fixed_ids)-len(selected_tool_keys),
        'visible_software_count':len(chosen),
        'empty_software_count':SOFTWARE_SLOTS-len(chosen),
        'software_priority':priority,
        'per_platform':stats,
        'first_page':slot_map,'software_pages':software,
        'launchable_icon_allowlist':sorted(visible),
        'software_title_ids_in_order':software_ids,
        'hidden_installed_game_ids':hidden,
        'selected_platform_overflow':sum(s['deferred_by_capacity'] for s in stats),
        'not_selected_game_count':sum(t.platform not in chosen_platforms for t in candidates),
        'unclassified_game_ids':sorted(
            t.title_id for t in candidates if t.platform=='unclassified'
        ),
        'nonlaunchable_records_pass_through':sum(
            t.kind not in ('game','application') for t in installed),
        'warning':'Never write these proposed placeholder folders to Barista without '
                  'hardware-validated folder IDs/records. Enforce allowlist in '
                  'Menu-scoped MCP hook and separately reconcile redirected Barista.'
    }
