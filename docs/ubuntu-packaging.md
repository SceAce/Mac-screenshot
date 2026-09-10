# Ubuntu source packaging and community PPA maintenance

This repository contains conventional Debian source packaging for **Ubuntu
26.04 LTS (Resolute) only**. Any PPA created from these files is a community
service: it is **unofficial, unendorsed, and not an upstream release channel**
unless the Mark Shot maintainer explicitly adopts it.

## Package scope

The package uses source format `3.0 (quilt)`, debhelper compat 13, CMake, and
Ninja. Launchpad receives source uploads and builds the binaries without
network access. The orig tarball must come from an official stable upstream
tag; drafts and prereleases are not eligible.

Resolute provides the dependencies used by the package:

- Qt 6.10, Qt Wayland, and the Qt SVG image plugins;
- FFmpeg 8 development libraries, including `libavfilter`;
- PipeWire, PulseAudio, Wayland, EGL/GLES, X11/XCB, and libportal;
- LayerShellQt (`liblayershellqtinterface-dev`);
- ONNX Runtime and protobuf for the native RapidOCR provider plugin;
- zxing-cpp 2.3 for the native code-scanning provider plugin.

`python3-rapidocr` is not available in Resolute. The native ONNX Runtime plugin
is built, while the legacy Python helper can use Tesseract from the archive or
a user-managed RapidOCR virtual environment. `tesseract-ocr-eng` and
`tesseract-ocr-spa` are available and remain optional. Runtime helper programs
such as `grim`, `wl-clipboard`, and `xclip` are recommendations rather than
build dependencies.

## Build the source package

Start from a checkout whose application source matches the release tag. The
example below adds only `debian/` to the official tag archive:

```bash
version=0.1.49
work="$(mktemp -d)"
git archive --format=tar.gz \
  --prefix="mark-shot-$version/" \
  --output="../mark-shot_$version.orig.tar.gz" \
  "v$version"
mkdir "$work/mark-shot-$version"
tar -xzf "../mark-shot_$version.orig.tar.gz" \
  --strip-components=1 -C "$work/mark-shot-$version"
cp -a debian "$work/mark-shot-$version/"
(cd "$work/mark-shot-$version" && dpkg-buildpackage -S -us -uc)
```

For a local binary build on Resolute:

```bash
sudo apt update
sudo apt install build-essential devscripts equivs lintian
sudo mk-build-deps --install --remove \
  --tool 'apt-get --no-install-recommends -y' debian/control
dpkg-buildpackage -b -us -uc
lintian --display-info --pedantic ../mark-shot_*.changes
```

The CI workflow `.github/workflows/debian-source.yml` performs the source
build, unpacks the generated `.dsc`, builds the binary in an Ubuntu 26.04
container, runs Lintian, installs the package with APT, runs `apt-get check`,
and executes the smoke test.

## Clean Resolute build with sbuild

A local `sbuild` chroot provides the final clean-builder check. One common
Ubuntu setup is:

```bash
sudo apt install sbuild schroot debootstrap ubuntu-dev-tools
sudo sbuild-adduser "$USER"
newgrp sbuild
mk-sbuild resolute
sbuild --dist=resolute --arch=amd64 ../mark-shot_*.dsc
lintian --display-info --pedantic /var/lib/sbuild/build/mark-shot_*.changes
```

Review the mirror and chroot path printed by `mk-sbuild`; local site policy may
place results elsewhere. Do not add undeclared packages to the chroot.

## Inspect and install a build

```bash
dpkg-deb --info ../mark-shot_*_amd64.deb
dpkg-deb --contents ../mark-shot_*_amd64.deb
lintian --display-info --pedantic ../mark-shot_*_amd64.changes
sudo apt install ../mark-shot_*_amd64.deb
sudo apt-get check
QT_QPA_PLATFORM=offscreen /usr/bin/mark-shot --version
sudo debian/tests/smoke
```

Also inspect ownership, modes, generated dependencies, and maintainer scripts:

