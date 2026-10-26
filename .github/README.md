# CI, releases, and documentation

`push.yml` runs serial-selection and offline release regression tests, and Arduino
CLI compiles of every bundled example and hardware test sketch. Firmware builds
use `tests/build_compat.py`, which also checks library and source compile coverage.
The workflow does not upload to or run tests on an attached K210. An optional Mac
step detects the configured board using USB metadata. Build logs and source
coverage reports are saved as workflow artifacts. Runtime tests run on the board
through the [hardware test suite](../tests/testcases/README.md).

| Event | Result after checks pass |
| --- | --- |
| Pull request to `main` | Tests only |
| Code push to `main` | New development prerelease and dev Board Manager index |
| Push `1.2.3` or `v1.2.3` tag | Stable release and both Board Manager indexes |
| Push `1.2.3-rc.1` or `v1.2.3-rc.1` tag | Prerelease and dev index only |
| Publish a GitHub release | Package that tag; honor its prerelease flag |
| Run **CI and releases** manually | Tests only by default; enable **Publish a release after the tests pass** to publish/retry |

Documentation-only pushes do not publish firmware. Publication is restricted to
`kendryte/arduino-k210`; forks run checks only. Tags must be semantic versions
with three numeric components and an optional prerelease suffix. The package
version omits an optional leading `v`; download URLs retain the original tag.
The workflows and tests must already exist in the tagged commit.

Development tags are `dev-<UTC commit timestamp>-<12-character SHA>`. Their Arduino
versions are `<package.json version>-dev.<timestamp>.g<SHA>`. They are immutable
snapshots, so historical Board Manager checksums and URLs remain valid. Advance
`package.json` to the next intended version when starting a new development
cycle: a prerelease sorts below the stable release with the same numeric version.

`release.yml` is a reusable publisher invoked only after checks succeed. It checks
out the exact tested commit, verifies version tags, builds a deterministic ZIP,
and includes the root license and compatibility provenance records. All package
version fields are generated together. `tools/release.py` uses Python's standard library
and checks every API request; no `distutils` or access tokens in URLs are used.

