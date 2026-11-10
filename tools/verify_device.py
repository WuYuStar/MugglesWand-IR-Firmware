"""Read device health; optionally exercise sampling/back, without flashing or saving settings."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import time

class Device:
    def __init__(self,host,port=80):
        self.socket=socket.create_connection((host,port),timeout=5);self.buffer=b''
        try:self._handshake(host)
        except Exception:self.close();raise
    def _handshake(self,host):
        key=base64.b64encode(os.urandom(16)).decode()
        self.socket.sendall(('GET /ws HTTP/1.1\r\nHost: '+host+'\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: '+key+'\r\nSec-WebSocket-Version: 13\r\n\r\n').encode())
        while b'\r\n\r\n' not in self.buffer:
            chunk=self.socket.recv(4096)
            if not chunk:raise EOFError('Incomplete WebSocket handshake')
            self.buffer+=chunk
            if len(self.buffer)>8192:raise OSError('Oversized response header')
        header,self.buffer=self.buffer.split(b'\r\n\r\n',1)
        accept=base64.b64encode(hashlib.sha1((key+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest())
        if b' 101 ' not in header or accept not in header:raise OSError('Not a wand WebSocket')
    def read(self,n):
        while len(self.buffer)<n:
            chunk=self.socket.recv(4096)
            if not chunk:raise EOFError('Device disconnected')
            self.buffer+=chunk
        data,self.buffer=self.buffer[:n],self.buffer[n:];return data
    def send(self,data,opcode=1):
        data=data.encode() if isinstance(data,str) else data
        mask=os.urandom(4);n=len(data)
        header=bytes([0x80|opcode,0x80|n]) if n<126 else bytes([0x80|opcode,0xfe])+struct.pack('!H',n)
        self.socket.sendall(header+mask+bytes(c^mask[i%4] for i,c in enumerate(data)))
    def status(self):
        self.send('web:status');message=b''
        for _ in range(500):
            a,b=self.read(2);size=b&127
            if size==126:size=struct.unpack('!H',self.read(2))[0]
            elif size==127:size=struct.unpack('!Q',self.read(8))[0]
            if size>65536:raise OSError('Oversized device frame')
            data=self.read(size);opcode=a&15
            if opcode==8:raise EOFError('Device closed WebSocket')
            if opcode==9:self.send(data,10);continue
            if opcode not in (0,1):continue
            message=message+data if opcode==0 else data
            if not a&0x80:continue
            try:value=json.loads(message)
            except ValueError:continue
            if isinstance(value,dict) and value.get('type')=='status':return value
        raise TimeoutError('No device status')
    def close(self):self.socket.close()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',default='192.168.4.1');parser.add_argument('--version',default='6.1-rc1')
    parser.add_argument('--seconds',type=int,default=120);parser.add_argument('--cycles',type=int,default=0)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if args.seconds<1 or not 0<=args.cycles<=100:parser.error('Use seconds>=1 and cycles0..100')
    device=Device(args.host);samples=0;baseline=device.status()
    if baseline.get('version')!=args.version:device.close();raise SystemExit('Unexpected firmware version; no mode commands sent')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(type='baseline',status={k:v for k,v in baseline.items() if k!='password'}),ensure_ascii=False)+'\n',encoding='utf-8')
    def record():
        nonlocal samples
        status=device.status();samples+=1
        # JSON lines keep long soak logs bounded in memory and avoid rewrites.
        with args.output.open('a',encoding='utf-8') as log:
            log.write(json.dumps({k:v for k,v in status.items() if k!='password'},ensure_ascii=False)+'\n')
        for field in ('bindings','gestureEffects','gestureLabels'):
            if status.get(field)!=baseline.get(field):raise RuntimeError('Configuration changed: '+field)
        if status.get('version')!=args.version:raise RuntimeError('Firmware changed during verification')
        return status
    try:
        # Explicit opt-in: user holds the wand still during these mode cycles.
        for _ in range(args.cycles):
            device.send('imu');time.sleep(.2)
            if record()['mode']!=1:raise RuntimeError('Cannot enter sampling')
            device.send('back');deadline=time.monotonic()+5
            while True:
                s=record()
                if s.get('mode')==2 and s.get('imuReady') and s.get('inferenceReady'):break
                if time.monotonic()>deadline:raise RuntimeError('Cannot restore recognition')
                time.sleep(.2)
        deadline=time.monotonic()+args.seconds;previous=None
        while time.monotonic()<deadline:
            s=record()
            if s.get('mode')!=2 or not s.get('imuReady') or not s.get('inferenceReady'):raise RuntimeError('Recognition stream not ready')
            if previous and s.get('sampleCount',0)<=previous.get('sampleCount',0):raise RuntimeError('Sampling stopped or device restarted')
            previous=s;time.sleep(1)
        print(json.dumps(dict(health_passed=True,samples=samples,cycles=args.cycles,hardware_gesture_accuracy_tested=False),ensure_ascii=True))
    finally:
        if args.cycles:
            try:device.send('back')
            except OSError:pass
        device.close()

if __name__=='__main__':main()
