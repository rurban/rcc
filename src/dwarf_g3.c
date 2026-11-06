// SPDX-License-Identifier: LGPL-2.1-or-later
// -g3 full DWARF generation through libdwarf's producer library (libdwarfp):
// subprograms, formal parameters, locals in lexical blocks, globals and the
// type graph, next to the line table. ELF x86-64 only (RCC_DWARF3); codegen
// records function/scope ranges (ObjFile.debug_fns) and the parser keeps the
// extra Type/Typedef/Node data this file reads.
//
// All .debug_* sections are created as ObjFile extra sections; every
// libdwarfp symbolic relocation is translated to a normal ObjReloc:
//   * references to another .debug_* section -> R_X86_64_32 (section symbol)
//   * addresses (.text anchor, global variables) -> R_X86_64_64
// The in-place value libdwarfp already wrote becomes the addend.
#include "rcc.h"
#include "obj.h"

#ifdef RCC_DWARF3
#include <libdwarf.h>
#include <libdwarfp.h>
#include <dwarf.h>

int cg_obj_var_sym_idx(LVar *var);

// libdwarfp symbol indexes handed out are SYMBASE + ObjFile sym index
// (0 means "no symbol" to libdwarfp).
#define SYMBASE 1000000
#define MAX_SECS 32
#define MACRO_SENTINEL 0x7ffffff1u // 4-byte (data4) placeholder, see emit_macros()

typedef struct {
    const void *key;
    unsigned q;
    Dwarf_P_Die die;
} TEnt;

typedef struct {
    const char *name;
    Dwarf_P_Die die;
} BaseEnt;

typedef struct {
    ObjFile *obj;
    Program *prog;
    Dwarf_P_Debug dbg;
    Dwarf_Error err;
    bool failed;
    const char *what;
    char secname[MAX_SECS][32];
    int nsec;
    int text_sym; // ObjFile sym idx of the .text section symbol
    Dwarf_P_Die cu;
    TEnt *tt;
    size_t tcap, tcnt;
    bool macro_patch;
    unsigned char *mac; // .debug_macinfo bytes
    size_t maclen, maccap;
    BaseEnt base[64];
    int nbase;
} G;

static G g;

#define D(call)                                                                                    \
    do {                                                                                           \
        if (!g.failed && (call) != DW_DLV_OK) {                                                    \
            g.failed = true;                                                                       \
            g.what = #call;                                                                        \
        }                                                                                          \
    } while (0)

// ---------------------------------------------------------------- sections
static int sec_anchor(const char *name) {
    int sec = objfile_find_or_add_section(g.obj, name, 0, 0);
    return objfile_add_sym(g.obj, name, sec, 0, 0, SB_LOCAL, ST_SECTION);
}

static int sect_cb(const char *name, int size, Dwarf_Unsigned type, Dwarf_Unsigned flags,
                   Dwarf_Unsigned link, Dwarf_Unsigned info, Dwarf_Unsigned *sect_name_index,
                   void *user_data, int *error) {
    (void)size, (void)type, (void)flags, (void)link, (void)info, (void)user_data;
    if (g.nsec >= MAX_SECS) {
        *error = DW_DLE_ELF_SECT_ERR;
        return -1;
    }
    snprintf(g.secname[g.nsec], sizeof g.secname[0], "%s", name);
    // libdwarfp also announces ".rel.debug_*" relocation sections; those are
    // not ours to create (relocations go to ObjReloc lists).
    *sect_name_index = strncmp(name, ".rel", 4) ? SYMBASE + sec_anchor(name) : 0;
    return ++g.nsec;
}

// ---------------------------------------------------------------- DIE helpers
static Dwarf_P_Die new_die(Dwarf_Tag tag, Dwarf_P_Die parent) {
    Dwarf_P_Die d = NULL;
    D(dwarf_new_die_a(g.dbg, tag, parent, NULL, NULL, NULL, &d, &g.err));
    return g.failed ? NULL : d;
}

static void at_name(Dwarf_P_Die die, const char *s) {
    Dwarf_P_Attribute a;
    if (die && s && *s) D(dwarf_add_AT_name_a(die, (char *)s, &a, &g.err));
}

static void at_u(Dwarf_P_Die die, Dwarf_Half at, Dwarf_Unsigned v) {
    Dwarf_P_Attribute a;
    if (die) D(dwarf_add_AT_unsigned_const_a(g.dbg, die, at, v, &a, &g.err));
}

static void at_s(Dwarf_P_Die die, Dwarf_Half at, int64_t v) {
    Dwarf_P_Attribute a;
    if (die) D(dwarf_add_AT_signed_const_a(g.dbg, die, at, (Dwarf_Signed)v, &a, &g.err));
}

static void at_flag(Dwarf_P_Die die, Dwarf_Half at) {
    Dwarf_P_Attribute a;
    if (die) D(dwarf_add_AT_flag_a(g.dbg, die, at, 1, &a, &g.err));
}

