#!/usr/bin/env python3
"""Minimal VNC (RFB 3.8) input client for VMware Workstation remote display.
Only needs to send key/pointer events; no framebuffer decoding.
"""
import socket, struct, sys, time, argparse

SPECIAL = {
    'enter': 0xFF0D, 'return': 0xFF0D, 'esc': 0xFF1B, 'escape': 0xFF1B,
    'tab': 0xFF09, 'space': 0x20, 'backspace': 0xFF08, 'bs': 0xFF08,
    'delete': 0xFFFF, 'del': 0xFFFF, 'insert': 0xFF63, 'home': 0xFF50,
    'end': 0xFF57, 'pageup': 0xFF55, 'pagedown': 0xFF56,
    'left': 0xFF51, 'up': 0xFF52, 'right': 0xFF53, 'down': 0xFF54,
    'ctrl': 0xFFE3, 'control': 0xFFE3, 'alt': 0xFFE9, 'shift': 0xFFE1,
    'win': 0xFFEB, 'super': 0xFFEB, 'meta': 0xFFEB,
    'capslock': 0xFFE5, 'printscreen': 0xFF61, 'pause': 0xFF13,
}
for i in range(1, 13):
    SPECIAL['f%d' % i] = 0xFFBE + i - 1

SHIFT_SYMBOLS = {
    '!': '1', '@': '2', '#': '3', '$': '4', '%': '5', '^': '6', '&': '7', '*': '8',
    '(': '9', ')': '0', '_': '-', '+': '=', '{': '[', '}': ']', '|': '\\',
    ':': ';', '"': "'", '<': ',', '>': '.', '?': '/', '~': '`',
}

class VNC:
    def __init__(self, host='127.0.0.1', port=5907, password=None, timeout=10):
        self.s = socket.create_connection((host, port), timeout=timeout)
        self.s.settimeout(timeout)
        self.password = password
        self._handshake()

    def recvall(self, n):
        buf = b''
        while len(buf) < n:
            chunk = self.s.recv(n - len(buf))
            if not chunk:
                raise ConnectionError('VNC connection closed')
            buf += chunk
        return buf

    def _handshake(self):
        banner = self.recvall(12)
        if not banner.startswith(b'RFB '):
            raise RuntimeError('not a VNC server: %r' % banner)
        self.s.sendall(b'RFB 003.008\n')
        n = self.recvall(1)[0]
        if n == 0:
            raise RuntimeError('VNC server refused: ' + self.recvall(0).decode('utf-8', 'replace'))
        types = list(self.recvall(n))
        if 1 in types:
            self.s.sendall(b'\x01')
        elif 2 in types and self.password is not None:
            self.s.sendall(b'\x02')
            challenge = self.recvall(16)
            self.s.sendall(des_encrypt(self.password, challenge))
        else:
            raise RuntimeError('unsupported VNC security types: %r (password needed=%s)' % (types, self.password is not None))
        result = struct.unpack('>I', self.recvall(4))[0]
        if result != 0:
            raise RuntimeError('VNC authentication failed')
        self.s.sendall(b'\x01')  # shared
        w, h = struct.unpack('>HH', self.recvall(4))
        self.recvall(16)
        name_len = struct.unpack('>I', self.recvall(4))[0]
        self.name = self.recvall(name_len).decode('utf-8', 'replace')
        self.width, self.height = w, h

    def key(self, keysym, down):
        self.s.sendall(struct.pack('>BBHI', 4, 1 if down else 0, 0, keysym))

    def tap(self, keysym):
        self.key(keysym, True)
        time.sleep(0.02)
        self.key(keysym, False)

    def chord(self, names):
        syms = [SPECIAL.get(n.lower(), ord(n) if len(n) == 1 else None) for n in names]
        if any(s is None for s in syms):
            raise ValueError('bad chord: %r' % (names,))
        for s in syms:
            self.key(s, True)
        time.sleep(0.03)
        for s in reversed(syms):
            self.key(s, False)

    def type_text(self, text, delay=0.012):
        for ch in text:
            if ch == '\n':
                self.tap(SPECIAL['enter'])
            elif ch == '\t':
                self.tap(SPECIAL['tab'])
            elif ch in SHIFT_SYMBOLS:
                base = SHIFT_SYMBOLS[ch]
                self.key(SPECIAL['shift'], True)
                self.tap(ord(base))
                self.key(SPECIAL['shift'], False)
            elif 'A' <= ch <= 'Z':
                self.key(SPECIAL['shift'], True)
                self.tap(ord(ch.lower()))
                self.key(SPECIAL['shift'], False)
            elif 0x20 <= ord(ch) <= 0x7E:
                self.tap(ord(ch))
            else:
                raise ValueError('unsupported character: %r' % ch)
            time.sleep(delay)

    def paste(self, text):
        data = text.encode('latin-1', 'replace')
        self.s.sendall(struct.pack('>B3xI', 6, len(data)) + data)

    def click(self, x, y, button=1, double=False):
        mask = {1: 1, 2: 4, 3: 2}.get(button, 1)
        self.s.sendall(struct.pack('>BBHH', 5, 0, x, y))
        time.sleep(0.05)
        for _ in range(2 if double else 1):
            self.s.sendall(struct.pack('>BBHH', 5, mask, x, y))
            time.sleep(0.04)
            self.s.sendall(struct.pack('>BBHH', 5, 0, x, y))
            time.sleep(0.08)

    def close(self):
        try: self.s.close()
        except Exception: pass

