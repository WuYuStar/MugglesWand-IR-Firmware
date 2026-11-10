"""Build the generic 39-LED firmware separately from the gesture-training app."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

SOURCE=Path(__file__).resolve().parents[1]
SKIP={'.git','.pio','.vscode','__pycache__','tools'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workdir',type=Path,required=True,help='Separate disposable firmware build directory')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();work=args.workdir.resolve();out=args.output.resolve()
    if work==SOURCE or SOURCE in work.parents:parser.error('Use a build directory outside the source tree.')
    work.mkdir(parents=True,exist_ok=True);out.mkdir(parents=True,exist_ok=True)
    hashes={}
    for path in SOURCE.rglob('*'):
        rel=path.relative_to(SOURCE)
        if not path.is_file() or any(part in SKIP for part in rel.parts):continue
        data=path.read_bytes();dest=work/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        if not dest.exists() or dest.read_bytes()!=data:shutil.copy2(path,dest)
        hashes[rel.as_posix()]=hashlib.sha256(data).hexdigest()
    version=re.search(r'#define FIRMWARE_VER "([^"]+)"',(work/'src/Config.h').read_text()).group(1)
    with (out/'build.log').open('w',encoding='utf-8') as log:
        p=subprocess.run([sys.executable,'-m','platformio','run','-d',str(work)],stdout=log,stderr=subprocess.STDOUT,env=dict(os.environ,PIP_INDEX_URL='https://pypi.org/simple'))
    if p.returncode:raise SystemExit('Build failed; see '+str(out/'build.log'))
    image=(work/'.pio/build/esp32-s3-fh4r2/firmware.bin').read_bytes()
    if not 24<=len(image)<=0x1e0000 or image[0]!=0xe9 or image[12:14]!=b'\x09\0':raise SystemExit('Invalid S3 OTA application image/size')
    name=f'MugglesWand-{version}-generic-39led-app.bin';(out/name).write_bytes(image)
    manifest=dict(version=version,status='candidate-until-hardware-acceptance',file=name,bytes=len(image),sha256=hashlib.sha256(image).hexdigest(),chip='ESP32-S3FH4R2',flash_bytes=4194304,quad_psram_bytes=2097152,led_count=39,model='original-project-builtin',model_protocol=1,source_sha256=hashes)
    (out/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in manifest.items() if k!='source_sha256'},ensure_ascii=True),flush=True)

if __name__=='__main__':main()
