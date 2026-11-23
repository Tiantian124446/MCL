import sys, struct, time
sys.path.insert(0, r'E:\MCL\tools')
from vnc import VNC
from PIL import Image

host = '127.0.0.1'
port = 5907
out = sys.argv[1] if len(sys.argv) > 1 else r'E:\MCL\dist\vm_vnc.png'

v = VNC(host, port)
v.s.settimeout(40)
# SetPixelFormat: 32bpp, depth 24, little-endian, true colour, RGB max/shifts
v.s.sendall(b'\x00\x00\x00\x00' + bytes([32, 24, 0, 1]) + struct.pack('>HHH', 255, 255, 255) + bytes([16, 8, 0, 0, 0, 0]))
# SetEncodings: raw
v.s.sendall(b'\x02\x00' + struct.pack('>H', 1) + struct.pack('>i', 0))
# FramebufferUpdateRequest
v.s.sendall(b'\x03\x00' + struct.pack('>HHHH', 0, 0, v.width, v.height))

msg = v.recvall(1)[0]
if msg != 0:
    raise RuntimeError('unexpected VNC message %d' % msg)
v.recvall(1)
nrects = struct.unpack('>H', v.recvall(2))[0]
img = None
for _ in range(nrects):
    x, y, w, h, enc = struct.unpack('>HHHHi', v.recvall(12))
    if enc != 0:
        raise RuntimeError('unsupported encoding %d' % enc)
    data = v.recvall(w * h * 4)
    part = Image.frombytes('RGB', (w, h), data, 'raw', 'BGRX')
    if img is None:
        img = Image.new('RGB', (v.width, v.height))
    img.paste(part, (x, y))
img.save(out)
print('saved', out, img.size)
