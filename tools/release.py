#!/usr/bin/env python3
"""Build reproducible Arduino packages; publish only with explicit --publish."""

import argparse
import base64
import hashlib
import json
import os
import re
import subprocess
import zipfile
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError
from urllib.parse import quote
from urllib.request import Request, urlopen

from merge_packages import merge_indexes

ROOT = Path(__file__).resolve().parents[1]
STABLE_INDEX = 'package_k210_index.json'
DEV_INDEX = 'package_k210_dev_index.json'
VERSION = re.compile(r'v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?')


def git(*args, root=ROOT):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def release_metadata(event_name, event, ref, root=ROOT):
    sha = git('rev-parse', 'HEAD', root=root)
    timestamp = int(git('show', '-s', '--format=%ct', 'HEAD', root=root))
    if event_name not in {'push', 'release', 'workflow_dispatch'}:
        raise ValueError(f'Cannot publish from {event_name}')
    release = event.get('release', {})
    if event_name == 'release':
        if event.get('action') != 'published' or release.get('draft'):
            raise ValueError('Only published release events are supported')
        tag = release['tag_name']
    elif ref.startswith('refs/tags/'):
        tag = ref.removeprefix('refs/tags/')
    elif ref == 'refs/heads/main':
        tag = None
    else:
        raise ValueError('Development releases must come from main')
    if tag is None:
        base = json.loads((root / 'package.json').read_text())['version']
        match = VERSION.fullmatch(base)
        if not match:
            raise ValueError(f'Invalid package.json version: {base}')
        date = datetime.fromtimestamp(timestamp, timezone.utc).strftime('%Y%m%d%H%M%S')
        version = '.'.join(match.group(1, 2, 3)) + f'-dev.{date}.g{sha[:12]}'
        tag = f'dev-{date}-{sha[:12]}'
        channel = 'dev'
    else:
        match = VERSION.fullmatch(tag)
        if not match:
            raise ValueError(f'Expected a tag like 1.2.3, v1.2.3, or 1.2.3-rc.1: {tag}')
        if git('rev-parse', f'refs/tags/{tag}^{{commit}}', root=root) != sha:
            raise ValueError('Checked-out commit does not match the release tag')
        version = tag.removeprefix('v')
        channel = 'prerelease' if match.group(4) or release.get('prerelease') else 'stable'
    return dict(tag=tag, version=version, channel=channel, sha=sha, timestamp=timestamp)


def build_package(root, output, metadata, repository):
    output.mkdir(parents=True, exist_ok=True)
    version = metadata['version']
    folder = f'k210-{version}'
    archive = output / f'{folder}.zip'
    package = json.loads((root / 'package.json').read_text())
    package['version'] = version
    platform, count = re.subn(
        r'^version=.*$', f'version={version}', (root / 'platform.txt').read_text(), flags=re.MULTILINE
    )
    if count != 1:
        raise ValueError('platform.txt must contain exactly one version')
    header = (root / 'cores/k210/k210_arduino_version.h').read_text()
    for key, value in zip(('MAJOR', 'MINOR', 'PATCH'), version.split('-', 1)[0].split('.')):
        header, count = re.subn(rf'(#define K210_ARDUINO_VERSION_{key})\s+\d+', rf'\g<1> {value}', header)
        if count != 1:
            raise ValueError(f'Missing version macro: {key}')
    definition = re.sub(r'[^A-Z0-9_]', '_', version.upper())
    overrides = {
        'platform.txt': platform.encode(),
        'package.json': (json.dumps(package, indent=2) + '\n').encode(),
        'cores/k210/k210_arduino_version.h': header.encode(),
        'cores/k210/core_version.h': (
            f'#define ARDUINO_K210_GIT_VER 0x{metadata["sha"][:8]}\n'
            f'#define ARDUINO_K210_GIT_DESC "{metadata["tag"]}"\n'
            f'#define ARDUINO_K210_RELEASE_{definition}\n'
            f'#define ARDUINO_K210_RELEASE "{version}"\n'
        ).encode(),
    }
    files = {}
    for name in (
        'cores',
        'libraries',
        'variants',
        'licenses',
        'LICENSE',
        'boards.txt',
        'platform.txt',
        'programmers.txt',
        'package.json',
    ):
        source = root / name
        for file in sorted(source.rglob('*')) if source.is_dir() else [source]:
            relative = file.relative_to(root)
            if (
                not file.is_file()
                or any(part.startswith('.git') or part == '__pycache__' for part in relative.parts)
                or file.name == '.DS_Store'
            ):
                continue
            files[relative.as_posix()] = file.read_bytes()
    files.update(overrides)
    date = datetime.fromtimestamp(max(metadata['timestamp'], 315532800), timezone.utc)
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(f'{folder}/{name}', date.timetuple()[:6])
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            bundle.writestr(info, data)
    index = json.loads((root / 'package/package_k210_index.template.json').read_text())
    index['packages'][0]['platforms'][0].update(
        version=version,
        archiveFileName=archive.name,
        size=str(archive.stat().st_size),
        checksum='SHA-256:' + hashlib.sha256(archive.read_bytes()).hexdigest(),
        url=f'https://github.com/{repository}/releases/download/{metadata["tag"]}/{archive.name}',
    )
    return archive, index