def des_encrypt(password, challenge):
    # Minimal DES (ECB) for VNC auth; password truncated/padded to 8 bytes.
    key = (password.encode('latin-1') + b'\x00' * 8)[:8]
    # Use pycryptodome if available, else pure-python DES.
    try:
        from Crypto.Cipher import DES
        return DES.new(key, DES.MODE_ECB).encrypt(challenge[:8]) + DES.new(key, DES.MODE_ECB).encrypt(challenge[8:])
    except ImportError:
        pass
    return _des_encrypt_pure(key, challenge)

def _des_encrypt_pure(key, data):
    # Very small DES implementation adapted for VNC auth (ECB, no padding).
    PC1 = [57,49,41,33,25,17,9,1,58,50,42,34,26,18,10,2,59,51,43,35,27,19,11,3,60,52,44,36,
           63,55,47,39,31,23,15,7,62,54,46,38,30,22,14,6,61,53,45,37,29,21,13,5,28,20,12,4]
    PC2 = [14,17,11,24,1,5,3,28,15,6,21,10,23,19,12,4,26,8,16,7,27,20,13,2,
           41,52,31,37,47,55,30,40,51,45,33,48,44,49,39,56,34,53,46,42,50,36,29,32]
    IP = [58,50,42,34,26,18,10,2,60,52,44,36,28,20,12,4,62,54,46,38,30,22,14,6,64,56,48,40,32,24,16,8,
          57,49,41,33,25,17,9,1,59,51,43,35,27,19,11,3,61,53,45,37,29,21,13,5,63,55,47,39,31,23,15,7]
    FP = [40,8,48,16,56,24,64,32,39,7,47,15,55,23,63,31,38,6,46,14,54,22,62,30,37,5,45,13,53,21,61,29,
          36,4,44,12,52,20,60,28,35,3,43,11,51,19,59,27,34,2,42,10,50,18,58,26,33,1,41,9,49,17,57,25]
    E = [32,1,2,3,4,5,4,5,6,7,8,9,8,9,10,11,12,13,12,13,14,15,16,17,
         16,17,18,19,20,21,20,21,22,23,24,25,24,25,26,27,28,29,28,29,30,31,32,1]
    P = [16,7,20,21,29,12,28,17,1,15,23,26,5,18,31,10,2,8,24,14,32,27,3,9,19,13,30,6,22,11,4,25]
    S = [
      [14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8],
      [4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13],
      [15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13,0,4,7,11,10,2,13,1,8,12,5,3,15,9,6,14],
      [1,13,2,15,11,8,4,14,0,12,6,3,9,5,7,10,15,3,1,8,14,12,5,6,11,0,13,9,2,7,10,4],
      [6,11,3,7,0,13,5,10,14,9,2,12,4,15,8,1,10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8],
      [13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2],
      [7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11],
      [4,13,1,6,2,15,11,8,10,7,4,10,9,0,8,6,3,15,4,11,1,12,7,14,10,3,5,9,6,0,13,2],
    ]
    def bits(x, seq):
        return [((x >> (64 - i)) & 1) for i in seq]
    def frombits(b):
        v = 0
        for bit in b: v = (v << 1) | bit
        return v
    def shift(b, n): return b[n:] + b[:n]
    def fe(r, k):
        e = [r[i-1] for i in E]
        x = [e[i] ^ k[i] for i in range(48)]
        out = []
        for i in range(8):
            chunk = x[i*6:(i+1)*6]
            row = (chunk[0] << 1) | chunk[5]
            col = (chunk[1] << 3) | (chunk[2] << 2) | (chunk[3] << 1) | chunk[4]
            out += [(S[i][row*16+col] >> (3-j)) & 1 for j in range(4)]
        return [out[i-1] for i in P]
    k = bits(int.from_bytes(key, 'big'), PC1)
    C, D = k[:28], k[28:]
    subkeys = []
    for rnd in range(16):
        C, D = shift(C, 1 if rnd in (0,1,8,15) else 2), shift(D, 1 if rnd in (0,1,8,15) else 2)
        subkeys.append(frombits([cat[i-1] for i in PC2 for cat in [C+D]]))
    out = b''
    for block in (data[:8], data[8:]):
        m = bits(int.from_bytes(block, 'big'), IP)
        L, R = m[:32], m[32:]
        for rnd in range(16):
            L, R = R, [L[i] ^ fe(R, [((subkeys[rnd] >> (48-1-j)) & 1) for j in range(48)])[i] for i in range(32)]
        out += frombits([ (L+R)[i-1] for i in FP]).to_bytes(8, 'big')
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=5907)
    ap.add_argument('--password', default=None)
    ap.add_argument('--text', default=None)
    ap.add_argument('--text-file', default=None)
    ap.add_argument('--paste', default=None)
    ap.add_argument('--keys', default=None, help='comma chord, e.g. win+r')
    ap.add_argument('--click', default=None, help='x,y[,double]')
    ap.add_argument('--info', action='store_true')
    args = ap.parse_args()
    v = VNC(args.host, args.port, args.password)
    if args.info:
        print('remote display %s %dx%d' % (v.name, v.width, v.height))
    if args.keys:
        for chord in args.keys.split(';'):
            v.chord(chord.split('+'))
            time.sleep(0.3)
    if args.text:
        v.type_text(args.text)
    if args.text_file:
        with open(args.text_file, 'r', encoding='utf-8') as fh:
            v.type_text(fh.read().strip())
    if args.paste:
        v.paste(args.paste)
        time.sleep(0.3)
    if args.click:
        parts = args.click.split(',')
        v.click(int(parts[0]), int(parts[1]), double=(len(parts) > 2 and parts[2] == 'double'))
    v.close()

if __name__ == '__main__':
    main()
