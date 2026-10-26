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


def index(version):
    value = json.loads((ROOT / 'package/package_k210_index.template.json').read_text())
    value['packages'][0]['platforms'][0]['version'] = version
    return value


def versions(value):
    return [item['version'] for item in value['packages'][0]['platforms']]


class FakeGitHub:
    def __init__(self, stable=None, dev=None, existing=None):
        self.indexes = {release.STABLE_INDEX: stable or {'packages': []}, release.DEV_INDEX: dev or {'packages': []}}
        self.existing = existing
        if existing:
            existing.setdefault('tag_name', 'v1.2.3')
        self.assets = []
        self.calls = []
        self.fail_asset = None
        self.fail_indexes = False
        self.fail_finalize = False

    def request(self, path, method='GET', data=None, missing_ok=False):
        self.calls.append((path, method, copy.deepcopy(data)))
        if path.startswith('releases/tags/'):
            return self.existing if self.existing and not self.existing['draft'] else None
        if path.startswith('releases?per_page=100&page='):
            return [self.existing] if self.existing else []
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
            self.assertIn(prefix + 'licenses/README.md', bundle.namelist())
            self.assertIn(prefix + 'licenses/compatibility-sources.txt', bundle.namelist())
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
        archive = self.output / 'k210-1.2.3.zip'
        archive.write_bytes(b'fixture archive')
        return api or FakeGitHub(index('1.0.0'), index('1.1.0-rc.1')), archive

    def publish(self, api, archive):
        release.publish(api, self.meta, archive, index('1.2.3'), self.output)

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