static void at_ref(Dwarf_P_Die die, Dwarf_Half at, Dwarf_P_Die target) {
    Dwarf_P_Attribute a;
    if (die && target) D(dwarf_add_AT_reference_c(g.dbg, die, at, target, &a, &g.err));
}

static void at_block(Dwarf_P_Die die, Dwarf_Half at, const char *bytes, size_t n) {
    Dwarf_P_Attribute a;
    if (die) D(dwarf_add_AT_block_a(g.dbg, die, at, (Dwarf_Small *)bytes, n, &a, &g.err));
}

static void at_text_range(Dwarf_P_Die die, uint64_t start, uint64_t size) {
    Dwarf_P_Attribute a;
    if (!die) return;
    D(dwarf_add_AT_targ_address_c(g.dbg, die, DW_AT_low_pc, start, SYMBASE + g.text_sym, &a,
                                  &g.err));
    at_u(die, DW_AT_high_pc, size);
}

// DW_AT_location = DW_OP_fbreg(off); frame base is rbp (see subprogram).
static void at_loc_fbreg(Dwarf_P_Die die, int off) {
    Dwarf_P_Expr e = NULL;
    Dwarf_Unsigned n;
    Dwarf_P_Attribute a;
    if (!die) return;
    D(dwarf_new_expr_a(g.dbg, &e, &g.err));
    D(dwarf_add_expr_gen_a(e, DW_OP_fbreg, (Dwarf_Unsigned)(Dwarf_Signed)off, 0, &n, &g.err));
    D(dwarf_add_AT_location_expr_a(g.dbg, die, DW_AT_location, e, &a, &g.err));
}

static void at_loc_addr(Dwarf_P_Die die, int sym_idx) {
    Dwarf_P_Expr e = NULL;
    Dwarf_Unsigned n;
    Dwarf_P_Attribute a;
    if (!die) return;
    D(dwarf_new_expr_a(g.dbg, &e, &g.err));
    D(dwarf_add_expr_addr_c(e, 0, SYMBASE + sym_idx, &n, &g.err));
    D(dwarf_add_AT_location_expr_a(g.dbg, die, DW_AT_location, e, &a, &g.err));
}

// ---------------------------------------------------------------- type cache
static TEnt *tfind(const void *key, unsigned q) {
    if (!g.tcap) return NULL;
    size_t i = (((uintptr_t)key >> 4) ^ (q * 0x9e3779b1u)) & (g.tcap - 1);
    while (g.tt[i].key) {
        if (g.tt[i].key == key && g.tt[i].q == q) return &g.tt[i];
        i = (i + 1) & (g.tcap - 1);
    }
    return NULL;
}

static void tput(const void *key, unsigned q, Dwarf_P_Die die);
static void tgrow(void) {
    TEnt *old = g.tt;
    size_t ocap = g.tcap;
    g.tcap = ocap ? ocap * 2 : 256;
    g.tt = calloc(g.tcap, sizeof(TEnt));
    if (!g.tt) {
        fprintf(stderr, "rcc: out of memory\n");
        exit(1);
    }
    g.tcnt = 0;
    for (size_t i = 0; i < ocap; i++)
        if (old[i].key) tput(old[i].key, old[i].q, old[i].die);
    free(old);
}

static void tput(const void *key, unsigned q, Dwarf_P_Die die) {
    if ((g.tcnt + 1) * 10 > g.tcap * 7) tgrow();
    size_t i = (((uintptr_t)key >> 4) ^ (q * 0x9e3779b1u)) & (g.tcap - 1);
    while (g.tt[i].key)
        i = (i + 1) & (g.tcap - 1);
    g.tt[i] = (TEnt){key, q, die};
    g.tcnt++;
}

// ---------------------------------------------------------------- types
static Dwarf_P_Die type_die(Type *ty);

static Dwarf_P_Die base_die(const char *name, Dwarf_Unsigned enc, int64_t size) {
    for (int i = 0; i < g.nbase; i++)
        if (!strcmp(g.base[i].name, name)) return g.base[i].die;
    Dwarf_P_Die d = new_die(DW_TAG_base_type, g.cu);
    at_name(d, name);
    at_u(d, DW_AT_encoding, enc);
    at_u(d, DW_AT_byte_size, size);
    if (g.nbase < 64) g.base[g.nbase++] = (BaseEnt){name, d};
    return d;
}

static Dwarf_P_Die int_base(int64_t size, bool uns) {
    switch (size) {
    case 1: return uns ? base_die("unsigned char", DW_ATE_unsigned_char, 1)
                       : base_die("signed char", DW_ATE_signed_char, 1);
    case 2: return uns ? base_die("short unsigned int", DW_ATE_unsigned, 2)
                       : base_die("short int", DW_ATE_signed, 2);
    case 8: return uns ? base_die("long unsigned int", DW_ATE_unsigned, 8)
                       : base_die("long int", DW_ATE_signed, 8);
    case 16: return uns ? base_die("unsigned __int128", DW_ATE_unsigned, 16)
                        : base_die("__int128", DW_ATE_signed, 16);
    default: return uns ? base_die("unsigned int", DW_ATE_unsigned, 4)
                        : base_die("int", DW_ATE_signed, 4);
    }
}

