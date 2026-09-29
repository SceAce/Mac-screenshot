#!/usr/bin/env python3
"""Deploy the built Qt application, verify its dependencies, and sign a local .app."""

import argparse
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
MACHO_MAGICS = {b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe", b"\xfe\xed\xfa\xcf",
                b"\xfe\xed\xfa\xce", b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"}


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True)


def find_deployer():
    for name in ("macdeployqt", "macdeployqt6"):
        if path := shutil.which(name):
            return path
    brew = shutil.which("brew")
    if brew:
        for formula in ("qtbase", "qt"):
            result = subprocess.run([brew, "--prefix", formula], capture_output=True, text=True)
            candidate = Path(result.stdout.strip()) / "bin/macdeployqt"
            if result.returncode == 0 and candidate.is_file():
                return str(candidate)
    raise RuntimeError("macdeployqt was not found. Install Qt 6 and add its bin directory to PATH.")


def macho_files(bundle):
    for binary in bundle.rglob("*"):
        if not binary.is_file() or binary.is_symlink():
            continue
        with binary.open("rb") as stream:
            if stream.read(4) not in MACHO_MAGICS:
                continue
        yield binary


def load_commands(binary):
    listing = subprocess.check_output(["/usr/bin/otool", "-l", str(binary)], text=True)
    for block in re.split(r"Load command \d+\n", listing)[1:]:
        command = re.search(r"^\s*cmd (LC_\w+)\s*$", block, re.MULTILINE)
        value = re.search(r"^\s*(?:name|path) (.+) \(offset \d+\)", block, re.MULTILINE)
        if command and value:
            yield command.group(1), value.group(1)


