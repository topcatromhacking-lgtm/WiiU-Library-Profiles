"""Platform-independent, offline-only format for a Menu/Barista paired allowlist.

A manifest specifies only which game/homebrew icons may be returned by a
future Menu-scoped MCP hook. It embeds the paired Barista SHA-256 but never
writes or switches either the real console save or the Menu.
"""
from __future__ import annotations

from hashlib import sha256
import struct
import zlib
from collections.abc import Iterable
from .core import CatalogError, normalized_title_id

MAGIC=b'WLPA'
VERSION=1
HEADER='>4sHHI32s'
HEADER_LENGTH=struct.calcsize(HEADER)
MAX_ICONS=300


def encode_allowlist(title_ids: Iterable[str], paired_barista: bytes)->bytes:
    ids=sorted({int(normalized_title_id(t),16) for t in title_ids})
    if not 1<=len(ids)<=MAX_ICONS or 0 in ids:
        raise CatalogError('Require between 1 and 300 distinct valid icon Title IDs')
    header=struct.pack(HEADER,MAGIC,VERSION,0,len(ids),sha256(paired_barista).digest())
    payload=header+b''.join(struct.pack('>Q',tid) for tid in ids)
    return payload+struct.pack('>I',zlib.crc32(payload)&0xFFFFFFFF)


def decode_allowlist(data: bytes)->dict:
    if not isinstance(data,bytes) or len(data)<HEADER_LENGTH+4:
        raise CatalogError('Truncated Menu allowlist')
    magic,version,flags,n,paired_digest=struct.unpack_from(HEADER,data)
    if (magic!=MAGIC or version!=VERSION or flags!=0 or
        not 1<=n<=MAX_ICONS or len(data)!=HEADER_LENGTH+n*8+4):
        raise CatalogError('Invalid Menu allowlist header/length')
    if (zlib.crc32(data[:-4])&0xFFFFFFFF)!=struct.unpack_from('>I',data,len(data)-4)[0]:
        raise CatalogError('Invalid Menu allowlist checksum')
    ids=list(struct.unpack_from('>'+str(n)+'Q',data,HEADER_LENGTH))
    if ids!=sorted(set(ids)) or not all(ids):
        raise CatalogError('Menu allowlist must contain unique, sorted, nonzero IDs')
    return {'format':'WLPA','version':version,
            'paired_barista_sha256':paired_digest.hex(),
            'title_ids':[f'{i:016X}' for i in ids]}


def require_exact_layout_match(icon_ids: Iterable[str], declared_allowlist: Iterable[str])->list[str]:
    actual=[normalized_title_id(t) for t in icon_ids]
    declared=[normalized_title_id(t) for t in declared_allowlist]
    if len(actual)!=len(set(actual)) or len(declared)!=len(set(declared)) or set(actual)!=set(declared):
        raise CatalogError('Menu allowlist MUST match exact Barista icon Title IDs')
    if len(actual)>MAX_ICONS:
        raise CatalogError('More than 300 Barista Menu positions')
    return sorted(actual)
