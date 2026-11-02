"""Static decoder for SDK-generated Win64 PMF materializers. Never executes code.

Accepts a small straight-line MOV/LEA/zero/copy subset, only stores into the
24-byte output buffer, and at most two direct tail thunks. All other code fails.
This handles exactly the two daAlink native methods pinned by pe_audit.py.
"""
import struct


def decode_materializer(pe, va, size, import_slots):
    if size != 24:
        raise ValueError('expected native daAlink 24-byte PMF')
    raw = pe.raw
    regs = {1: ('out', 0)}  # Win64 RCX = buffer owned by the loader, not Link
    xmm = {}
    out = bytearray(size)
    written = set()
    pc, branches = va, 0

    def read(addr, n):
        off = pe.off(addr - pe.image_base)
        b = raw[off:off+n]
        if len(b) != n:
            raise ValueError('truncated static materializer read')
        return b

    def number(b):
        return int.from_bytes(b, 'little')

    def getreg(r, width):
        v = regs.get(r)
        if isinstance(v, int):
            return v.to_bytes(8, 'little', signed=False)[:width]
        if isinstance(v, bytes):
            return v[:width]
        raise ValueError('unknown materializer register')

    def operand(pos, rex):
        modrm = read(pos, 1)[0]
        mod, reg, rm = modrm >> 6, ((modrm >> 3) & 7) + (8 if rex & 4 else 0), (modrm & 7) + (8 if rex & 1 else 0)
        pos += 1
        if mod == 3:
            return reg, ('reg', rm), pos
        if rm & 7 == 4:
            raise ValueError('SIB/stack materializer is not approved')
        disp = 0
        if mod == 0 and rm & 7 == 5:
            disp = struct.unpack('<i', read(pos, 4))[0]
            return reg, ('rip', disp), pos+4
        if mod == 1:
            disp = struct.unpack('<b', read(pos, 1))[0]; pos += 1
        elif mod == 2:
            disp = struct.unpack('<i', read(pos, 4))[0]; pos += 4
        base = regs.get(rm)
        if not isinstance(base, tuple) and not isinstance(base, int):
            raise ValueError('unproven materializer memory base')
        return reg, ('mem', base, disp), pos

    def address(op, end):
        if op[0] == 'rip':
            return end + op[1]
        if op[0] == 'mem':
            if isinstance(op[1], tuple):
                return ('out', op[1][1] + op[2])
            return op[1] + op[2]
        raise ValueError('expected memory address')

    def load(op, end, n):
        if op[0] == 'reg':
            return getreg(op[1], n)
        a = address(op, end)
        if isinstance(a, tuple):
            raise ValueError('materializer must not read its output before initialization')
        return read(a, n)

    def store(op, end, b):
        a = address(op, end)
        if not isinstance(a, tuple) or a[0] != 'out' or a[1] < 0 or a[1]+len(b) > size:
            raise ValueError('materializer writes outside its loader-owned buffer')
        out[a[1]:a[1]+len(b)] = b
        written.update(range(a[1],a[1]+len(b)))

    def finish():
        if not set(range(20)).issubset(written):
            raise ValueError('materializer did not initialize target and adjustment fields')
        if any(out[8:20]):
            raise ValueError('nonzero this/virtual-base adjustment is not approved: '+out.hex())
        return bytes(out)

    for _ in range(40):
        start = pc
        rex, prefix = 0, None
        op = read(pc,1)[0]; pc += 1
        if op in (0x66,0xf3):
            prefix=op;op=read(pc,1)[0];pc+=1
        if 0x40 <= op <= 0x4f:
            rex=op;op=read(pc,1)[0];pc+=1
        width = 8 if rex & 8 else 4
        if op == 0xc3:
            return finish()
        if op == 0x90:
            continue
        if op == 0xe9:
            delta=struct.unpack('<i',read(pc,4))[0];pc=pc+4+delta;branches+=1
            if branches>2:raise ValueError('materializer branch budget exceeded')
            continue
        if 0xb8<=op<=0xbf:
            reg=op-0xb8+(8 if rex&1 else 0);regs[reg]=number(read(pc,width));pc+=width;continue
        if op in (0x31,0x33):
            reg,dst,pc=operand(pc,rex)
            if dst!=('reg',reg):raise ValueError('nonzero XOR not approved')
            regs[reg]=0;continue
        if op in (0x89,0x8b,0x8d,0xc7):
            reg,dst,pc=operand(pc,rex)
            if op==0x8d:
                regs[reg]=address(dst,pc);continue
            if op==0xc7:
                imm=struct.unpack('<i',read(pc,4))[0];pc+=4;b=(imm & ((1<<(8*width))-1)).to_bytes(width,'little')
                if dst[0]=='reg':regs[dst[1]]=number(b)
                else:store(dst,pc,b)
            elif op==0x8b:
                if dst[0]=='reg' and isinstance(regs.get(dst[1]),tuple):regs[reg]=regs[dst[1]]
                else:regs[reg]=number(load(dst,pc,width))
            elif dst[0]=='reg':regs[dst[1]]=regs.get(reg)
            else:store(dst,pc,getreg(reg,width))
            continue
        if op==0x0f:
            ext=read(pc,1)[0];pc+=1;reg,dst,pc=operand(pc,rex)
            if ext in (0x57,0xef):
                if dst!=('reg',reg):raise ValueError('nonzero vector XOR')
                xmm[reg]=bytes(16)
            elif ext in (0x10,0x28,0x6f):xmm[reg]=load(dst,pc,16)
            elif ext in (0x11,0x29,0x7f):store(dst,pc,xmm[reg])
            else:raise ValueError('unapproved vector opcode %02x'%ext)
            continue
        if op==0xff:
            reg,dst,pc=operand(pc,rex)
            slot=address(dst,pc)
            if reg!=4 or not isinstance(slot,int) or import_slots.get(slot-pe.image_base,('', ''))[1]!='memcpy':
                raise ValueError('unapproved materializer indirect branch')
            if regs.get(1)!=('out',0) or regs.get(8)!=size or not isinstance(regs.get(2),int):
                raise ValueError('unproven memcpy arguments')
            store(('mem',('out',0),0),pc,read(regs[2],size));return finish()
        raise ValueError('unsupported materializer opcode at %#x: %s'%(start,read(start,40).hex()))
    raise ValueError('materializer instruction budget exceeded')