static int file_idx(const char *fname);
static bool is_void(Type *t) { return !t || t->kind == TY_VOID; }

static void fill_members(Dwarf_P_Die sd, Type *canon) {
    for (int i = 0; i < canon->dbg_member_count; i++) {
        DebugTMember *m = &canon->dbg_members[i];
        Dwarf_P_Die md = new_die(DW_TAG_member, sd);
        at_name(md, m->name);
        Dwarf_P_Die td = type_die(m->ty);
        at_ref(md, DW_AT_type, td);
        at_u(md, DW_AT_data_member_location, m->offset);
        if (m->bit_width) {
            // libdwarfp only accepts the DWARF2-style (MSB-relative)
            // DW_AT_bit_offset, not DW_AT_data_bit_offset.
            Dwarf_Unsigned unit = m->ty ? (Dwarf_Unsigned)m->ty->size : 4;
            at_u(md, DW_AT_byte_size, unit);
            at_u(md, DW_AT_bit_size, m->bit_width);
            at_u(md, DW_AT_bit_offset, unit * 8 - m->bit_offset - m->bit_width);
        }
    }
}

static Dwarf_P_Die unqualified_die(Type *ty) {
    switch (ty->kind) {
    case TY_VOID: return NULL;
    case TY_BOOL: return base_die("_Bool", DW_ATE_boolean, 1);
    case TY_CHAR:
        return ty->is_unsigned ? base_die("unsigned char", DW_ATE_unsigned_char, 1)
                               : base_die(ty->is_signed_char ? "signed char" : "char",
                                          DW_ATE_signed_char, 1);
    case TY_INT:
    case TY_SHORT:
    case TY_LONG:
    case TY_LLONG:
    case TY_INT128: {
        if (ty->is_enum) {
            Type *canon = ty->enum_id ? ty->enum_id : ty;
            TEnt *e = tfind(canon, 0);
            if (e) return e->die;
            Dwarf_P_Die ed = new_die(DW_TAG_enumeration_type, g.cu);
            tput(canon, 0, ed);
            at_name(ed, canon->tag_name ? canon->tag_name : ty->tag_name);
            at_u(ed, DW_AT_byte_size, ty->size);
            at_ref(ed, DW_AT_type, int_base(ty->size, ty->is_unsigned));
            Type *src = canon->dbg_enum_count ? canon : ty;
            for (int i = 0; i < src->dbg_enum_count; i++) {
                Dwarf_P_Die c = new_die(DW_TAG_enumerator, ed);
                at_name(c, src->dbg_enums[i].name);
                at_s(c, DW_AT_const_value, src->dbg_enums[i].val);
            }
            return ed;
        }
        if (ty->kind == TY_LLONG)
            return ty->is_unsigned ? base_die("long long unsigned int", DW_ATE_unsigned, 8)
                                   : base_die("long long int", DW_ATE_signed, 8);
        return int_base(ty->size, ty->is_unsigned);
    }
    case TY_FLOAT: return base_die("float", DW_ATE_float, 4);
    case TY_DOUBLE: return base_die("double", DW_ATE_float, 8);
    case TY_LDOUBLE: return base_die("long double", DW_ATE_float, ty->size);
    case TY_COMPLEX:
        return base_die(ty->size == 8        ? "complex float"
                            : ty->size == 16 ? "complex double"
                                             : "complex long double",
                        DW_ATE_complex_float, ty->size);
    case TY_NULLPTR_T: return base_die("decltype(nullptr)", DW_ATE_unsigned, 8);
    case TY_BITINT:
        return base_die(ty->is_unsigned ? "unsigned _BitInt" : "_BitInt",
                        ty->is_unsigned ? DW_ATE_unsigned : DW_ATE_signed, ty->size);
    case TY_DECIMAL32: return base_die("_Decimal32", DW_ATE_decimal_float, 4);
    case TY_DECIMAL64: return base_die("_Decimal64", DW_ATE_decimal_float, 8);
    case TY_DECIMAL128: return base_die("_Decimal128", DW_ATE_decimal_float, 16);
    case TY_PTR: {
        Dwarf_P_Die d = new_die(DW_TAG_pointer_type, g.cu);
        at_u(d, DW_AT_byte_size, 8);
        at_ref(d, DW_AT_type, type_die(ty->base));
        return d;
    }
    case TY_ARRAY:
    case TY_VLA: {
        Dwarf_P_Die d = new_die(DW_TAG_array_type, g.cu);
        Type *el = ty;
        // One array DIE with a subrange per dimension (int a[2][3]).
        while (el && (el->kind == TY_ARRAY || el->kind == TY_VLA) && !el->qual) {
            Dwarf_P_Die sr = new_die(DW_TAG_subrange_type, d);
            int64_t len = el->array_len;
            if (len <= 0 && el->kind == TY_ARRAY && el->base && el->base->size > 0)
                len = el->size / el->base->size;
            if (el->kind == TY_ARRAY && len > 0)
                at_u(sr, DW_AT_upper_bound, (Dwarf_Unsigned)(len - 1));
            el = el->base;
        }
        at_ref(d, DW_AT_type, type_die(el));
        return d;
    }
    case TY_STRUCT:
    case TY_UNION: {
        Type *canon = ty->struct_id ? ty->struct_id : ty;
        TEnt *e = tfind(canon, 0);
        if (e) return e->die;
        Dwarf_P_Die sd =
            new_die(ty->kind == TY_UNION ? DW_TAG_union_type : DW_TAG_structure_type, g.cu);
        tput(canon, 0, sd);
        at_name(sd, canon->tag_name ? canon->tag_name : ty->tag_name);
        if (!canon->has_body && !ty->has_body) {
            at_flag(sd, DW_AT_declaration);
        } else {
            at_u(sd, DW_AT_byte_size, canon->has_body ? canon->size : ty->size);
            fill_members(sd, canon->dbg_member_count ? canon : ty);
        }
        return sd;
    }
    case TY_FUNC: {
        Dwarf_P_Die d = new_die(DW_TAG_subroutine_type, g.cu);
        if (!ty->is_oldstyle) at_flag(d, DW_AT_prototyped);
        if (!is_void(ty->return_ty)) at_ref(d, DW_AT_type, type_die(ty->return_ty));
        for (Type *p = ty->param_types; p; p = p->param_next) {
            Dwarf_P_Die pd = new_die(DW_TAG_formal_parameter, d);
            at_ref(pd, DW_AT_type, type_die(p));
        }
        if (ty->is_variadic) new_die(DW_TAG_unspecified_parameters, d);
        return d;
    }
    }
    return NULL;
}

