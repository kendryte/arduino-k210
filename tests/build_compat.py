#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Copyright (c) 2026, arduino-k210 contributors
"""Compile bundled sketches, audit source coverage, and never upload firmware."""

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {'.c', '.cpp', '.S'}
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.MULTILINE)


def discover_sketches():
    return sorted(
        {
            file.parent
            for folder in ('libraries', 'tests/testcases')
            for file in (ROOT / folder).rglob('*.ino')
            if file.stem == file.parent.name
        }
    )


def library_catalog():
    catalog = {}
    for properties in sorted((ROOT / 'libraries').glob('*/library.properties')):
        library = properties.parent.name
        headers = set()
        source = properties.parent / 'src'
        if source.is_dir():
            for header in source.rglob('*.h'):
                headers.add(header.name)
                headers.add(header.relative_to(source).as_posix())
        catalog[library] = headers
    return catalog


def direct_library_uses(sketch, catalog):
    includes = set()
    for source in sketch.rglob('*'):
        if source.is_file() and source.suffix in {'.ino', '.c', '.cpp', '.h', '.hpp'}:
            includes.update(INCLUDE_PATTERN.findall(source.read_text(errors='replace')))
    return {library for library, headers in catalog.items() if includes & headers}


def compiled_libraries(build_path):
    folder = build_path / 'libraries'
    if not folder.is_dir():
        return set()
    return {obj.relative_to(folder).parts[0] for obj in folder.rglob('*.o') if obj.relative_to(folder).parts}


def source_names(folder, recursive=True):
    iterator = folder.rglob('*') if recursive else folder.glob('*')
    return {
        source.relative_to(folder).as_posix()
        for source in iterator
        if source.is_file() and source.suffix in SOURCE_SUFFIXES
    }


def object_source_names(folder, recursive=True):
    if not folder.is_dir():
        return set()
    iterator = folder.rglob('*.o') if recursive else folder.glob('*.o')
    names = set()
    for obj in iterator:
        relative = obj.relative_to(folder).as_posix()
        if relative.endswith('.o'):
            names.add(relative[:-2])
    return names


def relative_name(path):
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def unit_coverage(expected, observed):
    return {
        'total': len(expected),
        'covered': sorted(expected & observed),
        'missing': sorted(expected - observed),
    }


def create_coverage(full_matrix, results, library_evidence, observed_sources):
    libraries = []
    for library in sorted(library_catalog()):
        evidence = sorted(library_evidence.get(library, set()))
        libraries.append({'name': library, 'covered': bool(evidence), 'sketches': evidence})

    return {
        'schema_version': 1,
        'full_matrix': full_matrix,
        'sketches': {
            'total': len(results),
            'passed': sum(result['passed'] for result in results),
            'failed': [relative_name(Path(result['sketch'])) for result in results if not result['passed']],
        },
        'libraries': libraries,
        'core': unit_coverage(
            source_names(ROOT / 'cores/k210', recursive=False),
            observed_sources['core'],
        ),
        'sdk_drivers': unit_coverage(
            source_names(ROOT / 'cores/k210/k210-sdk/drivers', recursive=True),
            observed_sources['sdk_drivers'],
        ),
        'hal_drivers': unit_coverage(
            source_names(ROOT / 'cores/k210/k210-hal', recursive=True),
            observed_sources['hal_drivers'],
        ),
    }


