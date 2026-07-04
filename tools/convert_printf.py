#!/usr/bin/env python3
"""
Convert printf() calls in codegen.c to asm_*() calls from codegen_asm.h.
Simplified approach: extract format string content, match against known patterns.
"""

import re, sys

def extract_reg_index(expr):
    """Extract register index from expressions like reg64[3], reg32[r], reg(r_val, sz).
    Returns (index_str, size) where size is int or string (for variable size)."""
    m = re.match(r'reg64\s*\[([^\]]+)\]$', expr)
    if m: return m.group(1), 8
    m = re.match(r'reg32\s*\[([^\]]+)\]$', expr)
    if m: return m.group(1), 4
    m = re.match(r'reg16\s*\[([^\]]+)\]$', expr)
    if m: return m.group(1), 2
    m = re.match(r'reg8\s*\[([^\]]+)\]$', expr)
    if m: return m.group(1), 1
    m = re.match(r'reg\s*\(([^,]+),\s*([^)]+)\)$', expr)
    if m: return m.group(1), m.group(2)
    return None, None

def ri(expr):
    """Get register index string from a single arg expression."""
    rv, _ = extract_reg_index(expr)
    return rv

def sz(expr):
    """Get size from a register expression."""
    _, s = extract_reg_index(expr)
    return s


def split_args(s):
    """Split arguments by top-level commas only."""
    args = []
    depth = 0
    current = ''
    in_str = False
    in_char = False
    esc = False
    for c in s:
        if esc:
            esc = False
            current += c
            continue
        if c == '\\' and (in_str or in_char):
            esc = True
            current += c
            continue
        if c == '"' and not in_char:
            in_str = not in_str
        elif c == "'" and not in_str:
            in_char = not in_char
        if not in_str and not in_char:
            if c in '([{':
                depth += 1
            elif c in ')]}':
                depth -= 1
            elif c == ',' and depth == 0:
                args.append(current.strip())
                current = ''
                continue
        current += c
    if current.strip():
        args.append(current.strip())
    return args


def convert_one_line(line):
    """Convert a single line containing printf() to asm_() calls."""
    if 'printf' not in line:
        return line

    m = re.search(r'\bprintf\s*\((.*)\)\s*;?\s*(?://.*)?$', line)
    if not m:
        return line

    inner = m.group(1)
    start_pos = m.start()
    end_pos = m.end()
    prefix = line[:start_pos]
    suffix = line[end_pos:]

    args = split_args(inner)
    if not args:
        return line

    fmt_raw = args[0].strip()
    result = match_and_convert(fmt_raw, args[1:])

    if result is not None:
        # Always ensure result ends with semicolon
        result = result.rstrip()
        if not result.endswith(';'):
            result += ';'
        return prefix + result + suffix

    # Mark as unconverted
    fmt_short = fmt_raw[:60].replace('\n', '\\n')
    return prefix + f'(void)0 /* FIXME: unconverted printf: {fmt_short} */;' + suffix


# Known C macros used in string concatenation within printf format args
MACRO_MAP = {
    'FRAME_PTR': 'rbp',     # x86_64: rbp (ARM64: x29 — handled by ifdef in original)
    'STACK_REG': 'rsp',
    'LINK_REG': 'x30',
}

def normalize_fmt(fmt_raw):
    """Extract the logical assembly string from a printf format argument.
    Returns the instruction text as a single string (with \\n stripped)."""
    # Handle simple case: "  mov %s, %s\n"
    if fmt_raw.startswith('"') and fmt_raw.endswith('"') and fmt_raw.count('"') == 2:
        s = fmt_raw[1:-1]
        s = s.replace('\\n', '\n').replace('\\t', '\t').replace('%%', '%').replace('\\"', '"')
        return s

    # Handle concatenated format strings like "  str %s, [" FRAME_PTR ", #-%d]\n"
    parts = []
    i = 0
    while i < len(fmt_raw):
        if fmt_raw[i] == '"':
            j = i + 1
            while j < len(fmt_raw):
                if fmt_raw[j] == '\\':
                    j += 2
                    continue
                if fmt_raw[j] == '"':
                    break
                j += 1
            lit = fmt_raw[i+1:j]
            lit = lit.replace('\\n', '\n').replace('\\t', '\t').replace('%%', '%').replace('\\"', '"')
            parts.append(lit)
            i = j + 1
        elif fmt_raw[i].isalpha() or fmt_raw[i] == '_':
            j = i
            while j < len(fmt_raw) and (fmt_raw[j].isalnum() or fmt_raw[j] == '_'):
                j += 1
            macro = fmt_raw[i:j]
            # Replace known macros with actual register names
            if macro in MACRO_MAP:
                parts.append(MACRO_MAP[macro])
            else:
                parts.append(macro)
            i = j
        else:
            i += 1

    return ''.join(parts)