def verify_and_sign(bundle, identity):
    binaries = list(macho_files(bundle))
    for binary in binaries:
        commands = list(load_commands(binary))
        for command, dependency in commands:
            if command == "LC_RPATH" and dependency.startswith("/"):
                run("/usr/bin/install_name_tool", "-delete_rpath", dependency, binary)
            # LC_ID_DYLIB describes a library's own install name, not a dependency.
            if command not in {"LC_LOAD_DYLIB", "LC_LOAD_WEAK_DYLIB", "LC_REEXPORT_DYLIB", "LC_LOAD_UPWARD_DYLIB"}:
                continue
            candidate = None
            if dependency.startswith("@rpath/"):
                candidate = bundle / "Contents/Frameworks" / dependency.removeprefix("@rpath/")
            elif dependency.startswith("@loader_path/"):
                candidate = binary.parent / dependency.removeprefix("@loader_path/")
            elif dependency.startswith("@executable_path/"):
                candidate = bundle / "Contents/MacOS" / dependency.removeprefix("@executable_path/")
            elif dependency.startswith(("/System/Library/", "/usr/lib/")):
                continue
            if candidate is None or not candidate.exists():
                raise RuntimeError(f"Unbundled dependency in {binary.relative_to(bundle)}: {dependency}")
    print(f"Verified {len(binaries)} Mach-O files: all non-system dependencies are bundled.", flush=True)
    # Sign from the inside out after install-name/rpath changes. This also covers
    # third-party libraries that macdeployqt may copy with an old vendor signature.
    for binary in binaries:
        run("/usr/bin/codesign", "--force", "--sign", identity, "--timestamp=none", binary)
    for framework in sorted(bundle.rglob("*.framework"), key=lambda path: len(path.parts), reverse=True):
        run("/usr/bin/codesign", "--force", "--sign", identity, "--timestamp=none", framework)
    run("/usr/bin/codesign", "--force", "--sign", identity, "--timestamp=none", bundle)
    run("/usr/bin/codesign", "--verify", "--deep", "--strict", bundle)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-macos")
    parser.add_argument("--output", type=Path, default=ROOT / "dist/Mark Shot.app")
    parser.add_argument("--identity", default="-", help="Signing identity; '-' creates a local ad-hoc signature")
    parser.add_argument("--with-provider-plugins", action="store_true",
                        help="Also bundle optional OCR/translation/scanning provider plugins")
    parser.add_argument(
        "--without-provider-plugins",
        dest="with_provider_plugins",
        action="store_false",
        help="Skip building and bundling optional OCR and translation provider plugins.",
    )
    parser.set_defaults(with_provider_plugins=True)
    args = parser.parse_args()
    if sys.platform != "darwin":
        parser.error("This packaging command requires macOS.")
    build = args.build_dir.resolve()
    output = args.output.absolute()
    if output.suffix != ".app":
        parser.error("--output must end in .app")
    deployer = find_deployer()
    library_paths = []
    brew_library_path = None
    if brew := shutil.which("brew"):
        prefix = subprocess.check_output([brew, "--prefix"], text=True).strip()
        brew_library_path = f"{prefix}/lib"
        library_paths.append(f"-libpath={brew_library_path}")
    run("cmake", "--build", build, "--parallel", min(os.cpu_count() or 2, 8))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".mark-shot-package-", dir=output.parent) as temporary:
        stage = Path(temporary)
        run("cmake", "--install", build, "--prefix", stage / "install")
        bundle = stage / "Mark Shot.app"
        shutil.copytree(stage / "install/mark-shot.app", bundle, symlinks=True)
        plugins = bundle / "Contents/PlugIns/markshot"
        plugins.mkdir(parents=True, exist_ok=True)
        if args.with_provider_plugins:
            for library in sorted((stage / "install/lib/mark-shot/plugins").glob("*.dylib")):
                shutil.copy2(library, plugins / library.name)
        qtpaths = Path(deployer).resolve().with_name("qtpaths")
        qt_plugins = Path(subprocess.check_output([str(qtpaths), "--query", "QT_INSTALL_PLUGINS"], text=True).strip())
        # Deploy the native Widgets runtime and common image formats explicitly.
        # An unrestricted Homebrew scan also imports unrelated QML virtual-keyboard
        # and PDF plugins whose framework rpaths point at a different Qt formula.
        qt_plugin_names = {
            "platforms": ("qcocoa", "qoffscreen"),
            "styles": ("qmacstyle",),
            "imageformats": ("qjpeg", "qgif", "qico", "qicns", "qsvg", "qtiff", "qwebp", "qmacheif"),
            "tls": ("qsecuretransportbackend", "qcertonlybackend"),
        }
        for category, names in qt_plugin_names.items():
            destination = bundle / "Contents/PlugIns" / category
            destination.mkdir(parents=True, exist_ok=True)
            for name in names:
                library = qt_plugins / category / f"lib{name}.dylib"
                if library.exists():
                    shutil.copy2(library, destination / library.name)
                elif name == "qcocoa":
                    raise RuntimeError("Qt's native Cocoa platform plugin was not found")
        if brew_library_path:
            # Each plugin needs its own search path while macdeployqt resolves
            # split Homebrew Qt formulas. External paths are removed before signing.
            for binary in macho_files(bundle):
                if ("LC_RPATH", brew_library_path) not in set(load_commands(binary)):
                    run("/usr/bin/install_name_tool", "-add_rpath", brew_library_path, binary)
        resources = bundle / "Contents/Resources"
        shutil.copy2(ROOT / "LICENSE", resources / "LICENSE.txt")
        print("Deploying Qt frameworks and provider libraries...", flush=True)
        run(deployer, bundle, "-always-overwrite", "-no-strip", "-no-codesign", "-no-plugins", *library_paths,
            *(f"-executable={library}" for library in sorted((bundle / "Contents/PlugIns").rglob("*.dylib"))))
        verify_and_sign(bundle, args.identity)
        with (bundle / "Contents/Info.plist").open("rb") as stream:
            identifier = plistlib.load(stream)["CFBundleIdentifier"]
        if output.exists():
            # Replace only a previous build of the same application.
            if output.is_symlink():
                raise RuntimeError(f"Refusing to replace a symlink: {output}")
            with (output / "Contents/Info.plist").open("rb") as stream:
                previous_identifier = plistlib.load(stream).get("CFBundleIdentifier")
            if previous_identifier != identifier:
                raise RuntimeError(f"Refusing to replace a different application: {output}")
            output.rename(stage / "previous.app")
        bundle.rename(output)
    print(f"Ready: {output}")
    print("Open this bundle to authorize Screen Recording and capture. Keep its location stable after authorization.")


if __name__ == "__main__":
    main()
