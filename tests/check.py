# SPDX-License-Identifier: MIT
# Builds every expected packet with struct, then compares with what the C encoder wrote.
import struct, subprocess, sys

def s(x):
    b = x.encode() + b"\0"
    return b + b"\0" * (-len(b) % 4)

def msg(addr, tags, *args):
    return s(addr) + s(tags) + b"".join(args)

unix_ms = 1795382657123
secs = unix_ms // 1000 + 2208988800
frac = ((unix_ms % 1000) << 32) // 1000
p = msg("/presence/P", ",ffi", struct.pack(">f", 72.0), struct.pack(">f", 0.9), struct.pack(">i", 1800))
want = {
    "hr": msg("/avatar/parameters/HR", ",i", struct.pack(">i", 72)),
    "conn_t": msg("/avatar/parameters/isHRConnected", ",T"),
    "conn_f": msg("/avatar/parameters/isHRConnected", ",F"),
    "pct": msg("/avatar/parameters/HRPercent", ",f", struct.pack(">f", 72.0 / 255.0)),
    "p": s("#bundle") + struct.pack(">II", secs, frac) + struct.pack(">i", len(p)) + p,
}
lines = subprocess.check_output([sys.argv[1]], text=True).splitlines()
conf = next(l for l in lines if l.startswith("conf ")).split()[1:]
got = dict(l.split() for l in lines if not l.startswith("conf "))
bad = 0
for k, v in want.items():
    ok = got.get(k) == v.hex()
    bad += not ok
    print(("ok  " if ok else "FAIL"), k, len(v), "bytes")
ok = conf == ["1.0000", "0.5000", "0.0000", "0.0000"]
bad += not ok
print(("ok  " if ok else "FAIL"), "confidence falls from 1 to 0 over a minute")
ok = got.get("small") == "0"
bad += not ok
print(("ok  " if ok else "FAIL"), "too small a buffer returns 0")
sys.exit(1 if bad else 0)
