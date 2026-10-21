import csv
import hashlib
import os
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
import v13_pair_generator as engine


def mk_template(extra=False):
    b = bytearray(engine.SIZE)
    for i in range(600):
        struct.pack_into('>II', b, i*16 + 8, 1 if i < 300 else 2, 0)
    system = [0x000500101004E100 + i*0x100 for i in range(8)]
    homebrew = [0x0005000F01000001 + i for i in range(7)]
    for i, v in enumerate(system + homebrew):
        struct.pack_into('>Q',b,i*16,v)
    if extra:struct.pack_into('>Q',b,15*16,0x0005000F11111111)
    for i in range(285):
        struct.pack_into('>Q',b,(315+i)*16,0x0005000010001000 + i)
    for folder in range(engine.FOLDERS):
        base=engine.FOLDER_START + folder*engine.FOLDER_STRIDE
        for slot in range(120):
            struct.pack_into('>II',b,base + slot*16 + 8,1 if slot < 60 else 2,0)
    b[9607] = 0x22
    return bytes(b)


def mk_fixture(folder, n=5):
    folder=Path(folder)
    ids=[f'{0x0005000010001000+i:016X}' for i in range(n)]
    platforms=['WIIU']*n
    p=folder/'preview.csv';a=folder/'audit.csv';inv=folder/'inventory.csv'
    with p.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=['selection','menu_position','title_id','platform','region_mask','manual_inclusion','name']);w.writeheader()
        for i,x in enumerate(ids):w.writerow(dict(selection='PREVIEW',menu_position=i+16,title_id=x,platform=platforms[i],region_mask='02',manual_inclusion='0',name=f'Game {i}'))
    with a.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=['title_id','app_type_hex','db_match','platform','region_mask','verified','name','product_code']);w.writeheader()
        for i,x in enumerate(ids):w.writerow(dict(title_id=x,app_type_hex='80000000',db_match='CONFIRMED',platform=platforms[i],region_mask='02',verified=1,name=f'Game {i}',product_code=''))
    with inv.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=['title_id','kind']);w.writeheader()
        for x in ids:w.writerow(dict(title_id=x,kind='GAME'))
    return ids,p,inv,a


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        self.work=tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.ids,self.preview,self.inventory,self.audit=mk_fixture(self.work.name)
        self.source=mk_template()
        self.rows=engine.load_preview(self.preview,self.inventory,self.audit)

    def build(self,source=None,rows=None,account='80000001',digest=None):
        if source is None:source=self.source
        if rows is None:rows=self.rows
        return engine.make_pair(source,rows,account,digest or engine.sha(source))

    def test_rewrite_exact_games_and_8_remaining_empty(self):
        candidate,allowlist,status=self.build()
        self.assertEqual(status['selected_games'],5)
        self.assertEqual(status['empty_usb_game_slots'],280)
        self.assertEqual(status['wlpa_count'],12)
        self.assertTrue(status['protected_prefix_equal'])
        self.assertTrue(status['opaque_suffix_equal'])
        self.assertEqual(candidate[9607],0x22)
        self.assertEqual(candidate[:engine.START],self.source[:engine.START])
        self.assertEqual(candidate[engine.END:],self.source[engine.END:])
        self.assertEqual(allowlist[12:44],hashlib.sha256(candidate).digest())
        self.assertEqual(zlib.crc32(allowlist[:-4])&0xffffffff,struct.unpack('>I',allowlist[-4:])[0])
        engine.verify_pair(self.source,candidate,allowlist,self.rows,status['seven_homebrew_title_ids'])

    def test_source_mismatch_rejected(self):
        with self.assertRaisesRegex(engine.Reject,'SHA-256 mismatch'):
            self.build(digest='0'*64)

    def test_other_account_refused(self):
        with self.assertRaisesRegex(engine.Reject,'Only account 80000001'):
            self.build(account='80000009')

    def test_historical_extra_homebrew_cannot_be_removed_silently(self):
        src=mk_template(extra=True)
        with self.assertRaisesRegex(engine.Reject,'outside first 15'):
            self.build(source=src)

    def test_truncated_source_refused(self):
        src=self.source[:-1]
        with self.assertRaisesRegex(engine.Reject,'Unknown Barista size'):
            self.build(source=src)

    def test_unusual_game_record_flags_refused(self):
        s=bytearray(self.source);s[315*16+15]=1
        with self.assertRaisesRegex(engine.Reject,'Unknown trailing flags'):
            self.build(source=bytes(s))

    def test_unknown_usb_record_type_refused(self):
        s=bytearray(self.source);s[315*16+11]=3
        with self.assertRaisesRegex(engine.Reject,'Unexpected USB bank record type'):
            self.build(source=bytes(s))

    def test_bad_first_page_detected(self):
        s=bytearray(self.source);s[0:8]=b'\x00'*8
        with self.assertRaisesRegex(engine.Reject,'Missing protected'):
            self.build(source=bytes(s))

    def test_duplicate_usb_ids_detected(self):
        s=bytearray(self.source);s[316*16:316*16+8]=s[315*16:315*16+8]
        with self.assertRaisesRegex(engine.Reject,'Duplicate current USB'):
            self.build(source=bytes(s))

    def test_populated_folder_rejected(self):
        s=bytearray(self.source)
        struct.pack_into('>Q',s,engine.FOLDER_START,0x0005000010100000)
        with self.assertRaisesRegex(engine.Reject,'Populated folder'):
            self.build(source=bytes(s))

    def test_malformed_folder_type_rejected(self):
        s=bytearray(self.source)
        struct.pack_into('>I',s,engine.FOLDER_START+8,9)
        with self.assertRaisesRegex(engine.Reject,'Unknown folder record'):
            self.build(source=bytes(s))

    def test_inactive_folder_metadata_rejected(self):
        s=bytearray(self.source)
        s[engine.FOLDER_START+1920]=1
        with self.assertRaisesRegex(engine.Reject,'Unexpected metadata in inactive folder'):
            self.build(source=bytes(s))

    def test_corrupted_paired_manifest_rejected(self):
        candidate,manifest,status=self.build()
        m=bytearray(manifest);m[-1]^=1
        with self.assertRaisesRegex(engine.Reject,'WLPA CRC invalid'):
            engine.verify_pair(self.source,candidate,bytes(m),self.rows,status['seven_homebrew_title_ids'])

    def test_corrupted_candidate_outside_game_zone_rejected(self):
        candidate,manifest,status=self.build()
        c=bytearray(candidate);c[9610]^=1
        with self.assertRaisesRegex(engine.Reject,'non-target sections'):
            engine.verify_pair(self.source,bytes(c),manifest,self.rows,status['seven_homebrew_title_ids'])

    def test_oversized_preview_rejected(self):
        rows=[dict(self.rows[0],menu_position=str(i+16),title_id=f'{0x0005000010001000+i:016X}') for i in range(286)]
        with self.assertRaisesRegex(engine.Reject,'fit 1 to 285'):
            # validation against inventory should occur before make_pair
            with tempfile.TemporaryDirectory() as d:
                ids,p,inv,a=mk_fixture(d,286)
                engine.load_preview(p,inv,a)

    def test_bad_preview_position_rejected(self):
        txt=self.preview.read_text().replace(',16,',',17,',1)
        self.preview.write_text(txt)
        with self.assertRaisesRegex(engine.Reject,'consecutive positions'):
            engine.load_preview(self.preview,self.inventory,self.audit)

    def test_preview_not_installed_rejected(self):
        data=self.inventory.read_text().splitlines()
        self.inventory.write_text('\n'.join(data[:-1])+'\n')
        with self.assertRaisesRegex(engine.Reject,'not installed'):
            engine.load_preview(self.preview,self.inventory,self.audit)

    def test_tampered_audit_status_rejected(self):
        s=self.audit.read_text().replace('CONFIRMED','REVIEW',1)
        self.audit.write_text(s)
        with self.assertRaisesRegex(engine.Reject,'unconfirmed'):
            engine.load_preview(self.preview,self.inventory,self.audit)

    def test_manual_overlap_rejected(self):
        self.rows[1]['title_id']=self.rows[0]['title_id']
        with self.assertRaisesRegex(engine.Reject,'Allowlist IDs overlap'):
            # The pair helper relies on load_preview for uniqueness, while the
            # manifest layer independently rejects a duplicate game ID.
            self.build()

if __name__ == '__main__':unittest.main(verbosity=2)