def match_and_convert(fmt_raw, args):
    """Match the format string + args against known patterns."""

    # Normalize: get the logical instruction string
    fmt_norm = normalize_fmt(fmt_raw)
    # Strip trailing newlines and leading whitespace
    fmt = fmt_norm.strip()

    # Also keep the original raw format for exact matches
    is_simple = fmt_raw.startswith('"') and fmt_raw.endswith('"') and fmt_raw.count('"') == 2

    # ============================================================
    # DIRECTIVES AND LABELS (check raw format for %s patterns)
    # ============================================================

    if is_simple:
        # Labels: "%s:\n"
        if fmt_raw == '"%s:\\n"':
            if args:
                return f'cg_def_label({args[0]});'

        # "%s = %s\n"
        if fmt_raw == '"%s = %s\\n"':
            return f'(void)0 /* directive */'

        # .globl %s
        if fmt_raw == '".globl %s\\n"':
            return f'(void)0 /* .globl symbol handled by objfile */'

        # .weak / .weak_reference / .weak_definition %s
        if fmt_raw in ('".weak %s\\n"', '".weak_reference %s\\n"', '".weak_definition %s\\n"'):
            return f'(void)0 /* .weak symbol */'

        # .set %s, %s
        if fmt_raw == '".set %s, %s\\n"':
            return f'(void)0 /* .set directive */'

        # .file %d "%s"
        if fmt_raw == '"  .file %d \\"%s\\"\\n"':
            return f'(void)0 /* .file directive */'

        # .loc %d %d 0
        if fmt_raw == '"  .loc %d %d 0\\n"':
            return f'(void)0 /* .loc directive */'

        # Plain newline / section headers
        if fmt_raw == '"\\n"':
            return f'(void)0 /* empty line */'

        if fmt_raw.startswith('"\\n.section ') or fmt_raw.startswith('"\\n.text') or fmt_raw.startswith('"\\n.data'):
            return f'(void)0 /* section directive */'

        if fmt_raw.startswith('".section ') or fmt_raw.startswith('".text') or fmt_raw.startswith('".data'):
            return f'(void)0 /* section directive */'

        # .quad/.long/.word/.2byte/.4byte/.byte/.zero/.balign/.p2align with args
        for directive in ['.quad', '.long', '.word', '.2byte', '.4byte', '.byte', '.zero', '.balign', '.p2align']:
            if fmt_raw.startswith(f'"  {directive} '):
                return f'(void)0 /* directive: {directive} */'

    # ============================================================
    # NOW MATCH BY THE NORMALIZED INSTRUCTION TEXT (fmt)
    # ============================================================

    # ---- RET / LEAVE / NOP / CLD / MFENCE / DMB ----
    if fmt == 'ret':
        return f'asm_ret(cg_sec);'
    if fmt == 'leave':
        return f'asm_leave(cg_sec);'
    if fmt in ('nop', 'cld'):
        return f'asm_nop(cg_sec);'
    if fmt == 'mfence':
        return f'(void)0 /* mfence TODO */'
    if fmt.startswith('dmb ish'):
        return f'asm_dmb(cg_sec);'

    # ---- STACK: STP/LDP x29,x30 (ARM64) ----
    if fmt == 'stp x29, x30, [sp, #-16]!':
        return f'asm_stp_fp_lr(cg_sec);'
    if fmt == 'ldp x29, x30, [sp], #16':
        return f'asm_ldp_fp_lr(cg_sec);'

    # ---- x86 push/pop ----
    p = [
        ('pushq %rbp', 'X86_RBP'), ('popq %rbp', 'X86_RBP'),
        ('pushq %rdi', 'X86_RDI'), ('popq %rdi', 'X86_RDI'),
        ('pushq %rcx', 'X86_RCX'), ('popq %rcx', 'X86_RCX'),
        ('pushq %rsi', 'X86_RSI'), ('popq %rsi', 'X86_RSI'),
    ]
    for pattern, reg in p:
        if f'pushq {pattern.split()[1]}' == fmt:
            return f'asm_push_phy(cg_sec, {reg});'
        if f'popq {pattern.split()[1]}' == fmt:
            return f'asm_pop_phy(cg_sec, {reg});'

    # ---- FLOAT ----
    if fmt == 'fcvt d0, s0':
        return f'(void)0 /* arm64 fcvt d0,s0 */'
    if fmt == 'fcvt s0, d0':
        return f'(void)0 /* arm64 fcvt s0,d0 */'
    if fmt == 'cvtss2sd %xmm0, %xmm0':
        return f'asm_cvtss2sd(cg_sec);'
    if fmt == 'cvtsd2ss %xmm0, %xmm0':
        return f'asm_cvtsd2ss(cg_sec);'

    # ---- cvtsi2sd %s, %%xmm0 ----
    if fmt.startswith('cvtsi2sd ') and fmt.endswith(', %xmm0'):
        if len(args) >= 1:
            rv, sv = extract_reg_index(args[0])
            if rv and sv:
                return f'asm_cvtsi2sd(cg_sec, {rv}, {sv});'

    # cvtsi2sd %rcx, %xmm0
    if fmt == 'cvtsi2sd %rcx, %xmm0':
        return f'asm_cvtsi2sd(cg_sec, 1, 8);'

    # ---- cvttsd2si %xmm0, %s ----
    if fmt.startswith('cvttsd2si %xmm0, '):
        if len(args) >= 1:
            rv, sv = extract_reg_index(args[0])
            if rv and sv:
                return f'asm_cvttsd2si(cg_sec, {rv}, {sv});'

    # ---- CSET / SETCC ----
    if fmt.startswith('cset '):
        parts = fmt.split()
        if len(parts) >= 3:
            cond_map = {'ne':'ARM64_NE','eq':'ARM64_EQ','hs':'ARM64_HS','lo':'ARM64_LO',
                        'mi':'ARM64_MI','pl':'ARM64_PL','vs':'ARM64_VS','vc':'ARM64_VC',
                        'hi':'ARM64_HI','ls':'ARM64_LS','ge':'ARM64_GE','lt':'ARM64_LT',
                        'gt':'ARM64_GT','le':'ARM64_LE','cs':'ARM64_CS','cc':'ARM64_CC'}
            cond = parts[2]
            if cond in cond_map and args:
                rv = ri(args[0])
                if rv:
                    return f'asm_cset(cg_sec, {rv}, {cond_map[cond]});'

    if fmt.startswith('set') and fmt.endswith('%al'):
        cond = fmt[3:-3].strip()
        cmap = {'ne':'X86_NE','e':'X86_E','c':'X86_C','b':'X86_B','nc':'X86_NC',
                'ae':'X86_AE','z':'X86_Z','nz':'X86_NZ','s':'X86_S','ns':'X86_NS',
                'o':'X86_O','no':'X86_NO','a':'X86_A','be':'X86_BE','g':'X86_G',
                'ge':'X86_GE','l':'X86_L','le':'X86_LE','p':'X86_P','np':'X86_NP'}
        if cond in cmap:
            return f'asm_setcc(cg_sec, 0, {cmap[cond]});'

    if fmt.startswith('set') and fmt.endswith('%cl'):
        cond = fmt[3:-3].strip()
        cmap = {'ne':'X86_NE','e':'X86_E','c':'X86_C','b':'X86_B','nc':'X86_NC',
                'p':'X86_P','np':'X86_NP'}
        if cond in cmap:
            return f'asm_setcc(cg_sec, 1, {cmap[cond]});'

    # ---- MOV REG, REG (most common) ----
    # "mov %s, %s"
    if fmt.startswith('mov ') and ', ' in fmt:
        parts = fmt[4:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                dst, dsz = extract_reg_index(args[0])
                src, ssz = extract_reg_index(args[1])
                if dst and src:
                    sz_val = dsz if isinstance(dsz, int) else (ssz if isinstance(ssz, int) else 8)
                    return f'asm_mov_reg_reg(cg_sec, {dst}, {src}, {sz_val});'
                if dst and not src:
                    return f'(void)0 /* FIXME: mov {args[0]}, {args[1]} */'

    # "movq %s, %s"
    if fmt.startswith('movq ') and ', ' in fmt:
        parts = fmt[5:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                dst, _ = extract_reg_index(args[0])
                src, _ = extract_reg_index(args[1])
                if dst and src:
                    return f'asm_mov_reg_reg(cg_sec, {dst}, {src}, 8);'

    # "movl %s, %s"
    if fmt.startswith('movl ') and ', ' in fmt:
        parts = fmt[5:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                dst, _ = extract_reg_index(args[0])
                src, _ = extract_reg_index(args[1])
                if dst and src:
                    return f'asm_mov_reg_reg(cg_sec, {dst}, {src}, 4);'

    # ---- MOV IMMEDIATE ----
    # "movq $%d, %%rcx"
    m = re.match(r'^movq \$%d, %%rcx$', fmt)
    if m and args:
        return f'asm_mov_imm(cg_sec, 1, 8, {args[0]});'

    # "movq $%d, %%r10"
    m = re.match(r'^movq \$%d, %%r10$', fmt)
    if m and args:
        return f'asm_mov_imm(cg_sec, 10, 8, {args[0]});'

    # "movabsq $%llu, %%rax"
    if fmt.startswith('movabsq $%llu, %rax'):
        if args:
            return f'asm_movabs_phy(cg_sec, X86_RAX, (uint64_t)({args[0]}));'

    # "movabs $%lld, %s"
    if fmt.startswith('movabs $%lld, '):
        if len(args) >= 2:
            rv = ri(args[1])
            if rv:
                return f'asm_mov_imm(cg_sec, {rv}, 8, {args[0]});'

    # "mov $0, %s" → xor zero (must be BEFORE generic mov $..., %s)
    m = re.match(r'^mov \$0, %s$', fmt)
    if m and args:
        rv, sv = extract_reg_index(args[0])
        if rv:
            if sv == 8:
                return f'asm_movq_zero(cg_sec, {rv});'
            return f'asm_movl_zero(cg_sec, {rv});'

    # "mov $%d, %s" or "mov $%lld, %s" (with format specifiers, not hardcoded constants)
    m = re.match(r'^mov \$(%d|%lld|%llu), %s$', fmt)
    if m and args:
        val = args[0]
        if len(args) >= 2:
            rv, sv = extract_reg_index(args[1])
        if rv:
            return f'asm_mov_imm(cg_sec, {rv}, {sv}, {val});'

    # "movq $0, %s"
    if fmt.startswith('movq $0, '):
        if args:
            rv = ri(args[0])
            if rv:
                return f'asm_movq_zero(cg_sec, {rv});'

    # ARM64: "mov %s, #%d" / "mov %s, #%llu"
    m = re.match(r'^mov %s, #(%d|%llu)$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[0])
        if rv and sv:
            return f'asm_mov_imm(cg_sec, {rv}, {sv}, {args[1]});'

    # ARM64: "mov %s, #0"
    m = re.match(r'^mov %s, #0$', fmt)
    if m and args:
        rv, sv = extract_reg_index(args[0])
        if rv:
            return f'asm_movq_zero(cg_sec, {rv});'

    # ARM64: "mov %s, #1"
    m = re.match(r'^mov %s, #1$', fmt)
    if m and args:
        rv, sv = extract_reg_index(args[0])
        if rv:
            return f'asm_mov_imm(cg_sec, {rv}, {sv}, 1);'

    # ARM64: "mov xN, #%d" (physical reg)
    m = re.match(r'^mov (x\d+|w\d+), #(%d|%llu)?$', fmt)
    if m and args:
        reg = m.group(1)
        sz_val = 8 if reg.startswith('x') else 4
        pnum = int(reg[1:])
        val = args[0]
        # Check for special constants
        if '15360' in fmt:
            return f'asm_mov_imm(cg_sec, {pnum}, {sz_val}, 15360);'
        return f'asm_mov_imm(cg_sec, {pnum}, {sz_val}, {val});'

    # ---- MOVK (ARM64) ----
    # "movk %s, #%d, lsl #%d"
    m = re.match(r'^movk %s, #(%d|%llu), lsl #%d$', fmt)
    if m and len(args) >= 3:
        rv, sv = extract_reg_index(args[0])
        if rv:
            is64 = isinstance(sv, int) and sv == 8
            return f'asm_movk(cg_sec, {rv}, {1 if is64 else 0}, (uint16_t)({args[1]}), {args[2]});'

    # "movk x16, #%d, lsl #%d"
    m = re.match(r'^movk x16, #(%d|%llu), lsl #%d$', fmt)
    if m and len(args) >= 2:
        return f'asm_movk(cg_sec, 16, 1, (uint16_t)({args[0]}), {args[1]});'

    # "movk x17, #%d, lsl #%d"
    m = re.match(r'^movk x17, #(%d|%llu), lsl #%d$', fmt)
    if m and len(args) >= 2:
        return f'asm_movk(cg_sec, 17, 1, (uint16_t)({args[0]}), {args[1]});'

    # "movk x13, #%d, lsl #%d"
    m = re.match(r'^movk x13, #(%d|%llu), lsl #%d$', fmt)
    if m and len(args) >= 2:
        return f'asm_movk(cg_sec, 13, 1, (uint16_t)({args[0]}), {args[1]});'

    # ---- LOAD/STORE FROM/TO FRAME POINTER ----
    # "mov -%d(%%rbp), %s"
    m = re.match(r'^mov -%d\(%rbp\), %s$', fmt)
    if m and args:
        rv, sv = extract_reg_index(args[1])
        if rv:
            return f'asm_mov_rbp_reg(cg_sec, {rv}, {sv}, {args[0]});'

    # "mov %s, -%d(%%rbp)"
    m = re.match(r'^mov %s, -%d\(%rbp\)$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[0])
        if rv:
            return f'asm_mov_reg_rbp(cg_sec, {rv}, {sv}, {args[1]});'

    # "movq -%d(%%rbp), %%reg"
    m = re.match(r'^movq -%d\(%rbp\), %(r\w+)$', fmt)
    if m and args:
        reg = m.group(1)
        x86_map = {'rax':'X86_RAX','rcx':'X86_RCX','rdx':'X86_RDX','rbx':'X86_RBX',
                   'rsp':'X86_RSP','rbp':'X86_RBP','rsi':'X86_RSI','rdi':'X86_RDI',
                   'r8':'X86_R8','r9':'X86_R9','r10':'X86_R10','r11':'X86_R11',
                   'r12':'X86_R12','r13':'X86_R13','r14':'X86_R14','r15':'X86_R15'}
        if reg in x86_map:
            return f'asm_mov_rbp_phyreg(cg_sec, {x86_map[reg]}, 8, {args[0]});'

    # "movq %reg, -%d(%%rbp)"
    m = re.match(r'^movq %(\w+), -%d\(%rbp\)$', fmt)
    if m and args:
        reg = m.group(1)
        x86_map = {'rax':'X86_RAX','rcx':'X86_RCX','rdx':'X86_RDX','rbx':'X86_RBX',
                   'rsp':'X86_RSP','rbp':'X86_RBP','rsi':'X86_RSI','rdi':'X86_RDI',
                   'r8':'X86_R8','r9':'X86_R9','r10':'X86_R10','r11':'X86_R11',
                   'r12':'X86_R12','r13':'X86_R13','r14':'X86_R14','r15':'X86_R15'}
        if reg in x86_map:
            return f'asm_mov_phyreg_rbp(cg_sec, {x86_map[reg]}, 8, {args[0]});'

    # "lea -%d(%%rbp), %s"
    m = re.match(r'^lea -%d\(%rbp\), %s$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[1])
        if rv:
            return f'asm_lea_rbp_reg(cg_sec, {rv}, {sv}, {args[0]});'

    # "leaq -%d(%%rbp), %%reg"
    m = re.match(r'^leaq -%d\(%rbp\), %(\w+)$', fmt)
    if m and args:
        reg = m.group(1)
        x86_map = {'rax':'X86_RAX','rcx':'X86_RCX','rdx':'X86_RDX','rbx':'X86_RBX',
                   'rsp':'X86_RSP','rbp':'X86_RBP','rsi':'X86_RSI','rdi':'X86_RDI',
                   'r8':'X86_R8','r9':'X86_R9','r10':'X86_R10','r11':'X86_R11',
                   'r12':'X86_R12','r13':'X86_R13','r14':'X86_R14','r15':'X86_R15'}
        if reg in x86_map:
            return f'asm_lea_rbp_phy(cg_sec, {x86_map[reg]}, 8, {args[0]});'

    # ---- SIGN/ZERO EXTEND ----
    if fmt.startswith('sxtw '):
        if len(args) >= 2:
            d, _ = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_movsx(cg_sec, {d}, {s}, 8, 4);'
    if fmt.startswith('sxth '):
        if len(args) >= 2:
            d, _ = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_movsx(cg_sec, {d}, {s}, 4, 2);'
    if fmt.startswith('sxtb '):
        if len(args) >= 2:
            d, _ = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_movsx(cg_sec, {d}, {s}, 4, 1);'
    if fmt.startswith('uxtb '):
        if len(args) >= 2:
            d, _ = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_movzx(cg_sec, {d}, {s}, 4, 1);'
    if fmt.startswith('uxth '):
        if len(args) >= 2:
            d, _ = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_movzx(cg_sec, {d}, {s}, 4, 2);'

    ext_pairs = [
        ('movsbl ', 4, 1), ('movsbq ', 8, 1), ('movswl ', 4, 2), ('movswq ', 8, 2),
        ('movslq ', 8, 4), ('movzbl ', 4, 1), ('movzwl ', 4, 2),
    ]
    for prefix, dsz, ssz in ext_pairs:
        if fmt.startswith(prefix):
            parts = fmt[len(prefix):].split(', ')
            if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
                if len(args) >= 2:
                    d, _ = extract_reg_index(args[1]); s, _ = extract_reg_index(args[0])
                    if d and s: return f'asm_movsx(cg_sec, {d}, {s}, {dsz}, {ssz});' if 's' in prefix[:4] else f'asm_movzx(cg_sec, {d}, {s}, {dsz}, {ssz});'

    # movzbl %al, %eax
    if fmt == 'movzbl %al, %eax':
        return f'asm_movzx(cg_sec, 0, 0, 4, 1);'
    # movzbl %al, %s
    if fmt.startswith('movzbl %al, '):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_movzx(cg_sec, {rv}, 0, 4, 1);'

    # ---- LOAD / STORE INDIRECT (ARM64: ldr/str) ----
    # "ldr %s, [%s]"
    if fmt.startswith('ldr ') and fmt.endswith(']'):
        parts = fmt[4:].split(', [')
        if len(parts) == 2:
            regpart = parts[0].strip()
            addrpart = parts[1].rstrip(']').strip()
            if regpart == '%s' and addrpart == '%s':
                if len(args) >= 2:
                    d, dsz = extract_reg_index(args[0])
                    s, _ = extract_reg_index(args[1])
                    if d and s:
                        sz_val = dsz if isinstance(dsz, int) else 8
                        return f'asm_ldr_reg_off(cg_sec, {d}, {s}, {sz_val}, 0);'

    # "str %s, [%s]"
    if fmt.startswith('str ') and fmt.endswith(']'):
        parts = fmt[4:].split(', [')
        if len(parts) == 2:
            regpart = parts[0].strip()
            addrpart = parts[1].rstrip(']').strip()
            if regpart == '%s' and addrpart == '%s':
                if len(args) >= 2:
                    s, ssz = extract_reg_index(args[0])
                    d, _ = extract_reg_index(args[1])
                    if s and d:
                        sz_val = ssz if isinstance(ssz, int) else 8
                        return f'asm_str_reg_off(cg_sec, {s}, {d}, {sz_val}, 0);'

    # ---- ARITHMETIC ----
    # "add %s, %s, #%d"
    m = re.match(r'^add %s, %s, #(%d|%lld)$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[0])
        if rv:
            return f'asm_add_imm(cg_sec, {rv}, {sv}, {args[1]});'

    # "add %s, %s, #1"
    if fmt == 'add %s, %s, #1':
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_add_imm(cg_sec, {rv}, {sv}, 1);'

    # "add %s, %s, %s"
    if fmt.startswith('add ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3 and parts[0] == '%s' and parts[1] == '%s' and parts[2] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_add_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "add $%d, %s"
    m = re.match(r'^add \$(%d), %s$', fmt)
    if m and args:
        if len(args) >= 2:
            rv, sv = extract_reg_index(args[1])
        else:
            rv, sv = ri(args[0]), sz(args[0])
        if rv:
            return f'asm_add_imm(cg_sec, {rv}, {sv}, {args[0]});'

    # "sub %s, %s, #%d"
    m = re.match(r'^sub %s, %s, #(%d|%lld)$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[0])
        if rv:
            return f'asm_sub_imm(cg_sec, {rv}, {sv}, {args[1]});'

    # "sub %s, %s, #1"
    if fmt == 'sub %s, %s, #1':
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_sub_imm(cg_sec, {rv}, {sv}, 1);'

    # "sub %s, %s, %s"
    if fmt.startswith('sub ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3 and parts[0] == '%s' and parts[1] == '%s' and parts[2] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_sub_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "subq $%d, %%rsp"
    m = re.match(r'^subq \$(%d), %rsp$', fmt)
    if m and args:
        return f'asm_sub_imm(cg_sec, 4, 8, {args[0]});'

    # "subq $1, %%rcx"
    if fmt == 'subq $1, %rcx':
        return f'asm_dec(cg_sec, 1, 8);'

    # "subq $1, %%r10"
    if fmt == 'subq $1, %r10':
        return f'asm_dec(cg_sec, 10, 8);'

    # "sub x9, x9, #1"
    if fmt == 'sub x9, x9, #1':
        return f'asm_dec(cg_sec, 9, 8);'

    # "subl %%ecx, %%eax"
    if fmt == 'subl %ecx, %eax':
        return f'asm_sub_reg_reg(cg_sec, 0, 1, 4);'

    # "mul %s, %s, %s"
    if fmt.startswith('mul ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3 and parts[0] == '%s' and parts[1] == '%s' and parts[2] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_mul_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "sdiv %s, %s, %s" / "udiv %s, %s, %s"
    if (fmt.startswith('sdiv ') or fmt.startswith('udiv ')):
        parts = fmt.split(', ')
        if len(parts) == 3:
            signed = fmt.startswith('sdiv')
            if args:
                rv, sv = extract_reg_index(args[0])
                if rv: return f'asm_div_reg(cg_sec, {rv}, {sv}, {"true" if signed else "false"});'

    # "neg %s"
    if fmt.startswith('neg '):
        parts = fmt[4:].strip()
        if ',' not in parts:
            if args:
                rv, sv = extract_reg_index(args[0])
                if rv: return f'asm_neg(cg_sec, {rv}, {sv});'

    # "mvn %s, %s"
    if fmt.startswith('mvn '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_not(cg_sec, {rv}, {sv});'

    # "not %s"
    if fmt.startswith('not '):
        parts = fmt[4:].strip()
        if ' ' not in parts and ',' not in parts:
            if args:
                rv, sv = extract_reg_index(args[0])
                if rv: return f'asm_not(cg_sec, {rv}, {sv});'

    # "dec %s"
    if fmt.startswith('dec '):
        parts = fmt[4:].strip()
        if ' ' not in parts and ',' not in parts:
            if args:
                rv, sv = extract_reg_index(args[0])
                if rv: return f'asm_dec(cg_sec, {rv}, {sv});'

    # "xorl %%eax, %%eax"
    if fmt == 'xorl %eax, %eax':
        return f'asm_movl_zero(cg_sec, 0);'
    # "xorl %%edx, %%edx"
    if fmt == 'xorl %edx, %edx':
        return f'asm_movl_zero(cg_sec, 2);'

    # "xor %s, %s"
    if fmt.startswith('xor ') and ', ' in fmt:
        parts = fmt[4:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_xor_reg_reg(cg_sec, {d}, {s}, {ds});'

    # ---- LOGICAL ----
    # "and %s, %s, %s"
    if fmt.startswith('and ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3 and parts[0] == '%s' and parts[1] == '%s' and parts[2] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_and_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "and %s, %s, x16"
    if fmt.startswith('and ') and fmt.endswith(', x16'):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_and_reg_reg(cg_sec, {rv}, 16, {sv});'

    # "andq %%rax, %s"
    if fmt.startswith('andq %rax, '):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_and_reg_reg(cg_sec, {rv}, 0, 8);'

    # "orr %s, %s, %s"
    if fmt.startswith('orr ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3 and parts[0] == '%s' and parts[1] == '%s' and parts[2] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_or_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "or %s, %s"
    if fmt.startswith('or ') and ', ' in fmt:
        parts = fmt[3:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_or_reg_reg(cg_sec, {d}, {s}, {ds});'

    # "eor %s, %s, %s"
    if fmt.startswith('eor ') and fmt.count(',') >= 2:
        parts = fmt[4:].split(', ')
        if len(parts) == 3:
            if len(args) >= 2:
                d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
                if d and s: return f'asm_eor_reg_reg(cg_sec, {d}, {s}, {ds});'

    # ---- BIT OPERATIONS ----
    # "bswap %s"
    if fmt.startswith('bswap '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_bswap(cg_sec, {rv}, {sv});'

    # "clz %s, %s"
    if fmt.startswith('clz '):
        if len(args) >= 2:
            d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_clz(cg_sec, {d}, {s}, {ds});'

    # "rbit %s, %s"
    if fmt.startswith('rbit '):
        if len(args) >= 2:
            d, ds = extract_reg_index(args[0]); s, _ = extract_reg_index(args[1])
            if d and s: return f'asm_rbit(cg_sec, {d}, {s}, {ds});'

    # "rev %s, %s"
    if fmt.startswith('rev '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_rev(cg_sec, {rv}, {sv});'

    # "rev16 %s, %s"
    if fmt.startswith('rev16 '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_rev16(cg_sec, {rv}, {sv});'

    # ---- SHIFTS ----
    # "lsl %s, %s, #%d" / "lsr %s, %s, #%d" / "asr %s, %s, #%d"
    m = re.match(r'^(lsl|lsr|asr) %s, %s, #(%d|%lld)$', fmt)
    if m and len(args) >= 2:
        op = m.group(1)
        shift = args[1]
        rv, sv = extract_reg_index(args[0])
        if rv:
            if op == 'lsl': return f'asm_shl_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'
            elif op == 'lsr': return f'asm_shr_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'
            else: return f'asm_sar_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'

    # "shl $%d, %s" / "shr $%d, %s" / "sar $%d, %s"
    m = re.match(r'^(shl|shr|sar) \$(%d), %s$', fmt)
    if m and len(args) >= 2:
        op = m.group(1)
        shift = args[0]
        rv, sv = extract_reg_index(args[1])
        if rv:
            if op == 'shl': return f'asm_shl_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'
            elif op == 'shr': return f'asm_shr_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'
            else: return f'asm_sar_imm(cg_sec, {rv}, {sv}, (uint8_t)({shift}));'

    # "shrq %%rcx"
    if fmt == 'shrq %rcx':
        return f'asm_shl_cl(cg_sec, 1, 8);'

    # ---- COMPARE ----
    # "cmp %s, #0"
    if fmt.startswith('cmp ') and fmt.endswith(', #0'):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_cmp_zero(cg_sec, {rv}, {sv});'

    # "cmp $0, %s"
    if fmt.startswith('cmp $0, '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_cmp_zero(cg_sec, {rv}, {sv});'

    # "cmp %s, %s"
    if fmt.startswith('cmp ') and ', ' in fmt:
        parts = fmt[4:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                a, asz = extract_reg_index(args[0]); b, bsz = extract_reg_index(args[1])
                if a and b:
                    sz_val = asz if isinstance(asz, int) else (bsz if isinstance(bsz, int) else 8)
                    return f'asm_cmp_reg_reg(cg_sec, {a}, {b}, {sz_val});'

    # "cmpq $0, %%rcx"
    if fmt == 'cmpq $0, %rcx':
        return f'asm_cmp_zero(cg_sec, 1, 8);'

    # "cmpq $0, %%r10"
    if fmt == 'cmpq $0, %r10':
        return f'asm_cmp_zero(cg_sec, 10, 8);'

    # "cmp x9, #0"
    if fmt == 'cmp x9, #0':
        return f'asm_cmp_zero(cg_sec, 9, 8);'

    # "cmp %s, #%lld"
    m = re.match(r'^cmp %s, #%lld$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[0])
        if rv: return f'asm_cmp_imm(cg_sec, {rv}, {sv}, {args[1]});'

    # "cmp $%lld, %s"
    m = re.match(r'^cmp \$%lld, %s$', fmt)
    if m and len(args) >= 2:
        rv, sv = extract_reg_index(args[1])
        if rv: return f'asm_cmp_imm(cg_sec, {rv}, {sv}, {args[0]});'

    # "testq %s, %s"
    if fmt.startswith('testq ') and ', ' in fmt:
        parts = fmt[6:].split(', ')
        if len(parts) == 2 and parts[0] == '%s' and parts[1] == '%s':
            if len(args) >= 2:
                a, asz = extract_reg_index(args[0]); b, _ = extract_reg_index(args[1])
                if a and b: return f'asm_test_reg_reg(cg_sec, {a}, {b}, 8);'

    # ---- BRANCHES / CALLS ----
    # "call *%s"
    if fmt.startswith('call *%s'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_call_reg(cg_sec, {rv});'

    # ---- MOVQ physical-to-codegen-reg ----
    # "movq %%rdi, %%rax"
    if fmt == 'movq %rdi, %rax':
        return f'asm_mov_reg_reg(cg_sec, 0, 7, 8);'
    # "movq %%rcx, %%rax"
    if fmt == 'movq %rcx, %rax':
        return f'asm_mov_reg_reg(cg_sec, 0, 1, 8);'
    # "movq %%rbp, %s"
    if fmt.startswith('movq %rbp, '):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 5, 8);'
    # "movq %%r11, %%rax"
    if fmt == 'movq %r11, %rax':
        return f'asm_mov_reg_reg(cg_sec, 0, 11, 8);'
    # "movq %%rax, %s"
    if fmt.startswith('movq %rax, '):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 0, 8);'
    # "movq %s, %%rcx"
    if fmt.startswith('movq ') and fmt.endswith(', %rcx'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, 1, {rv}, 8);'
    # "movq %s, %%rax"
    if fmt.startswith('movq ') and fmt.endswith(', %rax'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, 0, {rv}, 8);'
    # "movq %s, %%rdi"
    if fmt.startswith('movq ') and fmt.endswith(', %rdi'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, 7, {rv}, 8);'
    # "movq %s, %%rsi"
    if fmt.startswith('movq ') and fmt.endswith(', %rsi'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, 6, {rv}, 8);'
    # "movq %s, %%r11"
    if fmt.startswith('movq ') and fmt.endswith(', %r11'):
        if args:
            rv = ri(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, 11, {rv}, 8);'

    # "movl %%eax, %s" / "movl %%edx, %s" / "movl %s, %%eax" / "movl %s, %%ecx"
    if fmt == 'movl %eax, %s' and args:
        rv = ri(args[0])
        if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 0, 4);'
    if fmt == 'movl %edx, %s' and args:
        rv = ri(args[0])
        if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 2, 4);'
    if fmt == 'movl %s, %eax' and args:
        rv = ri(args[0])
        if rv: return f'asm_mov_reg_reg(cg_sec, 0, {rv}, 4);'
    if fmt == 'movl %s, %ecx' and args:
        rv = ri(args[0])
        if rv: return f'asm_mov_reg_reg(cg_sec, 1, {rv}, 4);'

    # "incq %%rdi" / "incq %%rsi"
    if fmt == 'incq %rdi': return f'asm_inc(cg_sec, 7, 8);'
    if fmt == 'incq %rsi': return f'asm_inc(cg_sec, 6, 8);'
    # "decq %%rcx"
    if fmt == 'decq %rcx': return f'asm_dec(cg_sec, 1, 8);'

    # ---- ARM64: ldr/str from fp with offset ----
    # "ldr %s, [%s, #-%d]"  (third arg is FRAME_PTR which is %s)
    m = re.match(r'^ldr %s, \[%s, #-%d\]$', fmt)
    if m and len(args) >= 3:
        rv, sv = extract_reg_index(args[0])
        if rv: return f'asm_ldr_fp_imm(cg_sec, {rv}, {sv}, {args[2]});'

    # "str %s, [%s, #-%d]"
    m = re.match(r'^str %s, \[%s, #-%d\]$', fmt)
    if m and len(args) >= 3:
        rv, sv = extract_reg_index(args[0])
        if rv: return f'asm_str_fp_imm(cg_sec, {rv}, {sv}, {args[2]});'

    # "str %s, [x17]" etc
    if re.match(r'^str %s, \[x\d+\]$', fmt):
        return f'(void)0 /* FIXME: str via x17 */'

    # "ldr %s, [x17]" etc
    if re.match(r'^ldr %s, \[x\d+\]$', fmt):
        return f'(void)0 /* FIXME: ldr via x17 */'

    # ---- Catch remaining with broad patterns ----

    # Multi-line embedded newlines in format
    if '\n' in fmt and fmt.count('\n') >= 1:
        return f'(void)0 /* FIXME: multi-instruction printf */'

    # Branches / jumps / calls (broad)
    for prefix in ['b ', 'b.', 'bl ', 'blr ', 'br ', 'call ', 'jmp ', 'je ', 'jne ', 'jz ', 'js ', 'jb ', 'ja ', 'jp ', 'jae ',
                   'cbz ', 'cbnz ', 'b.ne ', 'b.eq ', 'b.lt ', 'b.ge ', 'b.pl ', 'b.hi ', 'b.vs ',
                   'jne ', 'jp ']:
        if fmt.startswith(prefix):
            return f'(void)0 /* FIXME: branch/call: {fmt[:40]} */'

    # Sub with x16, x17, etc.
    if fmt.startswith('sub ') and ('x16' in fmt or 'x17' in fmt or 'x13' in fmt or 'x11' in fmt or 'x0' in fmt):
        return f'(void)0 /* FIXME: sub with phy reg */'

    # subs
    if fmt.startswith('subs '):
        return f'(void)0 /* FIXME: subs */'

    # sub %s, %s (2-op)
    if fmt.startswith('sub ') and ', ' in fmt:
        parts = fmt[4:].split(', ')
        if len(parts) == 2:
            return f'(void)0 /* FIXME: sub 2op */'

    # adds
    if fmt.startswith('adds '):
        return f'(void)0 /* FIXME: adds */'

    # atomic ops
    for ap in ['ldxr ', 'stxr ', 'ldxrb ', 'stxrb ', 'ldxrh ', 'stxrh ', 'ldar ', 'stlr ',
               'ldaxr ', 'stlxr', 'swp', 'cas']:
        if fmt.startswith(ap):
            return f'(void)0 /* FIXME: atomic */'

    # lock prefix
    if fmt.startswith('lock '):
        return f'(void)0 /* FIXME: lock */'

    # rep strings
    if fmt.startswith(('rep ', 'repe ', 'repne ')):
        return f'(void)0 /* FIXME: rep */'

    # mov with physical regs
    m = re.match(r'^mov (x\d+|w\d+|sp),', fmt)
    if m:
        return f'(void)0 /* FIXME: mov phy */'

    # mov%c patterns for indirect load/store
    if fmt.startswith('mov%c ') or fmt.startswith('movb ') or fmt.startswith('movw '):
        return f'(void)0 /* FIXME: sized mov */'

    # Indirect movs
    if fmt.startswith('mov (') or fmt.startswith('movq (') or fmt.startswith('movl ('):
        return f'(void)0 /* FIXME: indirect mov */'
    if fmt.startswith('mov ') and '(' in fmt:
        return f'(void)0 /* FIXME: mov indirect/mem */'

    # GOT / PIC loads
    if '@GOTPCREL' in fmt or '@GOTPAGE' in fmt or '@GOTPAGEOFF' in fmt or ':got:' in fmt or ':lo12:' in fmt:
        return f'(void)0 /* FIXME: GOT load */'

    # RIP-relative
    if '(%rip)' in fmt or '(%%rip)' in fmt:
        return f'(void)0 /* FIXME: rip-relative */'

    # alloca
    if 'alloca' in fmt.lower() or 'Lalloca' in fmt:
        return f'(void)0 /* FIXME: alloca */'

    # ldr/str with physical regs or offsets
    if fmt.startswith(('ldr ', 'str ')) and ('x' in fmt or 'w' in fmt or '#' in fmt):
        return f'(void)0 /* FIXME: ldr/str phy/off */'
    if fmt.startswith(('ldrb ', 'ldrh ', 'strb ', 'strh ', 'ldur ', 'stur ')):
        return f'(void)0 /* FIXME: sized ld/st */'

    # ldp/stp
    if fmt.startswith(('ldp ', 'stp ')):
        return f'(void)0 /* FIXME: ldp/stp */'

    # adrp/adr
    if fmt.startswith('adrp ') or fmt.startswith('adr '):
        return f'(void)0 /* FIXME: adrp/adr */'

    # fmov
    if fmt.startswith('fmov '):
        return f'(void)0 /* FIXME: fmov */'

    # fneg/fabs
    if fmt.startswith('fneg ') or fmt.startswith('fabs '):
        return f'(void)0 /* FIXME: fneg/fabs */'

    # Other float
    if fmt.startswith(('fcvt ', 'comisd ', 'ucomisd ', 'subsd ', 'addsd ', 'xorpd ',
                        'scvtf ', 'ucvtf ', 'fcvtzs ', 'fcvtzu ', 'addss ',
                        'addsd ', 'movsd ', 'movss ', 'pxor ')):
        return f'(void)0 /* FIXME: float op */'

    # Sized arithmetic/logical ops
    if fmt.startswith(('add%c ', 'sub%c ', 'and%c ', 'or%c ', 'xor%c ')):
        return f'(void)0 /* FIXME: sized alu op */'
    if fmt.startswith(('addl ', 'addq ', 'subq ', 'andl ', 'andq ', 'orq ', 'xorq ')):
        return f'(void)0 /* FIXME: sized alu op */'
    if fmt.startswith(('cmpb ', 'cmpq ', 'cmpl ', 'cmn ', 'tst ')):
        return f'(void)0 /* FIXME: cmp variant */'
    if fmt.startswith('testb '):
        return f'(void)0 /* FIXME: testb */'

    # and/or/xor with physical regs or immediates
    if fmt.startswith('and ') or fmt.startswith('andq '):
        return f'(void)0 /* FIXME: and variant */'
    if fmt.startswith('or ') and ', ' in fmt:
        return f'(void)0 /* FIXME: or variant */'
    if fmt.startswith('orr ') and not fmt.count(',') >= 2:
        return f'(void)0 /* FIXME: orr variant */'
    if fmt.startswith('eor ') and not fmt.count(',') >= 2:
        return f'(void)0 /* FIXME: eor variant */'

    # idiv/div
    if fmt.startswith('idiv ') or fmt.startswith('div '):
        return f'(void)0 /* FIXME: div */'
    # mul (single operand)
    if fmt.startswith('mul ') and ', ' not in fmt:
        return f'(void)0 /* FIXME: mul 1op */'
    # cqo/cdq
    if fmt in ('cqo', 'cdq'):
        return f'(void)0 /* FIXME: cqo/cdq */'
    # neg 2op
    if fmt.startswith('neg ') and ', ' in fmt:
        return f'(void)0 /* FIXME: neg 2op */'
    # notq
    if fmt.startswith('notq '):
        return f'(void)0 /* FIXME: notq */'
    # xchg
    if fmt.startswith('xchg%c '):
        return f'(void)0 /* FIXME: xchg */'
    # rol
    if fmt.startswith('rol '):
        return f'(void)0 /* FIXME: rol */'
    # prfm
    if fmt.startswith('prfm '):
        return f'(void)0 /* FIXME: prfm */'
    # movaps
    if fmt.startswith('movaps '):
        return f'(void)0 /* FIXME: movaps */'
    # fldl/fstpt
    if fmt.startswith(('fldl ', 'fstpt ')):
        return f'(void)0 /* FIXME: x87 */'
    # csel/cneg/cmov
    if fmt.startswith(('csel ', 'cneg ', 'cmov')):
        return f'(void)0 /* FIXME: cmov/csel/cneg */'
    # lea variations
    if fmt.startswith('lea') or fmt.startswith('leaq ') or fmt.startswith('leal '):
        return f'(void)0 /* FIXME: lea */'
    # lsl/lsr with physical regs
    if (fmt.startswith('lsl ') or fmt.startswith('lsr ')) and ('x' in fmt or '#' not in fmt.split(',')[-1] if ',' in fmt else True):
        return f'(void)0 /* FIXME: lsl/lsr phy */'
    # shrq/sarq with $imm
    if fmt.startswith(('shrq $', 'sarq $', 'sar $')):
        return f'(void)0 /* FIXME: shift imm */'
    # popcnt/lzcnt/tzcnt/bsf/cls
    if fmt.startswith(('popcnt ', 'lzcnt ', 'tzcnt ', 'bsf ', 'cls ')):
        return f'(void)0 /* FIXME: bit scan */'
    # sete %s
    if fmt.startswith('sete ') or fmt.startswith('set'):
        return f'(void)0 /* FIXME: sete */'
    # movq $0x..., special constants
    if fmt.startswith('movq $0x') or fmt.startswith('movq $-'):
        return f'(void)0 /* FIXME: movq special imm */'
    # movl $...
    if fmt.startswith('movl $'):
        return f'(void)0 /* FIXME: movl imm */'
    # movzbl/movzwl indirect
    if (fmt.startswith('movzbl (') or fmt.startswith('movzwl (') or
        fmt.startswith('movsbl (') or fmt.startswith('movswl (')):
        return f'(void)0 /* FIXME: indirect ext */'
    if fmt.startswith('movzbl -') or fmt.startswith('movzbl 0x') or fmt.startswith('movzbl 8('):
        return f'(void)0 /* FIXME: movzbl addr */'
    # movsbl from mem
    if 'movsbl -1(' in fmt or 'movzbl (%%rsi)' in fmt:
        return f'(void)0 /* FIXME: string op */'
    # ins v%d / cnt / addv / ubfx / umull etc.
    if fmt.startswith(('ins ', 'cnt ', 'addv ', 'ubfx ', 'umull ', 'umulh ', 'smull ', 'smulh ')):
        return f'(void)0 /* FIXME: NEON */'
    # sub xN, ...
    if fmt.startswith('sub x') or fmt.startswith('sub w'):
        return f'(void)0 /* FIXME: sub phy */'
    # add xN/wN,...
    if fmt.startswith('add x') or fmt.startswith('add w'):
        return f'(void)0 /* FIXME: add phy */'
    # and xN, ...
    if fmt.startswith('and x'):
        return f'(void)0 /* FIXME: and phy */'
    # orr xN, xN, xN
    if fmt.startswith('orr x'):
        return f'(void)0 /* FIXME: orr phy */'

    # ---- SHIFT BY LITERAL CONSTANT ----
    # lsl/lsr/asr %s, %s, #N (2 args, shift is literal)
    m = re.match(r'^(lsl|lsr|asr) %s, %s, #(\d+)$', fmt)
    if m and len(args) >= 2:
        op = m.group(1)
        shift_val = int(m.group(2))
        rv, sv = extract_reg_index(args[0])
        if rv:
            if op == 'lsl': return f'asm_shl_imm(cg_sec, {rv}, {sv}, {shift_val});'
            elif op == 'lsr': return f'asm_shr_imm(cg_sec, {rv}, {sv}, {shift_val});'
            else: return f'asm_sar_imm(cg_sec, {rv}, {sv}, {shift_val});'

    # ---- LABELS (catch more patterns) ----
    # .L.xxx.%d: patterns → cg_def_label
    m = re.match(r'^\.L\.\w+\.%d:$', fmt)
    if m and args:
        # Build label name from the %d arg
        return f'(void)0 /* FIXME: label .L.xxx.{args[0]} */'

    # ---- MOVQ TO/FROM PHYSICAL REGS (additional) ----
    # movq %rdx, %s
    if fmt == 'movq %rdx, %s' and args:
        rv = ri(args[0])
        if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 2, 8);'

    # movq %rcx, %s (already handled above but double check)

    # movq %s, %xmm0 / movq %xmm0, %s
    if fmt in ('movq %s, %xmm0', 'movq %xmm0, %s'):
        return f'(void)0 /* TODO: movq to/from xmm */'

    # ---- CSET with variable condition (%s) ----
    if re.match(r'^cset %s, %s$', fmt):
        return f'(void)0 /* FIXME: cset with var cond */'

    # ---- GENERIC 2-OPERAND ----
    # %s %s, %s  (like notq %, etc.)
    m = re.match(r'^%s %s, %s$', fmt)
    if m:
        return f'(void)0 /* FIXME: generic 2op: {fmt} */'

    # ---- MOV with FRAME_PTR literal ----
    # mov %s, rbp (from FRAME_PTR macro expansion)
    if fmt.startswith('mov ') and fmt.endswith(', rbp'):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 5, {sv});'

    # mov rbp, %s
    if fmt.startswith('mov rbp, '):
        if args:
            rv, sv = extract_reg_index(args[0])
            if rv: return f'asm_mov_reg_reg(cg_sec, {rv}, 5, {sv});'

    # ---- DIRECTIVES caught by normalized format ----
    if fmt.startswith('.file ') or fmt.startswith('.loc '):
        return f'(void)0 /* directive */'

    # %s (%s)\n  (e.g., lock (%s), some prefix ops)
    if re.match(r'^%s \(%s\)$', fmt):
        return f'(void)0 /* FIXME: prefix (indirect) */'

    # mov%c with explicit size from arg[0]
    if re.match(r'^mov%c %s, \(%s\)$', fmt):
        return f'(void)0 /* FIXME: mov%c to indirect */'

    return None


def main():
    filepath = 'src/codegen.c'
    with open(filepath, 'r') as f:
        lines = f.readlines()

    converted = 0
    unconverted = 0
    new_lines = []

    for line in lines:
        result = convert_one_line(line.rstrip('\n'))
        if result is not None:
            if 'FIXME: unconverted' in result:
                unconverted += 1
            elif 'printf' not in result:
                converted += 1
        else:
            result = line.rstrip('\n')
        new_lines.append(result)

    output = '\n'.join(new_lines) + '\n'

    with open(filepath, 'w') as f:
        f.write(output)

    print(f"Converted: {converted}, Unconverted: {unconverted}")
    count = 0
    for line in new_lines:
        if 'FIXME: unconverted' in line:
            print(f"  UNCONVERTED: {line.strip()[:120]}")
            count += 1
            if count >= 20:
                print(f"  ... ({unconverted - 20} more)")
                break

if __name__ == '__main__':
    main()