def write_coverage(output, coverage):
    (output / 'coverage.json').write_text(json.dumps(coverage, indent=2) + '\n')
    covered_libraries = sum(item['covered'] for item in coverage['libraries'])
    lines = [
        '# Firmware compile coverage',
        '',
        'This report verifies that Arduino CLI selected every bundled library and compiled every core and driver source file. Runtime behavior is checked separately by the onboard hardware suite.',
        '',
        '| Area | Covered | Missing |',
        '| --- | ---: | ---: |',
        f'| Sketches | {coverage["sketches"]["passed"]} | {len(coverage["sketches"]["failed"])} |',
        f'| Bundled libraries | {covered_libraries} | {len(coverage["libraries"]) - covered_libraries} |',
        f'| Core sources | {len(coverage["core"]["covered"])} | {len(coverage["core"]["missing"])} |',
        f'| SDK driver sources | {len(coverage["sdk_drivers"]["covered"])} | {len(coverage["sdk_drivers"]["missing"])} |',
        f'| HAL driver sources | {len(coverage["hal_drivers"]["covered"])} | {len(coverage["hal_drivers"]["missing"])} |',
        '',
        '## Libraries',
        '',
        '| Library | Status | Evidence |',
        '| --- | --- | --- |',
    ]
    for item in coverage['libraries']:
        evidence = '<br>'.join(item['sketches']) if item['sketches'] else '-'
        lines.append(f'| {item["name"]} | {"covered" if item["covered"] else "missing"} | {evidence} |')
    for title, key in [
        ('Missing core sources', 'core'),
        ('Missing SDK driver sources', 'sdk_drivers'),
        ('Missing HAL driver sources', 'hal_drivers'),
    ]:
        if coverage[key]['missing']:
            lines.extend(['', f'## {title}', ''])
            lines.extend(f'- `{name}`' for name in coverage[key]['missing'])
    if coverage['sketches']['failed']:
        lines.extend(['', '## Failed sketches', ''])
        lines.extend(f'- `{name}`' for name in coverage['sketches']['failed'])
    (output / 'coverage.md').write_text('\n'.join(lines) + '\n')


def coverage_missing(coverage):
    return (
        any(not item['covered'] for item in coverage['libraries'])
        or bool(coverage['core']['missing'])
        or bool(coverage['sdk_drivers']['missing'])
        or bool(coverage['hal_drivers']['missing'])
    )


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', default='arduino-cli')
    parser.add_argument('--config', type=Path)
    parser.add_argument('--toolchain', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fqbn', default='Kendryte:K210:k1')
    parser.add_argument('--sketch', action='append', type=Path, help='Only these sketch directories; repeatable')
    parser.add_argument('--application', action='append', type=Path, default=[], help='Additional sketch directories')
    return parser.parse_args()


def main():
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    full_matrix = args.sketch is None
    sketches = list(args.sketch) if args.sketch else discover_sketches()
    sketches.extend(args.application)
    catalog = library_catalog()
    results = []
    library_evidence = {library: set() for library in catalog}
    observed_sources = {'core': set(), 'sdk_drivers': set(), 'hal_drivers': set()}

    for sketch in sketches:
        sketch = sketch.resolve()
        key = sketch.name + '-' + hashlib.sha256(str(sketch).encode()).hexdigest()[:8]
        log = args.output / (key + '.log')
        build_path = args.output / key
        command = [
            args.cli,
            'compile',
            '--fqbn',
            args.fqbn,
            '--jobs',
            '4',
            '--build-property',
            'runtime.tools.xpack-riscv-none-embed-gcc.path=' + str(args.toolchain.resolve()),
            '--build-path',
            str(build_path),
            str(sketch),
        ]
        if args.config:
            command += ['--config-file', str(args.config.resolve())]
        with log.open('w') as stream:
            completed = subprocess.run(command, cwd=sketch, stdout=stream, stderr=subprocess.STDOUT)

        passed = completed.returncode == 0
        used_libraries = set()
        if passed:
            used_libraries = compiled_libraries(build_path) | direct_library_uses(sketch, catalog)
            sketch_name = relative_name(sketch)
            for library in used_libraries & set(catalog):
                library_evidence[library].add(sketch_name)
            observed_sources['core'].update(object_source_names(build_path / 'core', recursive=False))
            observed_sources['sdk_drivers'].update(object_source_names(build_path / 'core/k210-sdk/drivers'))
            observed_sources['hal_drivers'].update(object_source_names(build_path / 'core/k210-hal'))

        results.append(
            {
                'sketch': str(sketch),
                'passed': passed,
                'libraries': sorted(used_libraries),
                'log': str(log),
                'command': command,
            }
        )
        print(('PASS ' if passed else 'FAIL ') + str(sketch), flush=True)
        (args.output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')

    coverage = create_coverage(full_matrix, results, library_evidence, observed_sources)
    write_coverage(args.output, coverage)
    failed = any(not result['passed'] for result in results)
    if full_matrix and coverage_missing(coverage):
        print('FAIL incomplete bundled library/driver compile coverage', flush=True)
        failed = True
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