// Returns the DIE describing `ty` (NULL for void), wrapped in cv-qualifier
// DIEs as needed. Aggregates are canonicalized by struct_id/enum_id.
static Dwarf_P_Die type_die(Type *ty) {
    if (g.failed || !ty) return NULL;
    unsigned q = ty->qual & (QUAL_CONST | QUAL_VOLATILE | QUAL_RESTRICT);
    const void *key = ty->struct_id ? ty->struct_id : ty->enum_id ? ty->enum_id
                                                                  : ty;
    TEnt *e = tfind(key, q);
    if (e) return e->die;
    Dwarf_P_Die d = unqualified_die(ty);
    if (q && !is_void(ty)) {
        static const struct {
            unsigned bit;
            Dwarf_Tag tag;
        } quals[] = {{QUAL_CONST, DW_TAG_const_type},
                     {QUAL_VOLATILE, DW_TAG_volatile_type},
                     {QUAL_RESTRICT, DW_TAG_restrict_type}};
        for (int i = 0; i < 3; i++) {
            if (!(q & quals[i].bit)) continue;
            Dwarf_P_Die w = new_die(quals[i].tag, g.cu);
            at_ref(w, DW_AT_type, d);
            d = w;
        }
    } else if (q) {
        // qualified void: const void
        for (int i = 0; i < 1; i++) {
            Dwarf_P_Die w = new_die(DW_TAG_const_type, g.cu);
            d = w;
        }
    }
    if (g.failed) return NULL;
    // Aggregates/enums were registered unqualified inside unqualified_die();
    // everything else (and every qualified wrapper) is cached here.
    if (!tfind(key, q)) tput(key, q, d);
    return d;
}

// ---------------------------------------------------------------- variables
static bool var_ok(LVar *v) {
    return v->name && v->name[0] && v->name[0] != '.' && v->ty && !v->is_nested_fn &&
        !v->is_function && !v->is_global_reg && !v->is_register && !v->is_tls && !v->is_extern &&
        !v->defer_stmt && v->ty->kind != TY_VLA && v->ty->kind != TY_VOID;
}

static int cmp_ptr(const void *a, const void *b) {
    uintptr_t x = *(const uintptr_t *)a, y = *(const uintptr_t *)b;
    return x < y ? -1 : x > y;
}

static void emit_local(LVar *v, Dwarf_P_Die parent, bool is_param) {
    Dwarf_P_Die d = new_die(is_param ? DW_TAG_formal_parameter : DW_TAG_variable, parent);
    at_name(d, v->name);
    at_ref(d, DW_AT_type, type_die(v->ty));
    if (v->vreg) return; // lives in a register: no memory location
    at_loc_fbreg(d, -v->offset);
}

static int cmp_scope(const void *a, const void *b) {
    const DebugScope *x = a, *y = b;
    if (x->start != y->start) return x->start < y->start ? -1 : 1;
    if (x->end != y->end) return x->end > y->end ? -1 : 1; // larger (outer) first
    return 0;
}

static void emit_slice(Dwarf_P_Die parent, LVar *stop, LVar *newest) {
    // Chain is newest-first; emit in declaration order.
    int n = 0;
    for (LVar *v = newest; v && v != stop; v = v->next)
        n++;
    LVar **a = malloc((size_t)(n ? n : 1) * sizeof *a);
    int i = n;
    for (LVar *v = newest; v && v != stop; v = v->next)
        a[--i] = v;
    for (i = 0; i < n; i++)
        if (var_ok(a[i]) && !a[i]->is_static) emit_local(a[i], parent, false);
    free(a);
}