class GitHub:
    def __init__(self, repository, token):
        self.repository = repository
        self.token = token

    def request(self, path, method='GET', data=None, missing_ok=False):
        url = (
            path
            if path.startswith('https://uploads.github.com/')
            else (f'https://api.github.com/repos/{self.repository}/{path}')
        )
        binary = isinstance(data, bytes)
        request = Request(
            url,
            method=method,
            headers={
                'Authorization': f'Bearer {self.token}',
                'Accept': 'application/vnd.github+json',
                'Content-Type': 'application/octet-stream' if binary else 'application/json',
                'User-Agent': 'arduino-k210-release',
                'X-GitHub-Api-Version': '2026-03-10',
            },
            data=data if binary else json.dumps(data).encode() if data is not None else None,
        )
        try:
            with urlopen(request, timeout=180) as response:
                if response.status == 204:
                    return None
                return json.load(response)
        except HTTPError as error:
            if missing_ok and error.code == 404:
                return None
            raise RuntimeError(f'GitHub {method} {path}: HTTP {error.code}: ' + error.read().decode()) from error

    def read_index(self, name, commit):
        if commit is None:
            return {'packages': []}
        result = self.request(f'contents/{name}?ref={commit}', missing_ok=True)
        if result is None:
            return {'packages': []}
        return json.loads(base64.b64decode(result['content']))

    def update_indexes(self, indexes, parent, tag):
        tree = [
            {'path': name, 'mode': '100644', 'type': 'blob', 'content': content} for name, content in indexes.items()
        ]
        payload = {'tree': tree}
        if parent:
            payload['base_tree'] = self.request(f'git/commits/{parent}')['tree']['sha']
        tree_sha = self.request('git/trees', 'POST', payload)['sha']
        commit = self.request(
            'git/commits',
            'POST',
            {
                'message': f'Update Arduino indexes for {tag}',
                'tree': tree_sha,
                'parents': [parent] if parent else [],
            },
        )
        if parent:
            self.request('git/refs/heads/gh-pages', 'PATCH', {'sha': commit['sha'], 'force': False})
        else:
            self.request('git/refs', 'POST', {'ref': 'refs/heads/gh-pages', 'sha': commit['sha']})