New releases start as drafts. The publisher uploads assets, publishes the
release, then atomically updates both applicable indexes on `gh-pages`, retaining
other files and all historical package/tool versions. A failed upload cannot
publish a broken Board Manager entry. Reruns reuse identical ZIP assets and keep
maintainer-written release notes. Different ZIP bytes for an existing tag fail
instead of silently invalidating installed-package checksums. Index assets on
releases are snapshots; `gh-pages` is the current cumulative index. Reruns delete
and retry unfinished `starter` assets left by interrupted GitHub uploads, while
completed assets remain protected. Every new upload must report `uploaded` and
the expected size before publication. See
[GitHub's upload failure behavior](https://docs.github.com/en/rest/releases/assets#upload-a-release-asset).

The publisher has `contents: write`; build jobs have read access. No PAT is
required. Repository rules must permit the Actions token to create releases and
update `gh-pages`. The branch is created if absent. Publishers share a concurrency
group with `queue: max`, so queued releases are retained instead of replacing one
another. See [GitHub concurrency documentation](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency).
Actionlint 1.7.12 does not yet recognize `queue`; its unknown-key diagnostic for
this documented setting is the only exception in local workflow linting.
GitHub does not retrigger release workflows for releases created with the
workflow's own `GITHUB_TOKEN`.

Existing installation URLs remain:

- Stable: `https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_index.json`
- Development: `https://raw.githubusercontent.com/kendryte/arduino-k210/gh-pages/package_k210_dev_index.json`

`docs.yml` checks the actual PR revision with Sphinx warnings treated as errors.
On `main`, it builds the versioned site and deploys with GitHub Pages. Select
**GitHub Actions** as the repository's Pages source. The existing `/main/` docs URL
and branch-based Board Manager URLs remain separate. Dependencies are pinned and
Dependabot updates action SHAs weekly and documentation/test requirements monthly.

## Self-hosted macOS tests

The Python tests and firmware builds use `[self-hosted, macOS, ARM64, shenzhen_mac]`
for pushes and manual runs on `kendryte/arduino-k210`'s `main` branch, matching the
[K230 Burning Tool runner](https://github.com/kendryte/k230_burning_tool/blob/main/.github/workflows/build.yml).
All PRs (including same-repository PRs), tags, published releases, forks, and runs
on other branches use hosted Ubuntu. This lets tag checks finish with the Mac
restricted to the workflow on `main`. Neither `pull_request_target` nor
`workflow_run` is enabled. Publication and documentation deployment also use
Ubuntu. With one Mac runner, the two test jobs run sequentially.

These routing rules govern the checked-in workflows. They are not an access
control boundary against a PR that edits workflow YAML. For enforcement, the
organization must restrict the Mac runner group's **Workflow access** to
**Selected workflows**, with this exact entry for this repository:

```text
kendryte/arduino-k210/.github/workflows/push.yml@refs/heads/main
```

Use **Selected repositories** to grant `kendryte/arduino-k210` access to the group.
Retain any separately reviewed workflow entries needed by other repositories
sharing the Mac. Do not grant all workflows in this public repository access or
allow PR refs. GitHub documents this under
[runner-group workflow restrictions](https://docs.github.com/en/enterprise-cloud%40latest/actions/how-tos/manage-runners/self-hosted-runners/manage-access).
This organization setting must be configured separately. If workflow restrictions
are unavailable, isolate the runner in a private CI repository to enforce the
no-PR boundary.

Before running the workflow:

1. Confirm that the Mac is online and visible to **arduino-k210** under
   **Settings → Actions → Runners**, with all four labels. A runner registered
   only to `k230_burning_tool` cannot accept this repository's jobs: share an
   organization runner group with `arduino-k210`, or register a separate runner
   instance in an organization runner group with the workflow restriction above.
2. Keep the Actions runner current for the Node 24 actions (runner 2.327.1 or newer).
   Ensure Homebrew and Xcode Command Line Tools are available to the runner's
   service account. `xcode-select --install` installs the command line tools if
   they are missing.
3. Install Rosetta once on the Apple Silicon Mac, if needed:

   ```sh
   softwareupdate --install-rosetta
   ```

The pinned K210 cross-compiler has only an Intel macOS build, which runs through
[Rosetta](https://support.apple.com/en-us/102527). Setup checks Rosetta before
downloading the compiler and verifies the archive checksum.

The shared setup action uses Homebrew `python@3.12` in a virtual environment on
the Mac. Ubuntu jobs use `setup-python` with Python 3.14.

### Board detection

Set `K210_PORT_SERIAL_NUMBER` under **Settings → Secrets and variables → Actions**
to enable detection. Repository variables and secrets are both supported; secrets
take precedence. VID and PID already default to the Canaan K1's CH342 adapter:

| Setting | Value |
| --- | --- |
| `K210_PORT_SERIAL_NUMBER` | The board's USB serial number from `--list` (required) |
| `K210_PORT_VID` | `0x1A86` or `6790` (this CH342 value is the default) |
| `K210_PORT_PID` | `0x55D2` or `21970` (this CH342 value is the default) |

The **Detect configured Canaan K1 on macOS** step resolves UART B by USB interface
2, using `ioreg` to supplement pySerial metadata. It reads USB metadata without
opening, resetting, or flashing the board. With no serial number configured, the
step is skipped. An absent or ambiguous configured board fails the checks and
blocks publication for that run. Tags and PRs do not perform board detection.

Leave `K210_PORT`, `K210_PORT_LOCATION`, and `K210_PORT_INTERFACE` unset for
automatic selection. They are optional overrides for drivers that cannot report
the interface number, and still enforce the configured USB identity. The local
pytest hardware runner supports the same identity filters and infers the board
profile from the compiled sketch's FQBN. See
[hardware port selection](../tests/testcases/README.md#selecting-the-hardware-test-port).
The serial regression tests use simulated port lists and a mocked flasher.

Once these workflows are on `main`, open **Actions → CI and releases → Run
workflow**, select `main`, and leave publishing unchecked to test the Mac setup.
Enable publishing on `main` or a version tag to retry a release. Main pushes and
version tags publish automatically after successful tests.

## Local validation

```sh
python3 -m unittest discover -s tests/release -v
python3 -m pip install -r tests/requirements.txt
python3 -m pytest -c tests/serial/pytest.ini tests/serial -v
# Build a dev ZIP and index locally; no network writes:
python3 tools/release.py --output /tmp/k210-release
# Linux or macOS, with Arduino CLI 1.5.1 on PATH:
python3 tools/ci_prepare.py --directory /tmp/k210-ci
python3 tests/build_compat.py --config /tmp/k210-ci/arduino-cli.yaml \
  --toolchain /tmp/k210-ci/tools/xpack-riscv-none-embed-gcc-10.2.0-1.2 \
  --output /tmp/k210-builds
```

The build scripts need Python 3.11 or newer (macOS CI uses 3.12, Linux uses 3.14).
Documentation uses Python 3.12. `tools/on-release.sh` is the publishing entry point;
it requires an Actions event and `GH_TOKEN` (or `GITHUB_TOKEN`). Use `tools/release.py` without
`--publish` for local previews. Actual publishing is intentionally not part of
local tests.