```bash
dpkg-query -L mark-shot
find /usr/bin /usr/lib/mark-shot /usr/share/applications \
  -xdev -user root -group root -ls
dpkg-query -W -f='${Depends}\n${Recommends}\n${Suggests}\n' mark-shot
ls -l /var/lib/dpkg/info/mark-shot.*
dpkg-trigger --check-supported
```

The package deliberately installs private plugins under `/usr/lib/mark-shot`,
which is a runtime search path used by Mark Shot. It installs the GNOME helper
extension under `/usr/share/gnome-shell/extensions`; enabling it remains an
explicit per-user action.

## Create an unofficial Launchpad channel

1. Create a Launchpad team whose display name and summary clearly contain
   “Unofficial Mark Shot community packaging”. Do not use names implying
   endorsement.
2. Create a stable PPA owned by that team and repeat the unofficial status in
   its description.
3. Generate a dedicated upload-only OpenPGP key. Publish its public key to
   Launchpad and wait for Launchpad to confirm it. Never reuse a personal key.
4. Add a GitHub environment named `ppa-production` and require manual approval
   while the channel is being validated.
5. Configure the repository variable `PPA_TARGET` as
   `ppa:<team>/<archive-name>`.
6. Configure `LAUNCHPAD_GPG_PRIVATE_KEY` and, when applicable,
   `LAUNCHPAD_GPG_PASSPHRASE` as GitHub Actions secrets.

The target is intentionally not hard-coded. The manual workflow
`.github/workflows/ppa-publish.yml` verifies the requested tag against the
canonical GitHub release API, rejects drafts and prereleases, creates revision
`<version>-0ppa<N>~resolute1`, signs the source upload, verifies its signatures,
and runs `dput`. Launchpad—not GitHub Actions—builds the binary.

Before the first upload, download the workflow's signed-source artifact and
review its `.dsc`, `.changes`, `debian.tar.*`, and `orig.tar.*`. Keep public
Launchpad build logs as provenance evidence.

## Install, upgrade, roll back, and remove

After Launchpad has published a successful build:

```bash
sudo add-apt-repository ppa:<team>/<archive-name>
sudo apt update
apt-cache policy mark-shot
sudo apt install mark-shot
QT_QPA_PLATFORM=offscreen mark-shot --version
```

Keep at least one prior PPA revision published during validation. Test an
upgrade from that revision, then roll back using the exact version listed by
APT:

```bash
apt-cache madison mark-shot
sudo apt install mark-shot='<new-version>'
sudo apt-get check
sudo apt install mark-shot='<old-version>' --allow-downgrades
sudo apt-get check
sudo apt install mark-shot='<new-version>'
```

Test removal without deleting user settings, followed by complete package
removal:

```bash
sudo apt remove mark-shot
sudo apt install mark-shot
sudo apt purge mark-shot
sudo apt autoremove
```

User files below `~/.config/mark-shot` and `~/.local/share/mark-shot` are not
owned by the Debian package and are intentionally not deleted by purge.

## Desktop acceptance checklist

On the actual GNOME 50.1 Wayland/NVIDIA workstation, record results for:

- installation through APT and desktop/tray/icon integration;
- both 4K displays and region, window, and full-display capture;
- clipboard image persistence, pinned windows, and scrolling capture;
- OCR with Spanish (`spa`) and English (`eng`), plus provider diagnostics;
- QR/barcode scanning and OpenAI-compatible translation configuration;
- GIF, MP4, and MKV recording with audio where applicable;
- upgrade, rollback, reinstall, remove, and purge.

Repeat core capture and clipboard checks in an X11 session. Do not call the PPA
production-ready until this matrix and the clean-builder checks pass. These
tests must not change NVIDIA, CUDA, Docker, Incus, libvirt, or VFIO host
configuration.

## Upstream handoff

Keep packaging, verification, publication, and documentation in focused
commits. If upstream adopts the channel, add its maintainer as a Launchpad team
administrator and let upstream configure new dedicated signing credentials. If
upstream chooses a separate official PPA, document migration and deprecate the
community channel only after the replacement is verified. If upstream declines
an in-tree `debian/` directory, move the same packaging to a dedicated
packaging repository rather than discarding it.
