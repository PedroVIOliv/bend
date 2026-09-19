/**
 * Vulkan Compute GLSL 460 Shader Generator for Bend 2.0.
 *
 * Implements a clean sibling emitter consuming the compiler's internal
 * segment representation (`CarbOutput` from `bend2/comp.ts`) rather than
 * scraping emitted C text with regexes.
 *
 * Emits a self-contained GLSL 460 compute shader using `GL_EXT_buffer_reference`
 * and 64-bit explicit integers to achieve 1:1 pointer parity with Bend's
 * 64-bit unified heap (Buffer Device Address).
 */

import * as Bend from "../bend.ts";
import * as Comp from "../comp.ts";

export function generateVulkanShader(input: Bend.Book | string): string {
  let carb: Comp.CarbOutput | null = null;
  let cSrc = "";

  if (typeof input === "string") {
    cSrc = input;
  } else {
    carb = Comp.compile_carb(input);
  }

  // Extract FID/CID definitions and tables
  let fidCidLines: string[] = [];
  let fidArity = "";
  let fidFlag = "";
  let fidResw = "";
  let cidArity = "";
  let resw = 1;
  let statLen = 0;
  let deviceSpins = "";
  let deviceSegs: string[] = [];

  function cleanGlsl(s: string): string {
    s = s.replace(/\be\.mem\[([^\]]+)\]\s*=\s*([^;]+);/g, "H_SET($1, $2);");
    s = s.replace(/\be\.mem\[([^\]]+)\]/g, "H_GET($1)");
    s = s.replace(/err_seen\(e\.mem\)/g, "err_seen()");
    s = s.replace(/task_deliver\(e\.mem,\s*/g, "task_deliver(");
    s = s.replace(/(\d+)ull/g, "$1ul");
    s = s.replace(/Fid\s+(\w+)\s*=\s*(?:\((?:u32|Fid)\))?\s*([^;]+);/g, "Fid $1 = Fid($2);");
    s = s.replace(/\((?:u32|Fid)\)\s*([a-zA-Z0-9_]+(?:\([^)]*\))?)/g, "uint($1)");
    s = s.replace(/\((?:u32|Fid)\)\s*\(([^)]+)\)/g, "uint($1)");
    s = s.replace(/(u32\s+\w+\s*=\s*)term_loc\(([^)]+)\)/g, "$1uint(term_loc($2))");
    s = s.replace(/(u32\s+\w+\s*=\s*)STK\(([^)]+)\)/g, "$1uint(STK($2))");
    s = s.replace(/(u32\s+\w+\s*=\s*)(r\d+);/g, "$1uint($2);");
    s = s.replace(/Term (o_\d+)\[\d+\];/g, "Term $1[4];");
    s = s.replace(/cls_fit\((\d+)\)/g, "cls_fit($1ul)");
    return s;
  }

  if (carb !== null) {
    const { fl, defs } = carb;
    const defText = defs.join("\n");
    fidCidLines = defs.filter((l) => /^\s*#define (FID_|CID_)\w+ \d+/.test(l));

    function getTable(name: string): string {
      const m = new RegExp(`CONSTV u8 ${name}\\[\\] = \\{ ([^}]+) \\};`).exec(defText);
      if (!m) throw new Error("Table not found: " + name);
      return `const uint ${name}[] = uint[](${m[1]});`;
    }

    fidArity = getTable("FID_ARITY_T");
    fidFlag = getTable("FID_FLAG_T");
    fidResw = getTable("FID_RESW_T");
    cidArity = getTable("CID_ARITY_T");

    const rm = /#define WL_RESW (\d+)/.exec(defText);
    resw = rm ? parseInt(rm[1]) : 1;
    const sm = /#define STAT_LEN (\d+)/.exec(defText);
    statLen = sm ? parseInt(sm[1]) : 0;

    // Direct segment translation from compiler's File structure
    const devSegments = fl.segs.filter((s) => !s.host);
    deviceSegs = devSegments.map((seg) => {
      const lines = [
        `  case ${seg.fid}: {`,
        ...Comp.seg_take(seg).map((l) => "    " + cleanGlsl(l)),
        `    WL_OPEN`,
        ...(seg.spin ? ["    WL_SPIN"] : []),
        ...seg.lines.map((l) => cleanGlsl(l)),
        ...(seg.spin ? ["    WL_SPUN"] : []),
        `  }}`
      ];
      return lines.join("\n");
    });

    deviceSpins = fl.spins.map(([name, body]) => {
      let s = typeof body === "string" ? body : (body as any).join("\n");
      s = s.replace(/INLINE Term (spin_\d+)\(Env e, THR Term\* o, /, "Term $1(inout Env e, inout Term o[4], ");
      s = s.replace(/INLINE Term (spin_\d+)\(Env e, THR Term\* o\)/, "Term $1(inout Env e, inout Term o[4])");
      return cleanGlsl(s);
    }).join("\n\n");
  } else {
    // Fallback if raw C source was provided
    fidCidLines = cSrc.split("\n").filter((l) => /^\s*#define (FID_|CID_)\w+ \d+/.test(l));
    function extractTable(name: string): string {
      const m = new RegExp(`CONSTV u8 ${name}\\[\\] = \\{ ([^}]+) \\};`).exec(cSrc);
      if (!m) throw new Error("Table not found: " + name);
      return `const uint ${name}[] = uint[](${m[1]});`;
    }

    fidArity = extractTable("FID_ARITY_T");
    fidFlag = extractTable("FID_FLAG_T");
    fidResw = extractTable("FID_RESW_T");
    cidArity = extractTable("CID_ARITY_T");

    const reswMatch = /#define WL_RESW (\d+)/.exec(cSrc);
    resw = reswMatch ? parseInt(reswMatch[1]) : 1;

    const spinMatches = [...cSrc.matchAll(/INLINE Term (spin_\d+)\([^)]+\) \{[\s\S]*?\n\}/g)];
    deviceSpins = spinMatches.map((m) => {
      let s = m[0];
      s = s.replace(/INLINE Term (spin_\d+)\(Env e, THR Term\* o, /, "Term $1(inout Env e, inout Term o[4], ");
      s = s.replace(/INLINE Term (spin_\d+)\(Env e, THR Term\* o\)/, "Term $1(inout Env e, inout Term o[4])");
      return cleanGlsl(s);
    }).join("\n\n");

    const lines = cSrc.split("\n");
    let isHost = false;
    let inSegment = false;
    let currentSeg: string[] = [];

    for (let i = 0; i < lines.length; i++) {
      const l = lines[i];
      if (/^#if !DEVICE/.test(l)) {
        isHost = true;
        continue;
      }
      if (isHost && /^#endif/.test(l)) {
        isHost = false;
        continue;
      }
      if (!isHost) {
        if (/^\s*WL_CASE\(/.test(l)) {
          inSegment = true;
          currentSeg = [l];
        } else if (inSegment) {
          currentSeg.push(l);
          if (/^\s*\}\}/.test(l)) {
            inSegment = false;
            deviceSegs.push(currentSeg.join("\n"));
            currentSeg = [];
          }
        }
      }
    }
  }

  const glslSegs = deviceSegs.map((seg) => {
    let s = seg;
    s = s.replace(/WL_CASE\(([^)]+)\)/, "case $1:");
    return cleanGlsl(s);
  }).join("\n\n");

  return `#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_control_flow_attributes : enable

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

// Note: coherent qualifier ensures cross-workgroup memory visibility across all GPU threads
layout(buffer_reference, std430, buffer_reference_align = 8) coherent buffer CorpusRef {
    uint64_t words[];
};

layout(buffer_reference, std430, buffer_reference_align = 4) coherent buffer U32Ref {
    uint uwords[];
};

layout(push_constant) uniform PushConstants {
    uint64_t H;
    uint pass;
    uint grids;
} push;

shared uint64_t hold[2304];
shared uint tg_cur;
shared uint tg_grew;
shared uint tg_has;

#define u64 uint64_t
#define u32 uint64_t
#define uint32_t uint64_t
#define Loc uint64_t
#define Term uint64_t
#define Fid uint
#define Cls uint
#define Err uint
#define Ring uint
#define Stk int64_t
#define Reply uint64_t
#define U32_BIN(a, o, b) (uint64_t(uint(a) o uint(b)))

#define LINE      16ul
#define PAGE_BITS 7ul
#define PAGE_LEN  (1ul << PAGE_BITS)
#define CUBE_T    128ul
#define CUBE      (CUBE_T * CUBE_T)
#define CUBE_LOG  7u
#define CUBE_G    (1u << CUBE_LOG)
#define LANES     (CUBE_T << CUBE_LOG)
#define RING_LOG  (17u - CUBE_LOG)
#define RING_LEN  (1ul << RING_LOG)
#define STAK_LEN  (1ul << 11)
#define NCLS      8u
#define NCLS_ALL  32u

#define ALC_WORDS NCLS_ALL
#define TG_HOLD   2304u
#define CHUNK     256ul
#define QUANTUM   PAGE_LEN
#define KEEP_WORDS CHUNK
#define RING_WORDS ((1ul << 10) + 2ul)

#define H_BUMP       0ul
#define H_CAP        1ul
#define H_CURSOR     LINE
#define H_ROOT_DONE  (2ul * LINE)
#define H_ERROR_CODE (3ul * LINE)
#define H_ROOT_WORD  (4ul * LINE)
#define H_BANK       (H_ROOT_WORD + ${resw}ul)
#define WL_RESW      ${resw}u

#define PAGE_UP(n) (((n) + PAGE_LEN - 1ul) & ~(PAGE_LEN - 1ul))
#define ALC_OFF  PAGE_UP(H_BANK + 3ul * NCLS_ALL)
#define RING_OFF (ALC_OFF + CUBE * 2ul * ALC_WORDS)
#define STAK_OFF (RING_OFF + CUBE * RING_WORDS)
#define STAT_LEN ${statLen}ul
#define STAT_OFF (STAK_OFF + CUBE * STAK_LEN)
#define HEAP_OFF (STAT_OFF + PAGE_UP(STAT_LEN))

#define TAG_PAK 1ul
#define TAG_CTR 2ul
#define TAG_CLO 3ul
#define TAG_BUF 4ul
#define TAG_TSK 5ul
#define TAG_ARR 6ul

#define TERM_HOLE (~0ul)
#define RFC_BIT  (1ul << 63)
#define RFC_CNT  ((1u << 24) - 1u)
#define LOC_MASK ((1ul << 40) - 1ul)

#define ERR_RING 1u
#define ERR_TAGS 2u
#define ERR_HEAP 3u
#define ERR_FIDS 4u
#define ERR_NATS 5u
#define ERR_RFCS 6u
#define ERR_DEEP 7u
#define ERR_ARRS 8u

#define CLZ(x) uint(31 - findMSB(x))
#define BAR() { groupMemoryBarrier(); barrier(); }
#define FENCE() memoryBarrierBuffer()
#define BARD() { memoryBarrierBuffer(); groupMemoryBarrier(); barrier(); }

#define W64_TO_U32(w) (uint((w) * 2ul))

#define H_GET(loc) CorpusRef(push.H).words[uint(loc)]
void h_set(uint64_t loc, uint64_t val) {
    if (loc < 16ul) return;
    CorpusRef(push.H).words[uint(loc)] = val;
}
#define H_SET(loc, val) h_set(uint64_t(loc), uint64_t(val))

#define LANE_STEP int64_t(CUBE)
#define STK(I) CorpusRef(push.H).words[uint(sp + int64_t(I) * LANE_STEP)]

#define LOCK(l)
#define UNLOCK(l)
#define WL_OPEN {
#define WL_JMP(F) { fid = Fid(F); break; }
#define WL_DYN WL_JMP
#define WL_POP() { sp -= LANE_STEP; WL_DYN(Fid(STK(0))); }
#define WL_RETN(N) { rn = (N); WL_POP(); }
#define WL_CONT STK(-3)
#define WL_IDX STK(-2)
#define WL_POPN(N) sp -= int64_t(N) * LANE_STEP
#define WL_PUSHN(N) sp += int64_t(N) * LANE_STEP
#define WL_FRAME(T) \\
  Loc wtl = task_tail(T); \\
  u64 wtw = H_GET(wtl + 1ul); \\
  STK(0) = H_GET(wtl); \\
  STK(1) = (wtw >> 32) & 0xFFFFul; \\
  STK(2) = uint64_t(FID_EXIT); \\
  sp += 3L * LANE_STEP;
#define WL_ARGS(A, N) \\
  for (u32 wi = 0u; wi + 1u < (N); wi += 1u) { \\
    STK(wi) = H_GET((A) + uint64_t(wi)); \\
  } \\
  sp += int64_t((N) - 1u) * LANE_STEP;
#define WL_ROOM(N)
#define WL_AGAIN(F) continue
#define WL_SPIN for (;;) { if (err_spun(wpoll)) { return 0ul; }
#define WL_SPUN } break;

${fidCidLines.join("\n")}

${fidArity}
${fidFlag}
${fidResw}
${cidArity}

#define fid_arity(x) FID_ARITY_T[uint(x)]
#define fid_bangs(x) ((FID_FLAG_T[uint(x)] & 1u) != 0u)
#define fid_nofk(x)  ((FID_FLAG_T[uint(x)] & 2u) != 0u)
#define fid_seqk(x)  (fid_resw(x) != 0u)
#define fid_resw(x)  FID_RESW_T[uint(x)]
#define cid_arity(x) CID_ARITY_T[uint(x)]

uint a32_load(uint64_t u32_idx) { return U32Ref(push.H).uwords[uint(u32_idx)]; }
void a32_store(uint64_t u32_idx, uint64_t val) { U32Ref(push.H).uwords[uint(u32_idx)] = uint(val); }
uint a32_add(uint64_t u32_idx, uint64_t val) { return atomicAdd(U32Ref(push.H).uwords[uint(u32_idx)], uint(val)); }
uint a32_sub(uint64_t u32_idx, uint64_t val) { return atomicAdd(U32Ref(push.H).uwords[uint(u32_idx)], -uint(val)); }

bool a32_cas(uint64_t u32_idx, inout uint expected, uint desired) {
    uint orig = atomicCompSwap(U32Ref(push.H).uwords[uint(u32_idx)], expected, desired);
    bool ok = (orig == expected);
    expected = orig;
    return ok;
}

void a32_store_rel(uint64_t u32_idx, uint64_t val) { FENCE(); a32_store(u32_idx, val); }
uint a32_sub_rel(uint64_t u32_idx, uint64_t val) { FENCE(); return a32_sub(u32_idx, val); }
uint a32_load_acq(uint64_t u32_idx) { uint v = a32_load(u32_idx); FENCE(); return v; }
#define a32_acq(w) FENCE()

bool err_seen() { return a32_load(W64_TO_U32(H_ERROR_CODE)) != 0u; }
bool err_spun(inout uint n) { n++; return ((n & 4095u) == 0u) && err_seen(); }
bool err_spun(inout uint64_t n) { n++; return ((n & 4095ul) == 0ul) && err_seen(); }
void err_post(uint code) {
    uint seen = 0u;
    while (seen == 0u && !a32_cas(W64_TO_U32(H_ERROR_CODE), seen, code)) {}
}

Cls cls_fit(u64 words) { return words > 1ul ? (32u - CLZ(uint(words - 1ul))) : 0u; }

Term term_make(u64 tag, u64 aux, Loc loc) { return (tag << 56) | (aux << 40) | loc; }
#define term_ctr(cid, loc) term_make(TAG_CTR, uint64_t(cid), loc)
#define term_pak(cid, loc) term_make(TAG_PAK, uint64_t(cid), loc)
#define term_clo(fid, loc) term_make(TAG_CLO, uint64_t(fid), loc)
#define term_buf(cls, loc) term_make(TAG_BUF, uint64_t(cls), loc)
#define term_tsk(fid, loc) term_make(TAG_TSK, uint64_t(fid), loc)

u64 term_tag(Term t) { return (t >> 56) & 0x7ful; }
bool term_rfc(Term t) { return (t & RFC_BIT) != 0ul; }
u64 term_aux(Term t) { return (t >> 40) & 0xFFFFul; }
Loc term_loc(Term t) { return t & LOC_MASK; }

uint64_t ring_word(uint64_t r, uint64_t w) { return RING_OFF + w * LANES + r; }
uint64_t ring_slot(uint64_t r, uint32_t p) { return ring_word(r, uint64_t(p & (uint32_t(RING_LEN) - 1u))); }
uint64_t ring_get_word(uint64_t r) { return ring_word(r, RING_LEN); }
uint64_t ring_put_word(uint64_t r) { return ring_word(r, RING_LEN + 1ul); }
uint ring_lap(uint pos) { return ~uint(pos / uint(RING_LEN)) & 1u; }

void ring_push(uint64_t r, Term tsk) {
    uint put_idx = W64_TO_U32(ring_put_word(r));
    uint get_idx = W64_TO_U32(ring_get_word(r));
    uint pos = a32_add(put_idx, 1u);
    if (pos - a32_load(get_idx) >= uint(RING_LEN)) {
        err_post(ERR_RING);
        return;
    }
    uint64_t slot = ring_slot(r, pos);
    uint slot_u32 = W64_TO_U32(slot);
    a32_store(slot_u32, uint(tsk));
    a32_store_rel(slot_u32 + 1u, uint(tsk >> 32) | (ring_lap(pos) << 31));
}

uint ring_flip(uint i) {
    return (i % uint(CUBE_T) << CUBE_LOG) + i / uint(CUBE_T);
}

struct Env { uint64_t alc; };

#define ALC_AT(e, i) CorpusRef(push.H).words[uint((e).alc + uint64_t(i) * CUBE)]
#define ALC_LEN(e, c) ALC_AT(e, ALC_WORDS + (c))
#define KEEP(c) ((KEEP_WORDS >> (c)) != 0ul ? (KEEP_WORDS >> (c)) : 1ul)

uint64_t bank_word(Cls c) { return H_BANK + uint64_t(c) * 3ul; }

Loc bank_pop(Cls c) {
    uint64_t b = bank_word(c);
    uint b32 = W64_TO_U32(b);
    Loc got = 0ul;
    uint t = a32_sub(b32 + 2u, 1u);
    if (int(t) > 0) {
        got = H_GET(H_GET(b) + uint64_t(t - 1u));
        if (got < HEAP_OFF) got = 0ul;
    } else {
        a32_add(b32 + 2u, 1u);
    }
    return got;
}

void bank_push(Cls c, Loc head) {
    if (head < HEAP_OFF) return;
    uint64_t b = bank_word(c);
    uint b32 = W64_TO_U32(b);
    uint idx = a32_add(b32 + 3u, 1u);
    H_SET(H_GET(b) + uint64_t(idx), head);
}

Loc heap_alloc_miss(inout Env e, Cls cls) {
    Loc got = bank_pop(cls);
    uint n = (got != 0ul) ? uint(KEEP(cls)) : ((cls < NCLS) ? (uint(QUANTUM) >> cls) : 1u);
    if (got == 0ul) {
        uint pages = (n << cls) >> uint(PAGE_BITS);
        uint p = a32_add(W64_TO_U32(H_BUMP), pages);
        uint cap_val = a32_load(W64_TO_U32(H_CAP));
        if (uint64_t(p) + uint64_t(pages) > uint64_t(cap_val)) {
            H_SET(H_ERROR_CODE + 1ul, uint64_t(p));
            H_SET(H_ERROR_CODE + 2ul, uint64_t(pages));
            H_SET(H_ERROR_CODE + 3ul, uint64_t(cap_val));
            err_post(ERR_HEAP);
            return 0ul;
        }
        got = HEAP_OFF + (uint64_t(p) << PAGE_BITS);
        for (uint i = 1u; i <= n; i += 1u) {
            H_SET(got + (uint64_t(i - 1u) << cls), (i < n) ? (got + (uint64_t(i) << cls)) : 0ul);
        }
    }
    ALC_AT(e, cls) = H_GET(got);
    ALC_LEN(e, cls) = uint64_t(n - 1u) << cls;
    return got;
}

Loc heap_alloc(inout Env e, Cls cls) {
    Loc h = ALC_AT(e, cls);
    if (h >= HEAP_OFF) {
        ALC_AT(e, cls) = H_GET(h);
        if (ALC_LEN(e, cls) >= (1ul << cls)) {
            ALC_LEN(e, cls) -= (1ul << cls);
        } else {
            ALC_LEN(e, cls) = 0ul;
        }
        return h;
    }
    ALC_AT(e, cls) = 0ul;
    ALC_LEN(e, cls) = 0ul;
    return heap_alloc_miss(e, cls);
}

void heap_free(inout Env e, Cls cls, Loc loc) {
    if (err_seen() || loc < HEAP_OFF) return;
    H_SET(loc, ALC_AT(e, cls));
    ALC_AT(e, cls) = loc;
    ALC_LEN(e, cls) += (1ul << cls);
}

Loc task_tail(Term t) {
    return term_loc(t) + uint64_t(fid_arity(uint(term_aux(t))));
}

Loc task_node(inout Env e, uint64_t fid, uint64_t cont, uint64_t idx, uint64_t rem) {
    uint ar = fid_arity(uint(fid));
    Loc loc = heap_alloc(e, cls_fit(uint64_t(ar + 2u)));
    if (loc < HEAP_OFF) { err_post(ERR_HEAP); return 0ul; }
    for (uint i = 0u; rem != 0ul && i < ar; i += 1u) {
        H_SET(loc + uint64_t(i), TERM_HOLE);
    }
    H_SET(loc + uint64_t(ar), cont);
    H_SET(loc + uint64_t(ar) + 1ul, ((idx & 0xFFFFul) << 32) | (rem & 0xFFFFFFFFul));
    return loc;
}

Term task_deliver(Term cont, uint64_t idx, inout Term v[WL_RESW], uint64_t n) {
    Loc at = (cont == TERM_HOLE) ? H_ROOT_WORD : (term_loc(cont) + idx);
    if (cont != TERM_HOLE && at < HEAP_OFF) {
        err_post(ERR_HEAP);
        return 0ul;
    }
    for (uint j = 0u; uint64_t(j) < uint64_t(WL_RESW); j += 1u) {
        if (uint64_t(j) < n) {
            H_SET(at + uint64_t(j), v[j]);
        }
    }
    if (cont == TERM_HOLE) {
        a32_store_rel(W64_TO_U32(H_ROOT_DONE), uint(n + 1ul));
        return 0ul;
    }
    Loc tl = task_tail(cont);
    if (tl < HEAP_OFF) {
        err_post(ERR_HEAP);
        return 0ul;
    }
    // Coherent buffer references + memoryBarrierBuffer guarantee all stores in v[]
    // are globally visible to the thread that decrements rem to 0
    if (a32_sub_rel(W64_TO_U32(tl + 1ul), 1u) == 1u) {
        a32_acq(W64_TO_U32(tl + 1ul));
        return cont;
    }
    return 0ul;
}

void task_deal(Term join, uint32_t base, uint32_t stride) {
    Loc loc = term_loc(join);
    if (loc < HEAP_OFF) return;
    uint ar = fid_arity(uint(term_aux(join)));
    uint g = 0u;
    if (stride == 0u) {
        uint rem = uint(H_GET(loc + uint64_t(ar) + 1ul));
        g = a32_add(W64_TO_U32(H_CURSOR), rem);
    }
    for (uint i = 0u; i < ar; i += 1u) {
        Term k = H_GET(loc + uint64_t(i));
        if (term_tag(k) == TAG_TSK) {
            H_SET(loc + uint64_t(i), TERM_HOLE);
            uint64_t to;
            if (stride != 0ul) {
                to = uint64_t(base) + uint64_t(stride) * uint64_t(atomicAdd(tg_cur, 1u) & (uint(CUBE_T) - 1u));
            } else {
                to = uint64_t(ring_flip(uint(g) & (uint(LANES) - 1u)));
                g += 1u;
            }
            ring_push(to, k);
        }
    }
}

void dev_cut(inout Env e) {
    if (err_seen()) return;
    for (Cls c = 0u; c < NCLS_ALL; c += 1u) {
        uint64_t gen = KEEP(c) << c;
        while (ALC_LEN(e, c) >= gen) {
            Loc head = ALC_AT(e, c);
            if (head < HEAP_OFF) { ALC_AT(e, c) = 0ul; ALC_LEN(e, c) = 0ul; break; }
            Loc tail = head;
            for (uint32_t i = uint32_t(KEEP(c)); --i != 0u;) {
                tail = H_GET(tail);
                if (tail < HEAP_OFF) break;
            }
            if (tail < HEAP_OFF) { ALC_AT(e, c) = 0ul; ALC_LEN(e, c) = 0ul; break; }
            ALC_AT(e, c) = H_GET(tail);
            ALC_LEN(e, c) -= gen;
            H_SET(tail, 0ul);
            bank_push(c, head);
        }
    }
}

void bank_pack(uint lane) {
    for (Cls c = 0u; c < NCLS_ALL; c += 1u) {
        uint64_t b = bank_word(c);
        uint b32 = W64_TO_U32(b);
        uint rd = a32_load(b32 + 2u);
        uint wr = a32_load(b32 + 3u);
        uint top = a32_load(b32 + 4u);
        uint n = wr - top;
        uint64_t off = H_GET(b);
        for (uint i = 0u; i < n; i += uint(CUBE_T)) {
            Term v = (i + lane < n) ? H_GET(off + uint64_t(top + i + lane)) : 0ul;
            BARD();
            if (i + lane < n) {
                H_SET(off + uint64_t(rd + i + lane), v);
            }
        }
        BARD();
        if (lane == 0u) {
            a32_store(b32 + 2u, rd + n);
            a32_store(b32 + 3u, rd + n);
            a32_store(b32 + 4u, rd + n);
        }
    }
}

bool term_triv(Term t) {
    return term_tag(t) <= TAG_PAK || t == TERM_HOLE || term_loc(t) < HEAP_OFF;
}

Cls blk_cls(Term t) { return uint(term_aux(t)) & 31u; }
Cls blk_span(Term t) {
    Cls c = blk_cls(t);
    return term_tag(t) == TAG_ARR ? c : ((c == 0u) ? 0u : (c - 1u));
}
void blk_free(inout Env e, Term t) {
    heap_free(e, blk_span(t), term_loc(t));
}

void term_drop(inout Env e, Term t) {
    uint64_t cur = 0ul;
    Term c0 = 0ul;
    uint step = 0u;
    for (;;) {
        if (!term_triv(t) && term_rfc(t)) {
            Loc r = term_loc(t);
            if ((a32_sub_rel(r * 2ul, 1u) & RFC_CNT) != 1u) {
                t = 0ul;
            } else {
                a32_acq(r * 2ul);
                t = (t & ~(RFC_BIT | LOC_MASK)) | (H_GET(r) >> 24);
                heap_free(e, 0u, r);
            }
        }
        if (!term_triv(t)) {
            uint64_t tag = term_tag(t);
            if (tag == TAG_BUF) {
                blk_free(e, t);
            } else {
                uint aux = uint(term_aux(t));
                Loc loc = term_loc(t);
                uint n = 0u;
                Cls cls;
                if (tag == TAG_ARR) {
                    cls = 64u | blk_cls(t);
                } else {
                    uint ar;
                    if (tag == TAG_CTR) {
                        ar = cid_arity(aux);
                    } else if (tag == TAG_CLO) {
                        ar = fid_arity(aux) - 1u;
                    } else {
                        ar = fid_arity(aux);
                    }
                    n = ar;
                    cls = cls_fit(tag == TAG_TSK ? (ar + 2u) : ar);
                }
                c0 = H_GET(loc);
                H_SET(loc, cur);
                cur = loc | (uint64_t(n) << 48) | (uint64_t(cls) << 56);
            }
        }
        for (;;) {
            if (err_spun(step)) return;
            if (cur == 0ul) return;
            Loc loc = cur & LOC_MASK;
            uint i = uint((cur >> 40) & 0xFFul);
            uint n = uint((cur >> 48) & 0xFFul);
            Cls cls = uint(cur >> 56);
            bool arr = cls > 63u;
            uint j = i;
            if (arr) {
                cls &= 63u;
                n = 1u << cls;
                if (i == 2u) {
                    j = uint(H_GET(loc + 1ul));
                }
            }
            if (j < n) {
                Term c = (j == 0u) ? c0 : H_GET(loc + uint64_t(j));
                if (arr && j > 0u) {
                    H_SET(loc + 1ul, uint64_t(j + 1u));
                }
                if (!arr || i < 2u) {
                    cur += (1ul << 40);
                }
                if (!term_triv(c)) {
                    t = c;
                    break;
                }
            } else {
                uint64_t up = H_GET(loc);
                heap_free(e, cls, loc);
                cur = up;
            }
        }
    }
}

void term_sink(inout Env e, Term t) {
    if (!term_triv(t)) {
        term_drop(e, t);
    }
}

// Work loop definition
#define WL_LOAD(A, N) \\
  do { \\
    if ((N) <= 0u) break; r0 = H_GET((A) + 0ul); \\
    if ((N) <= 1u) break; r1 = H_GET((A) + 1ul); \\
    if ((N) <= 2u) break; r2 = H_GET((A) + 2ul); \\
    if ((N) <= 3u) break; r3 = H_GET((A) + 3ul); \\
    if ((N) <= 4u) break; r4 = H_GET((A) + 4ul); \\
    if ((N) <= 5u) break; r5 = H_GET((A) + 5ul); \\
    if ((N) <= 6u) break; r6 = H_GET((A) + 6ul); \\
    if ((N) <= 7u) break; r7 = H_GET((A) + 7ul); \\
  } while (false);

#define WL_LAST(X) \\
  switch (uint(war)) { \\
    case 0u: r0 = (X); break; \\
    case 1u: r1 = (X); break; \\
    case 2u: r2 = (X); break; \\
    case 3u: r3 = (X); break; \\
    case 4u: r4 = (X); break; \\
    case 5u: r5 = (X); break; \\
    case 6u: r6 = (X); break; \\
    case 7u: r7 = (X); break; \\
  }

#define WL_SAVE(V) { (V)[0] = r0; (V)[1] = r1; (V)[2] = r2; (V)[3] = r3; }
#define WL_TAKE(V) { r0 = (V)[0]; r1 = (V)[1]; r2 = (V)[2]; r3 = (V)[3]; }

${deviceSpins}

Reply work_loop(inout Env e, Stk sp, Term t, bool seq) {
    Term r0 = t;
    Term r1 = 0ul;
    Term r2 = 0ul;
    Term r3 = 0ul;
    Term r4 = 0ul;
    Term r5 = 0ul;
    Term r6 = 0ul;
    Term r7 = 0ul;
    u32 rn = 0u;
    Fid fid = FID_ENTER;
    u32 wpoll = 0u;

    for (;;) {
        if (err_spun(wpoll)) {
            return 0ul;
        }
        switch (fid) {
            case FID_ENTER: {
                Term t = r0;
                WL_OPEN
                Fid f   = Fid(term_aux(t));
                Loc a   = term_loc(t);
                u32 war = fid_arity(uint(f));
                WL_FRAME(t)
                if (fid_seqk(uint(f))) {
                    u32 rw = fid_resw(uint(f));
                    WL_LOAD(a + uint64_t(war - rw), rw)
                    WL_ARGS(a, war - rw + 1u)
                } else {
                    WL_LOAD(a, war)
                }
                heap_free(e, cls_fit(uint64_t(war + 2u)), a);
                WL_DYN(f);
            }}

            case FID_IO_EMIT: {
                Term x = r0;
                WL_OPEN
                Loc l = heap_alloc(e, 0u);
                H_SET(l, x);
                r0 = term_ctr(CID_EMIT, l);
                WL_RETN(1u);
            }}

            case FID_CLO_APPLY: {
                Term fun = r0;
                Term arg = r1;
                WL_OPEN
                Fid f    = Fid(term_aux(fun));
                u32 war  = fid_arity(uint(f)) - 1u;
                Loc a    = term_loc(fun);
                WL_LOAD(a, war)
                heap_free(e, cls_fit(uint64_t(war)), a);
                WL_LAST(arg)
                WL_DYN(f);
            }}

            case FID_EXIT: {
                u32  n = rn;
                Term rv[WL_RESW];
                WL_SAVE(rv)
                WL_OPEN
                if (err_seen()) {
                    return 0ul;
                }
                sp -= 2L * LANE_STEP;
                Term cont = STK(0);
                u32  idx  = uint(STK(1));
                if (cont != TERM_HOLE && fid_seqk(uint(term_aux(cont)))) {
                    Fid wf = Fid(term_aux(cont));
                    Loc wa = term_loc(cont);
                    u32 wn = fid_arity(uint(wf));
                    WL_FRAME(cont)
                    WL_ARGS(wa, wn - n + 1u)
                    heap_free(e, cls_fit(uint64_t(wn + 2u)), wa);
                    WL_TAKE(rv)
                    WL_DYN(wf);
                }
                return task_deliver(cont, idx, rv, n);
            }}

${glslSegs}
            default: {
                err_post(ERR_FIDS);
                return 0ul;
            }
        }
    }
}

u32 monk_step(inout Env e, Stk stk, Ring rg, u32 put0, bool seq, u32 base, u32 stride) {
    uint get_u32 = W64_TO_U32(ring_get_word(uint64_t(rg)));
    uint get_val = a32_load(get_u32);
    if (get_val == put0) {
        return 0u;
    }
    uint64_t slot = ring_slot(uint64_t(rg), get_val);
    uint slot_u32 = W64_TO_U32(slot);
    u32 hi = a32_load_acq(slot_u32 + 1u);
    Term t = (((uint64_t(hi)) << 32) | uint64_t(a32_load(slot_u32))) & ~RFC_BIT;
    if ((hi >> 31) != ring_lap(get_val)) {
        return 0u;
    }
    if (!seq && fid_nofk(uint(term_aux(t)))) {
        return 0u;
    }
    a32_store(get_u32, get_val + 1u);
    u32 spin = 0u;
    for (;;) {
        Reply r = work_loop(e, stk, t, seq);
        if (r == 0ul) {
            return 2u;
        }
        if (uint(H_GET(task_tail(r) + 1ul)) == 0u) {
            if (err_spun(spin)) {
                return 2u;
            }
            if (stride != 0u) {
                u32 to = base + stride * (atomicAdd(tg_cur, 1u) & (uint(CUBE_T) - 1u));
                ring_push(uint64_t(to), r);
                return 2u;
            }
            t = r;
            seq = (stride == 0u);
            continue;
        }
        task_deal(r, uint(base), uint(stride));
        return 1u;
    }
}

bool root_done() {
    return a32_load_acq(W64_TO_U32(H_ROOT_DONE)) != 0u;
}

void main() {
    uint lane = gl_LocalInvocationIndex;
    uint row = gl_WorkGroupID.x;
    uint grids = push.grids;
    uint pass = push.pass;

    if (pass == 2u) {
        bank_pack(lane);
        return;
    }
    uint stride = (grids == 1u) ? CUBE_G : 1u;
    uint me = row * uint(CUBE_T) + stride * lane;
    Ring rg = (pass != 0u) ? ring_flip(me) : me;
    Env e;
    e.alc = ALC_OFF + uint64_t(me);
    Stk stk = int64_t(STAK_OFF + uint64_t(me));
    if (lane == 0u) {
        hold[0] = 0ul;
        tg_cur = 0u;
        tg_grew = 0u;
        tg_has = 0u;
    }
    BAR();
    uint put_idx = W64_TO_U32(ring_put_word(uint64_t(rg)));
    uint get_idx = W64_TO_U32(ring_get_word(uint64_t(rg)));
    uint put0 = a32_load(put_idx);
    uint seen_has = 0u;
    uint seen_grew = 0u;
    for (uint turn = 0u; pass != 0u || turn < uint(CUBE_T); turn += 1u) {
        if (pass != 0u) {
            if (a32_load(get_idx) == put0 || err_seen()) {
                break;
            }
        } else {
            put0 = a32_load(put_idx);
            uint vote = (put0 != a32_load(get_idx)) ? 1u : 0u;
            if (lane == 0u && (err_seen() || root_done())) {
                vote = uint(CUBE_T);
            }
            atomicAdd(tg_has, vote);
            BAR();
            uint has = tg_has;
            if (has - seen_has >= uint(CUBE_T)) {
                break;
            }
            seen_has = has;
        }
        u32 ran = monk_step(e, stk, rg, put0, pass != 0u, (pass != 0u) ? rg : (row * uint(CUBE_T)), (pass != 0u) ? 0u : stride);
        if (pass == 0u) {
            if (ran == 1u) {
                atomicAdd(tg_grew, 1u);
            }
            BARD();
            uint grew = tg_grew;
            if (grew == seen_grew) {
                break;
            }
            seen_grew = grew;
        }
    }
    dev_cut(e);
}
`;
}
