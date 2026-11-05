#!/usr/bin/env python3
"""Merge Arduino package indexes; the first index wins duplicate versions."""

import copy
import json
import sys


def version_key(version):
    """Apply SemVer precedence, including numeric prerelease identifiers."""
    value = version.removeprefix('v').split('+', 1)[0]
    base, _, suffix = value.partition('-')
    numbers = tuple(int(part) for part in base.split('.'))
    identifiers = tuple((0, int(part)) if part.isdigit() else (1, part) for part in suffix.split('.'))
    return numbers, not bool(suffix), identifiers


def merge_indexes(*indexes):
    packages = {}
    for index in indexes:
        for incoming in index['packages']:
            name = incoming['name']
            if name not in packages:
                packages[name] = copy.deepcopy(incoming)
                continue
            package = packages[name]
            for field, identity in [('platforms', 'architecture'), ('tools', 'name')]:
                existing = {(item[identity], item['version']) for item in package[field]}
                for item in incoming[field]:
                    key = item[identity], item['version']
                    if key not in existing:
                        package[field].append(copy.deepcopy(item))
                        existing.add(key)
    for package in packages.values():
        package['platforms'].sort(key=lambda item: version_key(item['version']), reverse=True)
    return {'packages': list(packages.values())}


def main():
    if len(sys.argv) < 3:
        raise SystemExit(f'Usage: {sys.argv[0]} <new-index> <old-index> [more-indexes...]')
    indexes = []
    for filename in sys.argv[1:]:
        with open(filename, encoding='utf-8') as stream:
            indexes.append(json.load(stream))
    print(json.dumps(merge_indexes(*indexes), indent=2))


if __name__ == '__main__':
    main()