static int file_idx(const char *fname) {
    if (!fname) return 0;
    for (int i = 0; i < g.obj->debug_file_count; i++)
        if (g.obj->debug_files[i] && !strcmp(g.obj->debug_files[i], fname)) return i + 1;
    return 0;
}

typedef struct {
    uint64_t end;
    Dwarf_P_Die die;
} StkEnt;

static void emit_function(DebugFnRec *rec) {
    Function *fn = rec->fn;
    ObjFile *obj = g.obj;
    Dwarf_P_Die sp = new_die(DW_TAG_subprogram, g.cu);
    at_name(sp, fn->name);
    bool external = rec->sym_idx >= 0 ? obj->syms[rec->sym_idx].bind != SB_LOCAL : !fn->is_static;
    if (external) at_flag(sp, DW_AT_external);
    if (fn->ty && !fn->ty->is_oldstyle) at_flag(sp, DW_AT_prototyped);
    if (fn->body && fn->body->tok && fn->body->tok->lineno > 0) {
        int fi = file_idx(fn->body->tok->filename);
        if (fi) {
            at_u(sp, DW_AT_decl_file, fi);
            at_u(sp, DW_AT_decl_line, fn->body->tok->lineno);
        }
    }
    if (fn->ty && !is_void(fn->ty->return_ty)) at_ref(sp, DW_AT_type, type_die(fn->ty->return_ty));
    at_text_range(sp, rec->start, rec->size);
    // Frame base = rbp+0: rcc functions keep rbp as the frame pointer and
    // address every local as rbp - offset (no CFI is emitted).
    static const char fb[] = {DW_OP_breg6, 0};
    at_block(sp, DW_AT_frame_base, fb, sizeof fb);

    // Parameters.
    size_t nmark = 0, cap = 16;
    uintptr_t *mark = malloc(cap * sizeof *mark);
    for (LVar *p = fn->params; p; p = p->param_next) {
        if (nmark == cap) mark = realloc(mark, (cap *= 2) * sizeof *mark);
        mark[nmark++] = (uintptr_t)p;
        if (var_ok(p)) emit_local(p, sp, true);
    }
    if (fn->is_variadic) new_die(DW_TAG_unspecified_parameters, sp);

    // Lexical blocks, sorted outer-first.
    qsort(rec->scopes, (size_t)rec->scope_count, sizeof(DebugScope), cmp_scope);
    for (int i = 0; i < rec->scope_count; i++) {
        DebugScope *s = &rec->scopes[i];
        for (LVar *v = s->vars_end; v && v != s->vars; v = v->next) {
            if (nmark == cap) mark = realloc(mark, (cap *= 2) * sizeof *mark);
            mark[nmark++] = (uintptr_t)v;
        }
    }
    qsort(mark, nmark, sizeof *mark, cmp_ptr);

    // Function-level locals: on fn->locals but in no block slice.
    int nloc = 0;
    for (LVar *v = fn->locals; v; v = v->next)
        nloc++;
    LVar **top = malloc((size_t)(nloc ? nloc : 1) * sizeof *top);
    int ntop = 0;
    for (LVar *v = fn->locals; v; v = v->next) {
        uintptr_t k = (uintptr_t)v;
        if (!bsearch(&k, mark, nmark, sizeof *mark, cmp_ptr)) top[ntop++] = v;
    }
    for (int i = ntop - 1; i >= 0; i--)
        if (var_ok(top[i]) && !top[i]->is_static) emit_local(top[i], sp, false);
    free(top);
    free(mark);

    // Blocks: stack of enclosing (end, die).
    StkEnt stk[128];
    int sd = 0;
    for (int i = 0; i < rec->scope_count && !g.failed; i++) {
        DebugScope *s = &rec->scopes[i];
        while (sd && s->end > stk[sd - 1].end)
            sd--;
        Dwarf_P_Die parent = sd ? stk[sd - 1].die : sp;
        Dwarf_P_Die blk = new_die(DW_TAG_lexical_block, parent);
        at_text_range(blk, s->start, s->end - s->start);
        emit_slice(blk, s->vars, s->vars_end);
        if (sd < 128) stk[sd++] = (StkEnt){s->end, blk};
    }
}

static void emit_globals(void) {
    ObjFile *obj = g.obj;
    char *seen = calloc((size_t)obj->sym_count + 1, 1);
    for (LVar *v = g.prog->globals; v && !g.failed; v = v->next) {
        if (!var_ok(v) || v->is_local) continue;
        int si = cg_obj_var_sym_idx(v);
        if (si < 0 || seen[si]) continue;
        if (obj->syms[si].section == SEC_UNDEF) continue;
        seen[si] = 1;
        Dwarf_P_Die d = new_die(DW_TAG_variable, g.cu);
        at_name(d, v->name);
        at_ref(d, DW_AT_type, type_die(v->ty));
        if (obj->syms[si].bind != SB_LOCAL) at_flag(d, DW_AT_external);
        at_loc_addr(d, si);
    }
    free(seen);
}

