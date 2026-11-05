"""Offline regression tests for release triggers, archives, and publication failures."""

import copy
import hashlib
import io
import json
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import MagicMock, patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
import release
from merge_packages import merge_indexes, version_key

SHA = '123456789abcdef0123456789abcdef0123456789a'


def index(version, tag=None):
    value = json.loads((ROOT / 'package/package_k210_index.template.json').read_text())
    platform = value['packages'][0]['platforms'][0]
    platform['version'] = version
    if tag:
        platform['url'] = f'https://github.com/owner/repo/releases/download/{tag}/k210-{version}.zip'
    return value


def versions(value):
    return [item['version'] for item in value['packages'][0]['platforms']]


class FakeGitHub:
    def __init__(self, stable=None, dev=None, existing=None):
        self.repository = 'owner/repo'
        self.indexes = {release.STABLE_INDEX: stable or {'packages': []}, release.DEV_INDEX: dev or {'packages': []}}
        self.existing = existing
        if existing:
            existing.setdefault('tag_name', 'v1.2.3')
        self.assets = []
        self.calls = []
        self.fail_asset = None
        self.fail_indexes = False
        self.fail_finalize = False
        self.releases = []
        self.tags = set()
        self.fail_delete = None

    def request(self, path, method='GET', data=None, missing_ok=False):
        self.calls.append((path, method, copy.deepcopy(data)))
        if path.startswith('releases/tags/'):
            tag = path.removeprefix('releases/tags/')
            return next(
                (
                    item
                    for item in self.releases + ([self.existing] if self.existing else [])
                    if item['tag_name'] == tag and not item['draft']
                ),
                None,
            )
        if path.startswith('releases?per_page=100&page='):
            start = (int(path.rsplit('=', 1)[1]) - 1) * 100
            return (self.releases + ([self.existing] if self.existing else []))[start : start + 100]
        if path == 'git/matching-refs/tags/dev-':
            return [{'ref': 'refs/tags/' + tag} for tag in sorted(self.tags) if tag.startswith('dev-')]
        if method == 'DELETE' and path == self.fail_delete:
            raise RuntimeError('Simulated cleanup failure')
        if path.startswith('git/refs/tags/') and method == 'DELETE':
            self.tags.remove(path.removeprefix('git/refs/tags/'))
            return None
        if path.startswith('releases/') and method == 'DELETE' and not path.startswith('releases/assets/'):
            release_id = int(path.rsplit('/', 1)[1])
            self.releases = [item for item in self.releases if item['id'] != release_id]
            return None
        if path == 'git/ref/heads/gh-pages':
            return {'object': {'sha': 'old-pages'}}
        if path == 'releases' and method == 'POST':
            self.existing = dict(data, id=42, upload_url='https://uploads.github.com/assets{?name}')
            return self.existing
        if path.startswith('releases/42/assets?'):
            return self.assets
        if path.startswith('releases/assets/') and method == 'DELETE':
            asset_id = int(path.rsplit('/', 1)[1])
            self.assets = [asset for asset in self.assets if asset['id'] != asset_id]
            return None
        if path.startswith('https://uploads.github.com/assets?name='):
            name = path.split('name=')[1]
            if name == self.fail_asset:
                raise RuntimeError('Simulated asset failure')
            asset = dict(
                id=len(self.calls),
                name=name,
                state='uploaded',
                size=len(data),
                digest='sha256:' + hashlib.sha256(data).hexdigest(),
            )
            self.assets.append(asset)
            return asset
        if path == 'releases/42' and method == 'PATCH':
            if self.fail_finalize:
                raise RuntimeError('Simulated finalization failure')
            self.existing.update(data)
            self.tags.add(self.existing['tag_name'])
            return self.existing
        raise AssertionError(f'Unexpected request: {method} {path}')

    def read_index(self, name, commit):
        return self.indexes[name]

    def update_indexes(self, indexes, parent, tag):
        self.calls.append(('update_indexes', 'WRITE', indexes))
        if self.fail_indexes:
            raise RuntimeError('Simulated index failure')
        assert self.existing['draft'] is False
        assert parent == 'old-pages'
        self.indexes.update({name: json.loads(value) for name, value in indexes.items()})


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.output = Path(self.temp.name)
        self.meta = dict(tag='v1.2.3', version='1.2.3', channel='stable', sha=SHA, timestamp=1700000000)

    def metadata(self, name='push', event=None, ref='refs/tags/v1.2.3', tag_sha=SHA):
        def git(*args, **kwargs):
            if args[:1] == ('show',):
                return '1700000000'
            return SHA if args[1] == 'HEAD' else tag_sha

        with patch.object(release, 'git', side_effect=git):
            return release.release_metadata(name, event or {}, ref)

    def test_tag_normalization_and_classification(self):
        for tag, channel in [('1.2.3', 'stable'), ('v1.2.3', 'stable'), ('v1.2.3-rc.1', 'prerelease')]:
            with self.subTest(tag=tag):
                result = self.metadata(ref='refs/tags/' + tag)
                self.assertEqual(result['tag'], tag)
                self.assertEqual(result['version'], tag.removeprefix('v'))
                self.assertEqual(result['channel'], channel)

    def test_dev_commit_identity_is_repeatable(self):
        first = self.metadata(ref='refs/heads/main')
        second = self.metadata('workflow_dispatch', ref='refs/heads/main')
        self.assertEqual(first, second)
        self.assertEqual(first['channel'], 'dev')
        self.assertIn('20231114221320', first['version'])
        self.assertIn(SHA[:12], first['tag'])
        self.assertNotEqual(first['tag'], 'dev')

    def test_release_event_preserves_numeric_prerelease(self):
        result = self.metadata(
            'release', {'action': 'published', 'release': {'tag_name': 'v1.2.3', 'prerelease': True, 'draft': False}}
        )
        self.assertEqual(result['channel'], 'prerelease')

    def test_reject_wrong_events_refs_and_versions(self):
        for name, ref in [
            ('pull_request', 'refs/heads/main'),
            ('push', 'refs/heads/feature'),
            ('push', 'refs/tags/dev'),
            ('push', 'refs/tags/v1.2'),
            ('push', 'refs/tags/1.2.3;echo-oops'),
        ]:
            with self.subTest(name=name, ref=ref), self.assertRaises(ValueError):
                self.metadata(name, ref=ref)
        with self.assertRaises(ValueError):
            self.metadata('release', {'action': 'created', 'release': {'draft': True}})

    def test_reject_moved_tag(self):
        with self.assertRaisesRegex(ValueError, 'does not match'):
            self.metadata(tag_sha='a' * 40)

    def test_archive_versions_licenses_and_checksum(self):
        archive, package = release.build_package(ROOT, self.output, self.meta, 'owner/repo')
        platform = package['packages'][0]['platforms'][0]
        data = archive.read_bytes()
        self.assertEqual(platform['size'], str(len(data)))
        self.assertEqual(platform['checksum'], 'SHA-256:' + hashlib.sha256(data).hexdigest())
        self.assertIn('/v1.2.3/k210-1.2.3.zip', platform['url'])
        with zipfile.ZipFile(archive) as bundle:
            prefix = 'k210-1.2.3/'
            self.assertIn('version=1.2.3\n', bundle.read(prefix + 'platform.txt').decode())
            self.assertEqual(json.loads(bundle.read(prefix + 'package.json'))['version'], '1.2.3')
            header = bundle.read(prefix + 'cores/k210/k210_arduino_version.h').decode()
            self.assertIn('#define K210_ARDUINO_VERSION_MAJOR 1', header)
            self.assertIn('#define K210_ARDUINO_VERSION_MINOR 2', header)
            self.assertIn('#define K210_ARDUINO_VERSION_PATCH 3', header)
            self.assertIn(b'"v1.2.3"', bundle.read(prefix + 'cores/k210/core_version.h'))
            license_text = bundle.read(prefix + 'LICENSE').decode()
            self.assertIn('Copyright (c) 2022, Canaan Bright Sight Co., Ltd', license_text)
            self.assertIn('Copyright (c) 2026, arduino-k210 contributors', license_text)
            self.assertEqual(bundle.read(prefix + 'LICENSE'), (ROOT / 'LICENSE').read_bytes())
            third_party_license = 'cores/k210/components/vfs/LICENSE'
            self.assertEqual(bundle.read(prefix + third_party_license), (ROOT / third_party_license).read_bytes())
        second, _ = release.build_package(ROOT, self.output / 'again', self.meta, 'owner/repo')
        self.assertEqual(data, second.read_bytes())

    def test_merge_retains_history_tools_and_prefers_new(self):
        first, second = index('1.2.3'), index('1.2.3')
        first['packages'][0]['platforms'][0]['url'] = 'new'
        second['packages'][0]['platforms'].append(index('1.0.0')['packages'][0]['platforms'][0])
        second['packages'][0]['tools'].append(dict(name='old-tool', version='1.0.0', systems=[]))
        result = merge_indexes(first, second)
        self.assertEqual(versions(result), ['1.2.3', '1.0.0'])
        self.assertEqual(result['packages'][0]['platforms'][0]['url'], 'new')
        self.assertEqual(len(result['packages'][0]['tools']), 3)
        self.assertEqual(versions(first), ['1.2.3'])  # Inputs must not be mutated.

    def test_semantic_version_order(self):
        values = ['1.2.3-rc.2', '1.2.3-dev.20260101', '1.2.3-rc.10', '1.2.3', '1.10.0', '1.9.0', '1.2.3-9']
        self.assertEqual(
            sorted(values, key=version_key, reverse=True),
            ['1.10.0', '1.9.0', '1.2.3', '1.2.3-rc.10', '1.2.3-rc.2', '1.2.3-dev.20260101', '1.2.3-9'],
        )

    def prepare_publish(self, api=None, channel='stable'):
        self.meta['channel'] = channel
        if channel == 'dev':
            self.meta.update(tag='dev-20231114221320-123456789abc', version='1.2.3-dev.20231114221320.g123456789abc')
        else:
            self.meta.update(tag='v1.2.3', version='1.2.3')
        archive = self.output / f'k210-{self.meta["version"]}.zip'
        archive.write_bytes(b'fixture archive')
        return api or FakeGitHub(index('1.0.0'), index('1.1.0-rc.1')), archive

    def publish(self, api, archive):
        release.publish(api, self.meta, archive, index(self.meta['version'], self.meta['tag']), self.output)

    def old_dev(self, api, number=1, date='20231113221320'):
        tag = f'dev-{date}-{number:012x}'
        version = f'1.2.3-dev.{date}.g{number:012x}'
        api.indexes[release.DEV_INDEX] = merge_indexes(api.indexes[release.DEV_INDEX], index(version, tag))
        api.releases.append({'id': 1000 + number, 'tag_name': tag, 'draft': False, 'prerelease': True})
        api.tags.add(tag)
        return tag, version

    def test_dev_retention_cleans_index_releases_and_orphan_tags_after_publication(self):
        api, archive = self.prepare_publish(channel='dev')
        old_tag, old_version = self.old_dev(api)
        orphan = 'dev-20231112221320-000000000002'
        api.tags.add(orphan)
        # Preserve unrelated prereleases, stable releases, and non-generated tags.
        api.releases.extend(
            [
                {'id': 10, 'tag_name': 'v1.1.0-rc.1', 'draft': False, 'prerelease': True},
                {'id': 11, 'tag_name': 'v1.0.0', 'draft': False, 'prerelease': False},
                {'id': 12, 'tag_name': 'dev-manual', 'draft': False, 'prerelease': True},
            ]
        )
        api.tags.update(item['tag_name'] for item in api.releases)
        stable = copy.deepcopy(api.indexes[release.STABLE_INDEX])
        self.publish(api, archive)
        self.assertEqual(versions(api.indexes[release.DEV_INDEX]), [self.meta['version'], '1.1.0-rc.1', '1.0.0'])
        self.assertEqual(api.indexes[release.STABLE_INDEX], stable)
        self.assertEqual({item['tag_name'] for item in api.releases}, {'v1.1.0-rc.1', 'v1.0.0', 'dev-manual'})
        self.assertEqual(api.tags, {self.meta['tag'], 'v1.1.0-rc.1', 'v1.0.0', 'dev-manual'})
        index_update = next(i for i, call in enumerate(api.calls) if call[0] == 'update_indexes')
        self.assertTrue(all(i > index_update for i, call in enumerate(api.calls) if call[1] == 'DELETE'))
        self.assertNotIn(old_version, versions(json.loads((self.output / release.DEV_INDEX).read_text())))
        self.assertIn((f'git/refs/tags/{old_tag}', 'DELETE', None), api.calls)

    def test_dev_retention_preserves_other_packages_architectures_and_tools(self):
        api, archive = self.prepare_publish(channel='dev')
        old_tag, _ = self.old_dev(api)
        other = index('9.0.0-dev.20231112221320.g000000000002', 'dev-20231112221320-000000000002')
        other['packages'][0]['platforms'][0]['architecture'] = 'OTHER'
        api.indexes[release.DEV_INDEX] = merge_indexes(api.indexes[release.DEV_INDEX], other)
        external = copy.deepcopy(other['packages'][0])
        external['name'] = 'AnotherVendor'
        api.indexes[release.DEV_INDEX]['packages'].append(external)
        api.indexes[release.DEV_INDEX]['packages'][0]['tools'].append(
            {'name': 'old-tool', 'version': '1.0.0', 'systems': []}
        )
        self.publish(api, archive)
        result = api.indexes[release.DEV_INDEX]
        self.assertEqual(result['packages'][1], external)
        self.assertIn(other['packages'][0]['platforms'][0], result['packages'][0]['platforms'])
        self.assertIn({'name': 'old-tool', 'version': '1.0.0', 'systems': []}, result['packages'][0]['tools'])
        self.assertNotIn(old_tag, api.tags)

    def test_dev_publication_failures_do_not_delete_old_snapshots(self):
        for failure in ['fail_asset', 'fail_finalize', 'fail_indexes']:
            with self.subTest(failure=failure):
                api, archive = self.prepare_publish(channel='dev')
                tag, version = self.old_dev(api)
                setattr(api, failure, release.DEV_INDEX if failure == 'fail_asset' else True)
                with self.assertRaises(RuntimeError):
                    self.publish(api, archive)
                self.assertIn(tag, api.tags)
                self.assertIn(version, versions(api.indexes[release.DEV_INDEX]))
                self.assertFalse(any(method == 'DELETE' for _, method, _ in api.calls))

    def test_dev_cleanup_failure_can_be_retried_after_index_is_updated(self):
        for fail_tag in [False, True]:
            with self.subTest(fail_tag=fail_tag):
                api, archive = self.prepare_publish(channel='dev')
                tag, version = self.old_dev(api)
                api.fail_delete = f'git/refs/tags/{tag}' if fail_tag else 'releases/1001'
                with self.assertRaisesRegex(RuntimeError, 'cleanup failure'):
                    self.publish(api, archive)
                self.assertNotIn(version, versions(api.indexes[release.DEV_INDEX]))
                self.assertFalse(api.existing['draft'])
                api.fail_delete = None
                api.calls.clear()
                self.publish(api, archive)
                self.assertNotIn(tag, api.tags)
                self.assertEqual(api.releases, [])
                self.assertFalse(any(method in {'POST', 'PATCH', 'WRITE'} for _, method, _ in api.calls))

    def test_old_dev_rerun_does_not_replace_newer_index_or_published_release(self):
        for indexed in [False, True]:
            with self.subTest(indexed=indexed):
                api, archive = self.prepare_publish(channel='dev')
                tag, _ = self.old_dev(api, date='20231115221320')
                if not indexed:
                    api.indexes[release.DEV_INDEX] = index('1.0.0')
                before = copy.deepcopy(api.indexes)
                self.publish(api, archive)
                self.assertEqual(api.indexes, before)
                self.assertIn(tag, api.tags)
                self.assertFalse(any(method != 'GET' for _, method, _ in api.calls))

    def test_dev_cleanup_enumerates_all_pages_before_deleting(self):
        api, archive = self.prepare_publish(channel='dev')
        for number in range(1, 103):
            self.old_dev(api, number)
        self.publish(api, archive)
        self.assertEqual(api.releases, [])
        self.assertEqual(api.tags, {self.meta['tag']})
        reads = [i for i, call in enumerate(api.calls) if call[0].startswith('releases?')]
        deletes = [i for i, call in enumerate(api.calls) if call[1] == 'DELETE']
        self.assertEqual(len(reads), 2)
        self.assertLess(max(reads), min(deletes))

    def test_stable_and_rc_publish_keep_latest_dev_without_deleting_releases(self):
        for channel in ['stable', 'prerelease']:
            with self.subTest(channel=channel):
                api, archive = self.prepare_publish(channel=channel)
                _, old_version = self.old_dev(api)
                latest_tag, latest_version = self.old_dev(api, number=2, date='20231115221320')
                self.publish(api, archive)
                self.assertNotIn(old_version, versions(api.indexes[release.DEV_INDEX]))
                self.assertIn(latest_version, versions(api.indexes[release.DEV_INDEX]))
                self.assertIn(latest_tag, api.tags)
                self.assertFalse(any(method == 'DELETE' for _, method, _ in api.calls))

    def test_latest_dev_is_selected_by_snapshot_date_not_base_version(self):
        api, archive = self.prepare_publish(channel='dev')
        _, old_version = self.old_dev(api)
        self.meta['version'] = '0.9.91-dev.20231114221320.g123456789abc'
        self.publish(api, archive)
        self.assertNotIn(old_version, versions(api.indexes[release.DEV_INDEX]))
        self.assertIn(self.meta['version'], versions(api.indexes[release.DEV_INDEX]))
        self.assertEqual(api.tags, {self.meta['tag']})

    def test_successful_dev_rerun_does_not_write_again(self):
        api, archive = self.prepare_publish(channel='dev')
        self.old_dev(api)
        self.publish(api, archive)
        api.calls.clear()
        self.publish(api, archive)
        self.assertFalse(any(method != 'GET' for _, method, _ in api.calls))

    def test_cleanup_preserves_newer_drafts_and_tags_still_referenced_by_indexes(self):
        api, archive = self.prepare_publish(channel='dev')
        old_tag, old_version = self.old_dev(api)
        referenced = index(old_version, old_tag)['packages'][0]['platforms'][0]
        referenced['architecture'] = 'OTHER'
        api.indexes[release.DEV_INDEX]['packages'][0]['platforms'].append(referenced)
        stable_tag = 'dev-20231112221320-000000000002'
        draft_tag = 'dev-20231115221320-000000000003'
        api.releases.extend(
            [
                {'id': 10, 'tag_name': stable_tag, 'draft': False, 'prerelease': False},
                {'id': 11, 'tag_name': draft_tag, 'draft': True, 'prerelease': True},
            ]
        )
        api.tags.update([stable_tag, draft_tag])
        self.publish(api, archive)
        self.assertEqual(api.tags, {self.meta['tag'], old_tag, stable_tag, draft_tag})
        self.assertFalse(any(method == 'DELETE' for _, method, _ in api.calls))

    def test_stable_updates_both_indexes_after_upload_and_publish(self):
        api, archive = self.prepare_publish()
        self.publish(api, archive)
        self.assertEqual(versions(api.indexes[release.STABLE_INDEX]), ['1.2.3', '1.0.0'])
        self.assertEqual(versions(api.indexes[release.DEV_INDEX]), ['1.2.3', '1.1.0-rc.1', '1.0.0'])
        self.assertEqual(api.calls[-1][0], 'update_indexes')
        self.assertFalse(api.existing['prerelease'])
        self.assertEqual(api.existing['target_commitish'], SHA)
        self.assertEqual(len(api.assets), 3)

    def test_dev_and_rc_leave_stable_unchanged(self):
        for channel in ['dev', 'prerelease']:
            api, archive = self.prepare_publish(channel=channel)
            before = copy.deepcopy(api.indexes[release.STABLE_INDEX])
            self.publish(api, archive)
            self.assertEqual(api.indexes[release.STABLE_INDEX], before)
            self.assertTrue(api.existing['prerelease'])
            self.assertEqual(api.existing['make_latest'], 'false')
            self.assertEqual(len(api.assets), 2)

    def test_asset_failure_does_not_publish_indexes(self):
        api, archive = self.prepare_publish()
        api.fail_asset = release.DEV_INDEX
        with self.assertRaises(RuntimeError):
            self.publish(api, archive)
        self.assertTrue(api.existing['draft'])
        self.assertFalse(any(call[0] == 'update_indexes' for call in api.calls))
        api.fail_asset = None
        self.publish(api, archive)
        self.assertFalse(api.existing['draft'])
        self.assertEqual(len(api.assets), 3)

    def test_finalize_failure_does_not_publish_indexes(self):
        api, archive = self.prepare_publish()
        api.fail_finalize = True
        with self.assertRaises(RuntimeError):
            self.publish(api, archive)
        self.assertFalse(any(call[0] == 'update_indexes' for call in api.calls))

    def test_rerun_replaces_starter_assets_before_publishing(self):
        for name in ['k210-1.2.3.zip', release.DEV_INDEX, release.STABLE_INDEX]:
            with self.subTest(asset=name):
                api, archive = self.prepare_publish()
                api.fail_finalize = True
                with self.assertRaises(RuntimeError):
                    self.publish(api, archive)
                asset = next(item for item in api.assets if item['name'] == name)
                asset.update(state='starter', size=0, digest=None)
                asset_id = asset['id']
                api.fail_finalize = False
                api.calls.clear()
                self.publish(api, archive)
                self.assertIn((f'releases/assets/{asset_id}', 'DELETE', None), api.calls)
                uploads = [call for call in api.calls if call[1] == 'POST']
                self.assertEqual(len(uploads), 1)
                self.assertTrue(uploads[0][0].endswith('?name=' + name))
                self.assertEqual(len(api.assets), 3)
                self.assertTrue(all(item['state'] == 'uploaded' and item['size'] > 0 for item in api.assets))
                self.assertFalse(api.existing['draft'])
                self.assertEqual(api.calls[-1][0], 'update_indexes')

    def test_incomplete_upload_response_keeps_release_draft(self):
        for name in ['k210-1.2.3.zip', release.DEV_INDEX]:
            api, archive = self.prepare_publish()
            original = api.request

            def request(path, method='GET', data=None, **kwargs):
                result = original(path, method, data, **kwargs)
                if method == 'POST' and path.endswith('?name=' + name):
                    result['state'] = 'starter'
                return result

            with patch.object(api, 'request', side_effect=request):
                with self.assertRaisesRegex(ValueError, 'Upload did not complete'):
                    self.publish(api, archive)
            self.assertTrue(api.existing['draft'])
            self.assertFalse(any(call[0] == 'update_indexes' for call in api.calls))

    def test_invalid_completed_asset_is_not_deleted_or_published(self):
        for state, size in [('uploaded', 0), ('unknown', 100)]:
            api, archive = self.prepare_publish()
            api.assets = [dict(id=99, name=release.DEV_INDEX, state=state, size=size)]
            with self.assertRaisesRegex(ValueError, 'not a completed upload'):
                self.publish(api, archive)
            self.assertTrue(api.existing['draft'])
            self.assertFalse(any(method == 'DELETE' or path == 'update_indexes' for path, method, _ in api.calls))

    def test_rerun_recovers_failed_index_update(self):
        api, archive = self.prepare_publish()
        api.fail_indexes = True
        with self.assertRaises(RuntimeError):
            self.publish(api, archive)
        api.fail_indexes = False
        self.publish(api, archive)
        self.assertEqual(len(api.assets), 3)
        self.assertEqual(versions(api.indexes[release.STABLE_INDEX]), ['1.2.3', '1.0.0'])

    def test_successful_rerun_does_not_upload_or_commit_again(self):
        api, archive = self.prepare_publish()
        self.publish(api, archive)
        api.calls.clear()
        self.publish(api, archive)
        self.assertFalse(any(method != 'GET' for _, method, _ in api.calls))

    def test_existing_release_notes_are_preserved(self):
        api, archive = self.prepare_publish()
        self.publish(api, archive)
        api.existing['body'] = 'Maintainer-written notes'
        self.publish(api, archive)
        self.assertEqual(api.existing['body'], 'Maintainer-written notes')

    def test_existing_package_cannot_be_silently_replaced(self):
        api, archive = self.prepare_publish()
        self.publish(api, archive)
        archive.write_bytes(b'different package')
        api.calls.clear()
        with self.assertRaisesRegex(ValueError, 'different package bytes'):
            self.publish(api, archive)
        self.assertFalse(any(method != 'GET' for _, method, _ in api.calls))

    def test_v_prefix_cannot_overwrite_an_existing_arduino_version(self):
        previous = index('1.2.3')
        previous['packages'][0]['platforms'][0]['checksum'] = 'SHA-256:old'
        api, archive = self.prepare_publish(FakeGitHub(stable=previous))
        with self.assertRaisesRegex(ValueError, 'use a new version'):
            self.publish(api, archive)
        self.assertFalse(any(method != 'GET' for _, method, _ in api.calls))

    def test_atomic_index_update_preserves_other_files(self):
        api = release.GitHub('owner/repo', 'test-token')
        with patch.object(
            api,
            'request',
            side_effect=[
                {'tree': {'sha': 'old-tree'}},
                {'sha': 'new-tree'},
                {'sha': 'new-commit'},
                {},
            ],
        ) as request:
            api.update_indexes(
                {release.DEV_INDEX: 'dev json', release.STABLE_INDEX: 'stable json'}, 'old-commit', 'v1.2.3'
            )
        calls = request.call_args_list
        self.assertEqual(calls[1].args[2]['base_tree'], 'old-tree')
        self.assertEqual(len(calls[1].args[2]['tree']), 2)
        self.assertEqual(calls[2].args[2]['parents'], ['old-commit'])
        self.assertEqual(calls[3].args, ('git/refs/heads/gh-pages', 'PATCH', {'sha': 'new-commit', 'force': False}))

    def test_missing_pages_branch_is_initialized(self):
        api = release.GitHub('owner/repo', 'test-token')
        with patch.object(api, 'request', side_effect=[{'sha': 'tree'}, {'sha': 'commit'}, {}]) as request:
            api.update_indexes({release.DEV_INDEX: 'dev json'}, None, 'dev-tag')
        calls = request.call_args_list
        self.assertNotIn('base_tree', calls[0].args[2])
        self.assertEqual(calls[1].args[2]['parents'], [])
        self.assertEqual(calls[2].args, ('git/refs', 'POST', {'ref': 'refs/heads/gh-pages', 'sha': 'commit'}))

    def test_numeric_prerelease_is_never_added_to_stable(self):
        for draft in [True, False]:
            api, archive = self.prepare_publish(
                FakeGitHub(
                    index('1.0.0'),
                    existing={
                        'id': 42,
                        'draft': draft,
                        'prerelease': True,
                        'upload_url': 'https://uploads.github.com/assets{?name}',
                    },
                )
            )
            self.publish(api, archive)
            self.assertEqual(versions(api.indexes[release.STABLE_INDEX]), ['1.0.0'])

    def test_api_auth_and_errors(self):
        from urllib.error import HTTPError

        api = release.GitHub('owner/repo', 'test-token')
        with patch.object(release, 'urlopen', side_effect=HTTPError('url', 404, 'missing', {}, None)):
            self.assertIsNone(api.request('missing', missing_ok=True))
        with patch.object(
            release, 'urlopen', side_effect=HTTPError('url', 403, 'forbidden', {}, io.BytesIO(b'denied'))
        ):
            with self.assertRaisesRegex(RuntimeError, 'HTTP 403'):
                api.request('missing', missing_ok=True)

    def test_asset_delete_accepts_empty_204_response(self):
        api = release.GitHub('owner/repo', 'test-token')
        response = MagicMock(status=204)
        response.__enter__.return_value = response
        with patch.object(release, 'urlopen', return_value=response) as request:
            self.assertIsNone(api.request('releases/assets/99', 'DELETE'))
        self.assertEqual(request.call_args.args[0].get_method(), 'DELETE')
        response.read.assert_not_called()


if __name__ == '__main__':
    unittest.main()