def publish(api, metadata, archive, index, output):
    tag = metadata['tag']
    encoded = quote(tag, safe='')
    release = api.request(f'releases/tags/{encoded}', missing_ok=True)
    if release is None:
        # The tag endpoint returns published releases only. Find interrupted drafts
        # through the authenticated list endpoint, including older pages.
        page = 1
        while True:
            batch = api.request(f'releases?per_page=100&page={page}')
            release = next((item for item in batch if item['tag_name'] == tag), None)
            if release or len(batch) < 100:
                break
            page += 1
    prerelease = metadata['channel'] != 'stable'
    if release and release['prerelease']:
        # Honor manually selected prereleases, including drafts with numeric tags.
        prerelease = True
    elif release and not release['draft'] and prerelease:
        raise ValueError('Existing release classification conflicts with this build')
    ref = api.request('git/ref/heads/gh-pages', missing_ok=True)
    parent = ref['object']['sha'] if ref else None
    stable = api.read_index(STABLE_INDEX, parent)
    dev = api.read_index(DEV_INDEX, parent)
    package = index['packages'][0]
    platform = package['platforms'][0]
    for previous in (stable, dev):
        for old_package in previous['packages']:
            if old_package['name'] != package['name']:
                continue
            for old in old_package['platforms']:
                if (
                    old['architecture'] == platform['architecture']
                    and old['version'].removeprefix('v') == platform['version']
                    and old['checksum'] != platform['checksum']
                ):
                    raise ValueError(
                        f'Package version {platform["version"]} already has different bytes; use a new version'
                    )
    indexes = {DEV_INDEX: merge_indexes(index, dev, stable)}
    if not prerelease:
        indexes[STABLE_INDEX] = merge_indexes(index, stable)
    if release is None:
        release = api.request(
            'releases',
            'POST',
            {
                'tag_name': tag,
                'target_commitish': metadata['sha'],
                'name': f'Development {metadata["version"]}' if metadata['channel'] == 'dev' else tag,
                'body': f'Arduino K210 {metadata["version"]}\n\nSource: {metadata["sha"]}',
                'draft': True,
                'prerelease': prerelease,
            },
        )
    release_id = release['id']
    # List all assets, including when retrying an interrupted publication.
    assets = {}
    page = 1
    while True:
        batch = api.request(f'releases/{release_id}/assets?per_page=100&page={page}')
        assets.update({asset['name']: asset for asset in batch})
        if len(batch) < 100:
            break
        page += 1
    upload_url = release['upload_url'].split('{', 1)[0]

    def upload(path):
        data = path.read_bytes()
        asset = assets.get(path.name)
        if asset and asset['state'] == 'starter':
            # A failed GitHub upload (e.g. HTTP 502) can leave this empty placeholder.
            # Remove only unfinished uploads; completed package bytes stay immutable.
            api.request(f'releases/assets/{asset["id"]}', 'DELETE')
            asset = None
        if asset:
            if asset['state'] != 'uploaded' or asset['size'] <= 0:
                raise ValueError(f'Existing asset is not a completed upload: {path.name}')
            if path == archive:
                digest = 'sha256:' + hashlib.sha256(data).hexdigest()
                old_digest = asset.get('digest')
                if not old_digest:
                    # Older assets have no API digest. Verify their actual content.
                    with urlopen(asset['browser_download_url'], timeout=180) as response:
                        old_digest = 'sha256:' + hashlib.sha256(response.read()).hexdigest()
                if old_digest != digest:
                    raise ValueError(f'Refusing to replace different package bytes for {tag}')
            return  # Existing index assets remain snapshots of their original release.
        result = api.request(upload_url + '?name=' + quote(path.name), 'POST', data)
        if result['state'] != 'uploaded':
            raise ValueError(f'Upload did not complete: {path.name}')
        if result['size'] != len(data):
            raise ValueError(f'Uploaded size mismatch: {path.name}')

    upload(archive)
    content = {}
    for name, value in indexes.items():
        content[name] = json.dumps(value, indent=2) + '\n'
        path = output / name
        path.write_text(content[name])
        upload(path)
    if release['draft']:
        api.request(
            f'releases/{release_id}',
            'PATCH',
            {
                'draft': False,
                'prerelease': prerelease,
                'make_latest': 'false' if prerelease else 'legacy',
            },
        )
    # Publish URLs only after every asset is available. Both indexes change atomically.
    if any(indexes[name] != old for name, old in [(DEV_INDEX, dev), (STABLE_INDEX, stable)] if name in indexes):
        api.update_indexes(content, parent, tag)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--event-name', default=os.environ.get('GITHUB_EVENT_NAME'))
    parser.add_argument('--event', type=Path, default=os.environ.get('GITHUB_EVENT_PATH'))
    parser.add_argument('--ref', default=os.environ.get('GITHUB_REF', 'refs/heads/main'))
    parser.add_argument('--repository', default=os.environ.get('GITHUB_REPOSITORY', 'kendryte/arduino-k210'))
    parser.add_argument('--output', type=Path, default=ROOT / 'build/release')
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()
    if args.publish and (not args.event_name or not args.event):
        parser.error('--publish requires an Actions event name and event file')
    event = json.loads(args.event.read_text()) if args.event else {}
    metadata = release_metadata(args.event_name or 'workflow_dispatch', event, args.ref)
    archive, index = build_package(ROOT, args.output, metadata, args.repository)
    (args.output / 'release.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (args.output / DEV_INDEX).write_text(json.dumps(index, indent=2) + '\n')
    if metadata['channel'] == 'stable':
        (args.output / STABLE_INDEX).write_text(json.dumps(index, indent=2) + '\n')
    if args.publish:
        publish(GitHub(args.repository, os.environ['GH_TOKEN']), metadata, archive, index, args.output)
    print(json.dumps(metadata, indent=2))
    print(archive)


if __name__ == '__main__':
    main()