static void emit_typedefs(void) {
    for (Typedef *td = g.prog->typedefs; td && !g.failed; td = td->next) {
        if (!td->dbg_type || !td->name || !td->ty) continue;
        Dwarf_P_Die d = new_die(DW_TAG_typedef, g.cu);
        at_name(d, td->name);
        at_ref(d, DW_AT_type, type_die(td->ty));
    }
}

// ---------------------------------------------------------------- macros
// .debug_macinfo is written here, not through dwarf_def_macro(): libdwarfp's
// composer copies each string's NUL twice, which truncates "name value" at
// the name and desynchronizes the stream. The DWARF 2-4 format is trivial:
// entries are (type, uleb line, string | uleb file), terminated by a 0 byte.
static void mac_byte(unsigned v) {
    if (g.maclen == g.maccap) {
        g.maccap = g.maccap ? g.maccap * 2 : 4096;
        g.mac = realloc(g.mac, g.maccap);
        if (!g.mac) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
    }
    g.mac[g.maclen++] = (unsigned char)v;
}

static void mac_uleb(uint64_t v) {
    do {
        unsigned b = v & 0x7f;
        v >>= 7;
        mac_byte(v ? b | 0x80 : b);
    } while (v);
}

static void mac_str(const char *s) {
    while (*s)
        mac_byte((unsigned char)*s++);
}

// The main file is the root; a change of file either returns to an
// enclosing file (end_file) or enters an include (start_file). The include
// line is not recorded, so start_file entries use line 0.
static void emit_macros(void) {
    if (!pp_macro_events) return;
    const char *stack[256];
    int depth = 0;
    int root = file_idx(g.obj->debug_files[0]);
    stack[depth++] = g.obj->debug_files[0];
    mac_byte(DW_MACINFO_start_file);
    mac_uleb(0);
    mac_uleb(root ? root : 1);
    for (PPMacroEvent *e = pp_macro_events; e; e = e->next) {
        int fi = file_idx(e->file);
        if (!fi) continue;
        if (strcmp(stack[depth - 1], e->file)) {
            int up = -1;
            for (int i = depth - 2; i >= 0; i--)
                if (!strcmp(stack[i], e->file)) {
                    up = i;
                    break;
                }
            if (up >= 0) {
                while (depth - 1 > up) {
                    mac_byte(DW_MACINFO_end_file);
                    depth--;
                }
            } else if (depth < 256) {
                mac_byte(DW_MACINFO_start_file);
                mac_uleb(0);
                mac_uleb(fi);
                stack[depth++] = e->file;
            }
        }
        mac_byte(e->value ? DW_MACINFO_define : DW_MACINFO_undef);
        mac_uleb(e->line);
        mac_str(e->name);
        if (e->value) {
            mac_byte(' ');
            mac_str(e->value);
        }
        mac_byte(0);
    }
    while (depth-- > 0)
        mac_byte(DW_MACINFO_end_file);
    mac_byte(0);

    // libdwarfp never adds the CU's DW_AT_macro_info for us, and
    // dwarf_add_AT_dataref_a writes 8 data bytes for a 4-byte form. So add a
    // 4-byte placeholder attribute (a distinctive DW_AT_decl_line value) and
    // retarget it to DW_AT_macro_info, with a .debug_macinfo relocation,
    // after the transform (see objfile_flush_debug_g3).
    at_u(g.cu, DW_AT_decl_line, MACRO_SENTINEL);
    g.macro_patch = true;
}

// ---------------------------------------------------------------- driver
typedef struct {
    int sec;
    uint64_t off;
    int sym;
    uint32_t type;
    int64_t addend;
} PendRel;

typedef struct {
    int sec; // ObjFile section id
    Dwarf_Unsigned idx;
    const unsigned char *bytes;
    size_t len;
} OutSec;

static bool fail(const char *what) {
    fprintf(stderr, "rcc: libdwarf -g3: %s failed: %s; using line-table-only DWARF\n", what,
            g.err ? dwarf_errmsg_by_number(dwarf_errno(g.err)) : "(no error)");
    return false;
}

static const char *sec_name(Dwarf_Unsigned idx) {
    return idx >= 1 && idx <= (Dwarf_Unsigned)g.nsec ? g.secname[idx - 1] : "";
}

