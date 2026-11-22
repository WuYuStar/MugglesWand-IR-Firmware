"""Build the ESP32-S3 application and first-flash images."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from platformio.project.config import ProjectConfig

SOURCE=Path(__file__).resolve().parents[1]
SKIP={'.git','.pio','.vscode','__pycache__','tools'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workdir',type=Path,required=True,help='Separate disposable firmware build directory')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();work=args.workdir.resolve();out=args.output.resolve()
    if work==SOURCE or SOURCE in work.parents or work in SOURCE.parents:
        parser.error('Use a separate build directory outside the source tree.')
    work.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    hashes={}
    for path in SOURCE.rglob('*'):
        rel=path.relative_to(SOURCE)
        if not path.is_file() or any(part in SKIP for part in rel.parts):continue
        data=path.read_bytes();dest=work/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        if not dest.exists() or dest.read_bytes()!=data:shutil.copy2(path,dest)
        hashes[rel.as_posix()]=hashlib.sha256(data).hexdigest()
    config_path = work / 'platformio.ini'
    config = ProjectConfig.get_instance(str(config_path))
    packages = Path(config.get('platformio', 'packages_dir'))
    build_root = work / '.pio' / 'build'
    build = build_root / 'esp32-s3-fh4r2'
    version=re.search(r'#define FIRMWARE_VER "([^"]+)"',(work/'src/Config.h').read_text()).group(1)
    esptool = [sys.executable, str(packages / 'tool-esptoolpy' / 'esptool.py'), '--chip', 'esp32s3']
    boot_app = packages / 'framework-arduinoespressif32' / 'tools' / 'partitions' / 'boot_app0.bin'
    merged = build / 'factory.bin'
    with (out/'build.log').open('w',encoding='utf-8') as log:
        try:
            subprocess.run([sys.executable, '-m', 'platformio', 'run', '-d', str(work)],
                           stdout=log, stderr=subprocess.STDOUT, check=True,
                           env=dict(os.environ, PLATFORMIO_BUILD_DIR=str(build_root)))
            app = build / 'firmware.bin'
            image = app.read_bytes()
            if not 24 <= len(image) <= 0x1e0000 or image[0] != 0xe9 or image[12:14] != b'\x09\0':
                raise SystemExit('Invalid ESP32-S3 application image or OTA slot overflow')
            flash_mode = config.get('env:esp32-s3-fh4r2', 'board_build.flash_mode', 'dio')
            flash_freq = config.get('env:esp32-s3-fh4r2', 'board_build.f_flash', '80000000L')
            flash_freq = f'{int(flash_freq.rstrip("L")) // 1000000}m'
            flash_size = config.get('env:esp32-s3-fh4r2', 'board_upload.flash_size', '4MB')
            subprocess.run([*esptool, 'merge_bin', '-o', str(merged),
                            '--flash_mode', flash_mode, '--flash_freq', flash_freq, '--flash_size', flash_size,
                            '0x0', str(build / 'bootloader.bin'), '0x8000', str(build / 'partitions.bin'),
                            '0xe000', str(boot_app), '0x10000', str(app)],
                           stdout=log, stderr=subprocess.STDOUT, check=True)
        except subprocess.CalledProcessError as exc:
            raise SystemExit(f'Build failed. See {out / "build.log"}') from exc
    factory = merged.read_bytes()
    if factory[0x10000:] != image:
        raise SystemExit('Factory and OTA application payloads differ')
    files = {}
    for kind, payload, offset in [('app', image, 0x10000), ('factory', factory, 0)]:
        name = f'MugglesWand-{version}-{kind}.bin'
        (out / name).write_bytes(payload)
        files[kind] = dict(file=name, offset=hex(offset), bytes=len(payload),
                           sha256=hashlib.sha256(payload).hexdigest())
    manifest=dict(version=version, chip='ESP32-S3FH4R2', files=files,
                  flash_mode=flash_mode, flash_frequency=flash_freq, model_protocol=1, source_sha256=hashes)
    (out/'SHA256SUMS.txt').write_text(''.join(f'{entry["sha256"]}  {entry["file"]}\n' for entry in files.values()),
                                    encoding='utf-8')
    (out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in manifest.items() if k!='source_sha256'},ensure_ascii=True),flush=True)

if __name__=='__main__':main()
