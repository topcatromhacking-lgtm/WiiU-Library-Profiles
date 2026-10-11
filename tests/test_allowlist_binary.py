import unittest
from hashlib import sha256
from wiiu_library_profiles.allowlist_binary import (
    decode_allowlist,encode_allowlist,require_exact_layout_match
)
from wiiu_library_profiles.core import CatalogError

class AllowlistManifestTests(unittest.TestCase):
    def test_paired_hash_and_sorted_ids(self):
        original=['0005000FF1BB961E','000500001010E600','0005001010040100']
        b=encode_allowlist(original,b'offline Barista test fixture')
        r=decode_allowlist(b)
        self.assertEqual(r['title_ids'],sorted(original))
        self.assertEqual(r['paired_barista_sha256'],sha256(b'offline Barista test fixture').hexdigest())
    def test_crc_rejects_tampering(self):
        b=bytearray(encode_allowlist(['000500001010E600'],b'template'))
        b[-5]^=1
        with self.assertRaisesRegex(CatalogError,'checksum'):
            decode_allowlist(bytes(b))
    def test_rejects_bad_count_and_truncation(self):
        b=encode_allowlist(['000500001010E600'],b'foo')
        with self.assertRaises(CatalogError):decode_allowlist(b[:-2])
        with self.assertRaises(CatalogError):decode_allowlist(b'')
    def test_requires_exact_layout_ids(self):
        expected=['0005000FF1BB961E','000500001010E600']
        self.assertEqual(require_exact_layout_match(expected,expected),sorted(expected))
        with self.assertRaisesRegex(CatalogError,'match'):
            require_exact_layout_match(expected,['0005000FF1BB961E'])
        with self.assertRaisesRegex(CatalogError,'match'):
            require_exact_layout_match(expected,expected+expected)
    def test_no_more_than_300_allowed_icons(self):
        ids=[f'00050000{i+1:08X}' for i in range(301)]
        with self.assertRaisesRegex(CatalogError,'300'):
            encode_allowlist(ids,b'foo')
    def test_duplicate_titles_collapse_when_encoding(self):
        b=encode_allowlist(['000500001010E600']*2,b'x')
        self.assertEqual(len(decode_allowlist(b)['title_ids']),1)

if __name__=='__main__':unittest.main()