bool objfile_flush_debug_g3(ObjFile *obj, uint64_t text_end, void *program) {
    memset(&g, 0, sizeof g);
    g.obj = obj;
    g.prog = program;
    bool ok = false;
    OutSec out[MAX_SECS];
    int nout = 0;
    PendRel *rels = NULL;
    size_t nrel = 0, caprel = 0;

    g.text_sym = objfile_add_sym(obj, ".text", SEC_TEXT, 0, 0, SB_LOCAL, ST_SECTION);
    D(dwarf_producer_init(DW_DLC_POINTER64 | DW_DLC_TARGET_LITTLEENDIAN |
                              DW_DLC_SYMBOLIC_RELOCATIONS,
                          sect_cb, NULL, NULL, NULL, "x86_64", "V4", NULL, &g.dbg, &g.err));
    if (g.failed) return fail(g.what);

    // Files that only appear in macro events still need a file-table entry.
    for (PPMacroEvent *e = pp_macro_events; e; e = e->next)
        if (!file_idx(e->file)) objfile_add_debug_file(obj, (char *)e->file);

    // --- .debug_line (same table as -g)
    for (int i = 0; i < obj->debug_file_count && !g.failed; i++) {
        Dwarf_Unsigned idx = 0;
        char *fname = obj->debug_files[i] ? obj->debug_files[i] : "<unknown>";
        D(dwarf_add_file_decl_a(g.dbg, fname, 0, 0, 0, &idx, &g.err));
        if (!g.failed && idx != (Dwarf_Unsigned)i + 1) {
            g.failed = true;
            g.what = "file index mismatch";
        }
    }
    D(dwarf_lne_set_address_a(g.dbg, 0, SYMBASE + g.text_sym, &g.err));
    // codegen only emits rows at statements; add one at each function's entry
    // (declaration line) so a breakpoint/backtrace at the first instruction
    // resolves, and so debuggers can skip the prologue to the next row.
    int nf = 0;
    for (int i = 0; i < obj->debug_line_count; i++) {
        const DebugLineEntry *e = &obj->debug_lines[i];
        while (nf < obj->debug_fn_count && obj->debug_fns[nf].start <= e->text_offset) {
            DebugFnRec *r = &obj->debug_fns[nf++];
            Function *fn = r->fn;
            if (r->start == e->text_offset || !fn->body || !fn->body->tok ||
                fn->body->tok->lineno <= 0)
                continue;
            int fi = file_idx(fn->body->tok->filename);
            D(dwarf_add_line_entry_c(g.dbg, fi ? (uint32_t)fi : e->file_idx, r->start,
                                     fn->body->tok->lineno, 0, true, false, false, false, 0, 0,
                                     &g.err));
        }
        D(dwarf_add_line_entry_c(g.dbg, e->file_idx, e->text_offset, e->line, 0, true, false,
                                 false, false, 0, 0, &g.err));
    }
    D(dwarf_lne_end_sequence_a(g.dbg, text_end, &g.err));

    // --- .debug_info
    Dwarf_P_Attribute attr;
    g.cu = new_die(DW_TAG_compile_unit, NULL);
    if (g.cu) {
        D(dwarf_add_AT_producer_a(g.cu, "rcc", &attr, &g.err));
        at_u(g.cu, DW_AT_language, DW_LANG_C11);
        at_name(g.cu, obj->debug_files[0] ? obj->debug_files[0] : "<unknown>");
        at_text_range(g.cu, 0, text_end);
        D(dwarf_add_die_to_debug_a(g.dbg, g.cu, &g.err));
    }
    for (int i = 0; i < obj->debug_fn_count && !g.failed; i++)
        emit_function(&obj->debug_fns[i]);
    emit_globals();
    emit_typedefs();

    emit_macros();

    // --- .debug_aranges
    D(dwarf_add_arange_c(g.dbg, 0, text_end, SYMBASE + g.text_sym, 0, 0, &g.err));
    if (g.failed) {
        fail(g.what);
        goto done;
    }

    Dwarf_Unsigned nbufs = 0;
    D(dwarf_transform_to_disk_form_a(g.dbg, &nbufs, &g.err));
    for (; !g.failed;) {
        Dwarf_Unsigned idx = 0, len = 0;
        Dwarf_Ptr bytes = NULL;
        int r = dwarf_get_section_bytes_a(g.dbg, 0, &idx, &len, &bytes, &g.err);
        if (r == DW_DLV_NO_ENTRY) break;
        if (r != DW_DLV_OK) {
            g.failed = true;
            g.what = "dwarf_get_section_bytes_a";
            break;
        }
        if (len == 0) continue;
        // libdwarfp hands a large section out in several chunks: join them,
        // relocation offsets are section-global.
        OutSec *os = NULL;
        for (int i = 0; i < nout; i++)
            if (out[i].idx == idx) os = &out[i];
        if (!os) {
            if (nout >= MAX_SECS) {
                g.failed = true;
                g.what = "too many sections";
                break;
            }
            os = &out[nout++];
            *os = (OutSec){objfile_find_or_add_section(obj, sec_name(idx), 0, 0), idx, NULL, 0};
        }
        unsigned char *nb = realloc((void *)os->bytes, os->len + (size_t)len);
        if (!nb) {
            fprintf(stderr, "rcc: out of memory\n");
            exit(1);
        }
        memcpy(nb + os->len, bytes, (size_t)len);
        os->bytes = nb;
        os->len += (size_t)len;
    }

    if (g.macro_patch && !g.failed) {
        OutSec *info = NULL, *abbrev = NULL;
        for (int i = 0; i < nout; i++) {
            if (!strcmp(sec_name(out[i].idx), ".debug_info")) info = &out[i];
            if (!strcmp(sec_name(out[i].idx), ".debug_abbrev")) abbrev = &out[i];
        }
        size_t at = 0;
        if (info) {
            for (size_t o = 11; o + 4 <= info->len && o < 256; o++) {
                uint32_t v;
                memcpy(&v, info->bytes + o, 4);
                if (v == MACRO_SENTINEL) {
                    at = o;
                    break;
                }
            }
        }
        unsigned char *ab = abbrev ? (unsigned char *)abbrev->bytes : NULL;
        size_t a = 0, alen = abbrev ? abbrev->len : 0;
        bool patched = false;
        if (ab && at) { // first abbrev: code, tag, children, (attr, form)* 0 0
            while (a < alen && ab[a] & 0x80) a++; // code
            a++;
            while (a < alen && ab[a] & 0x80) a++; // tag
            a += 2; // + children flag
            while (a + 1 < alen && (ab[a] || ab[a + 1])) {
                if (ab[a] == DW_AT_decl_line && ab[a + 1] == DW_FORM_data4) {
                    ab[a] = DW_AT_macro_info;
                    ab[a + 1] = DW_FORM_sec_offset; // both forms are 4 bytes
                    patched = true;
                    break;
                }
                a += 2;
            }
        }
        if (!patched) {
            g.failed = true;
            g.what = "locating the CU macro_info placeholder";
        } else {
            memset((void *)(info->bytes + at), 0, 4);
            if (nrel == caprel) {
                caprel = caprel ? caprel * 2 : 64;
                rels = realloc(rels, caprel * sizeof *rels);
            }
            rels[nrel++] = (PendRel){info->sec, at, sec_anchor(".debug_macinfo"), R_X86_64_32, 0};
        }
    }

    // Relocations: translate once, commit only if everything validated.
    Dwarf_Unsigned nrelsecs = 0;
    int drd_version = 0;
    D(dwarf_get_relocation_info_count(g.dbg, &nrelsecs, &drd_version, &g.err));
    for (Dwarf_Unsigned s = 0; s < nrelsecs && !g.failed; s++) {
        Dwarf_Unsigned relsec = 0, applies_to = 0, n = 0;
        Dwarf_Relocation_Data rd = NULL;
        D(dwarf_get_relocation_info(g.dbg, &relsec, &applies_to, &n, &rd, &g.err));
        if (g.failed) break;
        OutSec *os = NULL;
        for (int i = 0; i < nout; i++)
            if (out[i].idx == applies_to) os = &out[i];
        if (!os) {
            g.failed = true;
            g.what = "relocation for unknown section";
            break;
        }
        for (Dwarf_Unsigned k = 0; k < n; k++) {
            Dwarf_Unsigned si = rd[k].drd_symbol_index;
            if (rd[k].drd_type != dwarf_drt_data_reloc || si < SYMBASE ||
                si - SYMBASE >= (Dwarf_Unsigned)obj->sym_count) {
                g.failed = true;
                g.what = "unsupported relocation";
                break;
            }
            int sym = (int)(si - SYMBASE);
            // Reference to another .debug_* section -> 4-byte section
            // offset; address of code/data -> 8-byte absolute.
            bool sec_ref = obj->syms[sym].type == ST_SECTION && obj->syms[sym].section >= SEC_NUM;
            size_t w = sec_ref ? 4 : 8;
            if (rd[k].drd_offset > os->len || os->len - rd[k].drd_offset < w) {
                g.failed = true;
                g.what = "relocation offset outside section";
                break;
            }
            uint64_t add = 0;
            memcpy(&add, os->bytes + rd[k].drd_offset, w); // little endian host (x86-64)
            if (nrel == caprel) {
                caprel = caprel ? caprel * 2 : 64;
                rels = realloc(rels, caprel * sizeof *rels);
            }
            rels[nrel++] = (PendRel){os->sec, rd[k].drd_offset, sym,
                                     sec_ref ? R_X86_64_32 : R_X86_64_64, (int64_t)add};
        }
    }
    if (g.failed) {
        fail(g.what);
        goto done;
    }
    for (int i = 0; i < nout; i++)
        secbuf_emitbuf(objfile_section_buf(obj, out[i].sec), out[i].bytes, out[i].len);
    if (g.maclen)
        secbuf_emitbuf(objfile_section_buf(obj, objfile_find_or_add_section(obj, ".debug_macinfo", 0, 0)),
                       g.mac, g.maclen);
    for (size_t i = 0; i < nrel; i++)
        objfile_add_reloc(obj, rels[i].sec, rels[i].off, rels[i].sym, rels[i].type, rels[i].addend);
    ok = true;

done:
    pp_macro_events = NULL; // consumed (or abandoned on fallback)
    for (int i = 0; i < nout; i++)
        free((void *)out[i].bytes);
    free(rels);
    free(g.tt);
    free(g.mac);
    if (g.dbg) {
        Dwarf_Error ferr = NULL;
        dwarf_producer_finish_a(g.dbg, &ferr);
    }
    return ok;
}

#endif // RCC_DWARF3
