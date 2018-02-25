-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- NASM Assembly LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'asm'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, ';' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer * S('hqb')^-1)

-- Preprocessor.
local preproc_word = word_match{
  'arg', 'assign', 'clear', 'define', 'defstr', 'deftok', 'depend', 'elif',
  'elifctx', 'elifdef', 'elifempty', 'elifenv', 'elifid', 'elifidn', 'elifidni',
  'elifmacro', 'elifn', 'elifnctx', 'elifndef', 'elifnempty', 'elifnenv',
  'elifnid', 'elifnidn', 'elifnidni', 'elifnmacro', 'elifnnum', 'elifnstr',
  'elifntoken', 'elifnum', 'elifstr', 'eliftoken', 'else', 'endif', 'endmacro',
  'endrep', 'endwhile', 'error', 'exitmacro', 'exitrep', 'exitwhile', 'fatal',
  'final', 'idefine', 'idefstr', 'ideftok', 'if', 'ifctx', 'ifdef', 'ifempty',
  'ifenv', 'ifid', 'ifidn', 'ifidni', 'ifmacro', 'ifn', 'ifnctx', 'ifndef',
  'ifnempty', 'ifnenv', 'ifnid', 'ifnidn', 'ifnidni', 'ifnmacro', 'ifnnum',
  'ifnstr', 'ifntoken', 'ifnum', 'ifstr', 'iftoken', 'imacro', 'include',
  'ixdefine', 'line', 'local', 'macro', 'pathsearch', 'pop', 'push', 'rep',
  'repl', 'rmacro', 'rotate', 'stacksize', 'strcat', 'strlen', 'substr',
  'undef', 'unmacro', 'use', 'warning', 'while', 'xdefine',
}
local preproc_symbol = '??' + S('!$+?') + '%' * -l.space + R('09')^1
local preproc = token(l.PREPROCESSOR, '%' * (preproc_word + preproc_symbol))

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  -- Preprocessor macros.
  'struc', 'endstruc', 'istruc', 'at', 'iend', 'align', 'alignb', 'sectalign',
  '.nolist',
  -- Preprocessor Packages.
  --'altreg', 'smartalign', 'fp', 'ifunc'
  -- Directives.
  'absolute', 'bits', 'class', 'common', 'common', 'cpu', 'default', 'export',
  'extern', 'float', 'global', 'group', 'import', 'osabi', 'overlay', 'private',
  'public', '__SECT__', 'section', 'segment', 'stack', 'use16', 'use32',
  'use64',
  -- Section Names.
  '.bss', '.comment', '.data', '.lbss', '.ldata', '.lrodata', '.rdata',
  '.rodata', '.tbss', '.tdata', '.text',
  -- Section Qualifiers.
  'alloc', 'bss', 'code', 'exec', 'data', 'noalloc', 'nobits', 'noexec',
  'nowrite', 'progbits', 'rdata', 'tls', 'write',
  -- Operators.
  'abs', 'rel', 'seg', 'wrt', 'strict',
  '__utf16__', '__utf16be__', '__utf16le__', '__utf32__', '__utf32be__',
  '__utf32le__',
}, '.'))

-- Instructions.
-- awk '{print $1}'|uniq|tr '[:upper:]' '[:lower:]'|
-- lua -e "for l in io.lines() do print(\"'\"..l..\"',\") end"|fmt -w 78
local instruction = token('instruction', word_match{
  -- Special Instructions.
  'db', 'dd', 'do', 'dq', 'dt', 'dw', 'dy', 'resb', 'resd', 'reso', 'resq',
  'rest', 'resw', 'resy',
  -- Conventional Instructions.
  'aaa', 'aad', 'aam', 'aas', 'adc', 'add', 'and', 'arpl', 'bb0_reset',
  'bb1_reset', 'bound', 'bsf', 'bsr', 'bswap', 'bt', 'btc', 'btr', 'bts',
  'call', 'cbw', 'cdq', 'cdqe', 'clc', 'cld', 'cli', 'clts', 'cmc', 'cmp',
  'cmpsb', 'cmpsd', 'cmpsq', 'cmpsw', 'cmpxchg', 'cmpxchg486', 'cmpxchg8b',
  'cmpxchg16b', 'cpuid', 'cpu_read', 'cpu_write', 'cqo', 'cwd', 'cwde', 'daa',
  'das', 'dec', 'div', 'dmint', 'emms', 'enter', 'equ', 'f2xm1', 'fabs',
  'fadd', 'faddp', 'fbld', 'fbstp', 'fchs', 'fclex', 'fcmovb', 'fcmovbe',
  'fcmove', 'fcmovnb', 'fcmovnbe', 'fcmovne', 'fcmovnu', 'fcmovu', 'fcom',
  'fcomi', 'fcomip', 'fcomp', 'fcompp', 'fcos', 'fdecstp', 'fdisi', 'fdiv',
  'fdivp', 'fdivr', 'fdivrp', 'femms', 'feni', 'ffree', 'ffreep', 'fiadd',
  'ficom', 'ficomp', 'fidiv', 'fidivr', 'fild', 'fimul', 'fincstp', 'finit',
  'fist', 'fistp', 'fisttp', 'fisub', 'fisubr', 'fld', 'fld1', 'fldcw',
  'fldenv', 'fldl2e', 'fldl2t', 'fldlg2', 'fldln2', 'fldpi', 'fldz', 'fmul',
  'fmulp', 'fnclex', 'fndisi', 'fneni', 'fninit', 'fnop', 'fnsave', 'fnstcw',
  'fnstenv', 'fnstsw', 'fpatan', 'fprem', 'fprem1', 'fptan', 'frndint',
  'frstor', 'fsave', 'fscale', 'fsetpm', 'fsin', 'fsincos', 'fsqrt',
  'fst', 'fstcw', 'fstenv', 'fstp', 'fstsw', 'fsub', 'fsubp', 'fsubr',
  'fsubrp', 'ftst', 'fucom', 'fucomi', 'fucomip', 'fucomp', 'fucompp',
  'fxam', 'fxch', 'fxtract', 'fyl2x', 'fyl2xp1', 'hlt', 'ibts', 'icebp',
  'idiv', 'imul', 'in', 'inc', 'incbin', 'insb', 'insd', 'insw', 'int',
  'int01', 'int1', 'int03', 'int3', 'into', 'invd', 'invpcid', 'invlpg',
  'invlpga', 'iret', 'iretd', 'iretq', 'iretw', 'jcxz', 'jecxz', 'jrcxz',
  'jmp', 'jmpe', 'lahf', 'lar', 'lds', 'lea', 'leave', 'les', 'lfence',
  'lfs', 'lgdt', 'lgs', 'lidt', 'lldt', 'lmsw', 'loadall', 'loadall286',
  'lodsb', 'lodsd', 'lodsq', 'lodsw', 'loop', 'loope', 'loopne', 'loopnz',
  'loopz', 'lsl', 'lss', 'ltr', 'mfence', 'monitor', 'mov', 'movd', 'movq',
  'movsb', 'movsd', 'movsq', 'movsw', 'movsx', 'movsxd', 'movsx', 'movzx',
  'mul', 'mwait', 'neg', 'nop', 'not', 'or', 'out', 'outsb', 'outsd', 'outsw',
  'packssdw', 'packsswb', 'packuswb', 'paddb', 'paddd', 'paddsb', 'paddsiw',
  'paddsw', 'paddusb', 'paddusw', 'paddw', 'pand', 'pandn', 'pause', 'paveb',
  'pavgusb', 'pcmpeqb', 'pcmpeqd', 'pcmpeqw', 'pcmpgtb', 'pcmpgtd', 'pcmpgtw',
  'pdistib', 'pf2id', 'pfacc', 'pfadd', 'pfcmpeq', 'pfcmpge', 'pfcmpgt',
  'pfmax', 'pfmin', 'pfmul', 'pfrcp', 'pfrcpit1', 'pfrcpit2', 'pfrsqit1',
  'pfrsqrt', 'pfsub', 'pfsubr', 'pi2fd', 'pmachriw', 'pmaddwd', 'pmagw',
  'pmulhriw', 'pmulhrwa', 'pmulhrwc', 'pmulhw', 'pmullw', 'pmvgezb', 'pmvlzb',
  'pmvnzb', 'pmvzb', 'pop', 'popa', 'popad', 'popaw', 'popf', 'popfd',
  'popfq', 'popfw', 'por', 'prefetch', 'prefetchw', 'pslld', 'psllq',
  'psllw', 'psrad', 'psraw', 'psrld', 'psrlq', 'psrlw', 'psubb', 'psubd',
  'psubsb', 'psubsiw', 'psubsw', 'psubusb', 'psubusw', 'psubw', 'punpckhbw',
  'punpckhdq', 'punpckhwd', 'punpcklbw', 'punpckldq', 'punpcklwd', 'push',
  'pusha', 'pushad', 'pushaw', 'pushf', 'pushfd', 'pushfq', 'pushfw', 'pxor',
  'rcl', 'rcr', 'rdshr', 'rdmsr', 'rdpmc', 'rdtsc', 'rdtscp', 'ret', 'retf',
  'retn', 'rol', 'ror', 'rdm', 'rsdc', 'rsldt', 'rsm', 'rsts', 'sahf', 'sal',
  'salc', 'sar', 'sbb', 'scasb', 'scasd', 'scasq', 'scasw', 'sfence', 'sgdt',
  'shl', 'shld', 'shr', 'shrd', 'sidt', 'sldt', 'skinit', 'smi', 'smint',
  'smintold', 'smsw', 'stc', 'std', 'sti', 'stosb', 'stosd', 'stosq', 'stosw',
  'str', 'sub', 'svdc', 'svldt', 'svts', 'swapgs', 'syscall', 'sysenter',
  'sysexit', 'sysret', 'test', 'ud0', 'ud1', 'ud2b', 'ud2', 'ud2a', 'umov',
  'verr', 'verw', 'fwait', 'wbinvd', 'wrshr', 'wrmsr', 'xadd', 'xbts',
  'xchg', 'xlatb', 'xlat', 'xor', 'cmova', 'cmovae', 'cmovb', 'cmovbe',
  'cmovc', 'cmove', 'cmovg', 'cmovge', 'cmovl', 'cmovle', 'cmovna', 'cmovnae',
  'cmovnb', 'cmovnbe', 'cmovnc', 'cmovne', 'cmovng', 'cmovnge', 'cmovnl',
  'cmovnle', 'cmovno', 'cmovnp', 'cmovns', 'cmovnz', 'cmovo', 'cmovp',
  'cmovpe', 'cmovpo', 'cmovs', 'cmovz', 'cmovcc', 'ja', 'jae', 'jb', 'jbe',
  'jc', 'je', 'jg', 'jge', 'jl', 'jle', 'jna', 'jnae', 'jnb', 'jnbe', 'jnc',
  'jne', 'jng', 'jnge', 'jnl', 'jnle', 'jno', 'jnp', 'jns', 'jnz', 'jo', 'jp',
  'jpe', 'jpo', 'js', 'jz', 'seta', 'setae', 'setb', 'setbe', 'setc', 'sete',
  'setg', 'setge', 'setl', 'setle', 'setna', 'setnae', 'setnb', 'setnbe',
  'setnc', 'setne', 'setng', 'setnge', 'setnl', 'setnle', 'setno', 'setnp',
  'setns', 'setnz', 'seto', 'setp', 'setpe', 'setpo', 'sets', 'setz', 
  --" Katmai Streaming SIMD instructions (SSE -- a.k.a. KNI, XMM, MMX2).
  'addps', 'addss', 'andnps', 'andps', 'cmpeqps', 'cmpeqss', 'cmpleps',
  'cmpless', 'cmpltps', 'cmpltss', 'cmpneqps', 'cmpneqss', 'cmpnleps',
  'cmpnless', 'cmpnltps', 'cmpnltss', 'cmpordps', 'cmpordss', 'cmpunordps',
  'cmpunordss', 'cmpps', 'cmpss', 'comiss', 'cvtpi2ps', 'cvtps2pi', 'cvtsi2ss',
  'cvtss2si', 'cvttps2pi', 'cvttss2si', 'divps', 'divss', 'ldmxcsr', 'maxps',
  'maxss', 'minps', 'minss', 'movaps', 'movhps', 'movlhps', 'movlps',
  'movhlps', 'movmskps', 'movntps', 'movss', 'movups', 'mulps', 'mulss',
  'orps', 'rcpps', 'rcpss', 'rsqrtps', 'rsqrtss', 'shufps', 'sqrtps', 'sqrtss',
  'stmxcsr', 'subps', 'subss', 'ucomiss', 'unpckhps', 'unpcklps', 'xorps',
  -- Introduced in Deschutes but necessary for SSE support.
  'fxrstor', 'fxrstor64', 'fxsave', 'fxsave64',
  -- XSAVE group (AVX and extended state).
  'xgetbv', 'xsetbv', 'xsave', 'xsave64', 'xsaveopt', 'xsaveopt64', 'xrstor',
  'xrstor64',
  -- Generic memory operations.
  'prefetchnta', 'prefetcht0', 'prefetcht1', 'prefetcht2', 'sfence',
  -- New MMX instructions introduced in Katmai.
  'maskmovq', 'movntq', 'pavgb', 'pavgw', 'pextrw', 'pinsrw', 'pmaxsw',
  'pmaxub', 'pminsw', 'pminub', 'pmovmskb', 'pmulhuw', 'psadbw', 'pshufw',
  -- AMD Enhanced 3DNow! (Athlon) instructions.
  'pf2iw', 'pfnacc', 'pfpnacc', 'pi2fw', 'pswapd',
  -- Willamette SSE2 Cacheability Instructions.
  'maskmovdqu', 'clflush', 'movntdq', 'movnti', 'movntpd', 'lfence', 'mfence',
  -- Willamette MMX instructions (SSE2 SIMD Integer Instructions).
  'movd', 'movdqa', 'movdqu', 'movdq2q', 'movq', 'movq2dq', 'packsswb',
  'packssdw', 'packuswb', 'paddb', 'paddw', 'paddd', 'paddq', 'paddsb',
  'paddsw', 'paddusb', 'paddusw', 'pand', 'pandn', 'pavgb', 'pavgw', 'pcmpeqb',
  'pcmpeqw', 'pcmpeqd', 'pcmpgtb', 'pcmpgtw', 'pcmpgtd', 'pextrw', 'pinsrw',
  'pmaddwd', 'pmaxsw', 'pmaxub', 'pminsw', 'pminub', 'pmovmskb', 'pmulhuw',
  'pmulhw', 'pmullw', 'pmuludq', 'por', 'psadbw', 'pshufd', 'pshufhw',
  'pshuflw', 'pslldq', 'psllw', 'pslld', 'psllq', 'psraw', 'psrad', 'psrldq',
  'psrlw', 'psrld', 'psrlq', 'psubb', 'psubw', 'psubd', 'psubq', 'psubsb',
  'psubsw', 'psubusb', 'psubusw', 'punpckhbw', 'punpckhwd', 'punpckhdq',
  'punpckhqdq', 'punpcklbw', 'punpcklwd', 'punpckldq', 'punpcklqdq', 'pxor',
  -- Willamette Streaming SIMD instructions (SSE2).
  'addpd', 'addsd', 'andnpd', 'andpd', 'cmpeqpd', 'cmpeqsd', 'cmplepd',
  'cmplesd', 'cmpltpd', 'cmpltsd', 'cmpneqpd', 'cmpneqsd', 'cmpnlepd',
  'cmpnlesd', 'cmpnltpd', 'cmpnltsd', 'cmpordpd', 'cmpordsd', 'cmpunordpd',
  'cmpunordsd', 'cmppd', 'cmpsd', 'comisd', 'cvtdq2pd', 'cvtdq2ps',
  'cvtpd2dq', 'cvtpd2pi', 'cvtpd2ps', 'cvtpi2pd', 'cvtps2dq', 'cvtps2pd',
  'cvtsd2si', 'cvtsd2ss', 'cvtsi2sd', 'cvtss2sd', 'cvttpd2pi', 'cvttpd2dq',
  'cvttps2dq', 'cvttsd2si', 'divpd', 'divsd', 'maxpd', 'maxsd', 'minpd',
  'minsd', 'movapd', 'movhpd', 'movlpd', 'movmskpd', 'movsd', 'movupd',
  'mulpd', 'mulsd', 'orpd', 'shufpd', 'sqrtpd', 'sqrtsd', 'subpd', 'subsd',
  'ucomisd', 'unpckhpd', 'unpcklpd', 'xorpd',
  -- Prescott New Instructions (SSE3).
  'addsubpd', 'addsubps', 'haddpd', 'haddps', 'hsubpd', 'hsubps', 'lddqu',
  'movddup', 'movshdup', 'movsldup',
  -- VMX/SVM Instructions.
  'clgi', 'stgi', 'vmcall', 'vmclear', 'vmfunc', 'vmlaunch', 'vmload',
  'vmmcall', 'vmptrld', 'vmptrst', 'vmread', 'vmresume', 'vmrun', 'vmsave',
  'vmwrite', 'vmxoff', 'vmxon',
  -- Extended Page Tables VMX instructions.
  'invept', 'invvpid',
  -- Tejas New Instructions (SSSE3).
  'pabsb', 'pabsw', 'pabsd', 'palignr', 'phaddw', 'phaddd', 'phaddsw',
  'phsubw', 'phsubd', 'phsubsw', 'pmaddubsw', 'pmulhrsw', 'pshufb', 'psignb',
  'psignw', 'psignd',
  -- AMD SSE4A.
  'extrq', 'insertq', 'movntsd', 'movntss',
  -- New instructions in Barcelona.
  'lzcnt',
  -- Penryn New Instructions (SSE4.1).
  'blendpd', 'blendps', 'blendvpd', 'blendvps', 'dppd', 'dpps', 'extractps',
  'insertps', 'movntdqa', 'mpsadbw', 'packusdw', 'pblendvb', 'pblendw',
  'pcmpeqq', 'pextrb', 'pextrd', 'pextrq', 'pextrw', 'phminposuw', 'pinsrb',
  'pinsrd', 'pinsrq', 'pmaxsb', 'pmaxsd', 'pmaxud', 'pmaxuw', 'pminsb',
  'pminsd', 'pminud', 'pminuw', 'pmovsxbw', 'pmovsxbd', 'pmovsxbq', 'pmovsxwd',
  'pmovsxwq', 'pmovsxdq', 'pmovzxbw', 'pmovzxbd', 'pmovzxbq', 'pmovzxwd',
  'pmovzxwq', 'pmovzxdq', 'pmuldq', 'pmulld', 'ptest', 'roundpd', 'roundps',
  'roundsd', 'roundss',
  -- Nehalem New Instructions (SSE4.2).
  'crc32', 'pcmpestri', 'pcmpestrm', 'pcmpistri', 'pcmpistrm', 'pcmpgtq',
  'popcnt',
  -- Intel SMX.
  'getsec',
  -- Geode (Cyrix) 3DNow! additions.
  'pfrcpv', 'pfrsqrtv',
  -- Intel new instructions in ???.
  'movbe',
  -- Intel AES instructions.
  'aesenc', 'aesenclast', 'aesdec', 'aesdeclast', 'aesimc', 'aeskeygenassist',
  -- Intel AVX AES instructions.
  'vaesenc', 'vaesenclast', 'vaesdec', 'vaesdeclast', 'vaesimc',
  'vaeskeygenassist',
  -- Intel AVX instructions.
  'vaddpd', 'vaddps', 'vaddsd', 'vaddss', 'vaddsubpd', 'vaddsubps',
  'vandpd', 'vandps', 'vandnpd', 'vandnps', 'vblendpd', 'vblendps',
  'vblendvpd', 'vblendvps', 'vbroadcastss', 'vbroadcastsd', 'vbroadcastf128',
  'vcmpeq_ospd', 'vcmpeqpd', 'vcmplt_ospd', 'vcmpltpd', 'vcmple_ospd',
  'vcmplepd', 'vcmpunord_qpd', 'vcmpunordpd', 'vcmpneq_uqpd', 'vcmpneqpd',
  'vcmpnlt_uspd', 'vcmpnltpd', 'vcmpnle_uspd', 'vcmpnlepd', 'vcmpord_qpd',
  'vcmpordpd', 'vcmpeq_uqpd', 'vcmpnge_uspd', 'vcmpngepd', 'vcmpngt_uspd',
  'vcmpngtpd', 'vcmpfalse_oqpd', 'vcmpfalsepd', 'vcmpneq_oqpd', 'vcmpge_ospd',
  'vcmpgepd', 'vcmpgt_ospd', 'vcmpgtpd', 'vcmptrue_uqpd', 'vcmptruepd',
  'vcmpeq_ospd', 'vcmplt_oqpd', 'vcmple_oqpd', 'vcmpunord_spd', 'vcmpneq_uspd',
  'vcmpnlt_uqpd', 'vcmpnle_uqpd', 'vcmpord_spd', 'vcmpeq_uspd', 'vcmpnge_uqpd',
  'vcmpngt_uqpd', 'vcmpfalse_ospd', 'vcmpneq_ospd', 'vcmpge_oqpd',
  'vcmpgt_oqpd', 'vcmptrue_uspd', 'vcmppd', 'vcmpeq_osps', 'vcmpeqps',
  'vcmplt_osps', 'vcmpltps', 'vcmple_osps', 'vcmpleps', 'vcmpunord_qps',
  'vcmpunordps', 'vcmpneq_uqps', 'vcmpneqps', 'vcmpnlt_usps', 'vcmpnltps',
  'vcmpnle_usps', 'vcmpnleps', 'vcmpord_qps', 'vcmpordps', 'vcmpeq_uqps',
  'vcmpnge_usps', 'vcmpngeps', 'vcmpngt_usps', 'vcmpngtps', 'vcmpfalse_oqps',
  'vcmpfalseps', 'vcmpneq_oqps', 'vcmpge_osps', 'vcmpgeps', 'vcmpgt_osps',
  'vcmpgtps', 'vcmptrue_uqps', 'vcmptrueps', 'vcmpeq_osps', 'vcmplt_oqps',
  'vcmple_oqps', 'vcmpunord_sps', 'vcmpneq_usps', 'vcmpnlt_uqps',
  'vcmpnle_uqps', 'vcmpord_sps', 'vcmpeq_usps', 'vcmpnge_uqps',
  'vcmpngt_uqps', 'vcmpfalse_osps', 'vcmpneq_osps', 'vcmpge_oqps',
  'vcmpgt_oqps', 'vcmptrue_usps', 'vcmpps', 'vcmpeq_ossd', 'vcmpeqsd',
  'vcmplt_ossd', 'vcmpltsd', 'vcmple_ossd', 'vcmplesd', 'vcmpunord_qsd',
  'vcmpunordsd', 'vcmpneq_uqsd', 'vcmpneqsd', 'vcmpnlt_ussd', 'vcmpnltsd',
  'vcmpnle_ussd', 'vcmpnlesd', 'vcmpord_qsd', 'vcmpordsd', 'vcmpeq_uqsd',
  'vcmpnge_ussd', 'vcmpngesd', 'vcmpngt_ussd', 'vcmpngtsd', 'vcmpfalse_oqsd',
  'vcmpfalsesd', 'vcmpneq_oqsd', 'vcmpge_ossd', 'vcmpgesd', 'vcmpgt_ossd',
  'vcmpgtsd', 'vcmptrue_uqsd', 'vcmptruesd', 'vcmpeq_ossd', 'vcmplt_oqsd',
  'vcmple_oqsd', 'vcmpunord_ssd', 'vcmpneq_ussd', 'vcmpnlt_uqsd',
  'vcmpnle_uqsd', 'vcmpord_ssd', 'vcmpeq_ussd', 'vcmpnge_uqsd',
  'vcmpngt_uqsd', 'vcmpfalse_ossd', 'vcmpneq_ossd', 'vcmpge_oqsd',
  'vcmpgt_oqsd', 'vcmptrue_ussd', 'vcmpsd', 'vcmpeq_osss', 'vcmpeqss',
  'vcmplt_osss', 'vcmpltss', 'vcmple_osss', 'vcmpless', 'vcmpunord_qss',
  'vcmpunordss', 'vcmpneq_uqss', 'vcmpneqss', 'vcmpnlt_usss', 'vcmpnltss',
  'vcmpnle_usss', 'vcmpnless', 'vcmpord_qss', 'vcmpordss', 'vcmpeq_uqss',
  'vcmpnge_usss', 'vcmpngess', 'vcmpngt_usss', 'vcmpngtss', 'vcmpfalse_oqss',
  'vcmpfalsess', 'vcmpneq_oqss', 'vcmpge_osss', 'vcmpgess', 'vcmpgt_osss',
  'vcmpgtss', 'vcmptrue_uqss', 'vcmptruess', 'vcmpeq_osss', 'vcmplt_oqss',
  'vcmple_oqss', 'vcmpunord_sss', 'vcmpneq_usss', 'vcmpnlt_uqss',
  'vcmpnle_uqss', 'vcmpord_sss', 'vcmpeq_usss', 'vcmpnge_uqss',
  'vcmpngt_uqss', 'vcmpfalse_osss', 'vcmpneq_osss', 'vcmpge_oqss',
  'vcmpgt_oqss', 'vcmptrue_usss', 'vcmpss', 'vcomisd', 'vcomiss',
  'vcvtdq2pd', 'vcvtdq2ps', 'vcvtpd2dq', 'vcvtpd2ps', 'vcvtps2dq',
  'vcvtps2pd', 'vcvtsd2si', 'vcvtsd2ss', 'vcvtsi2sd', 'vcvtsi2ss',
  'vcvtss2sd', 'vcvtss2si', 'vcvttpd2dq', 'vcvttps2dq', 'vcvttsd2si',
  'vcvttss2si', 'vdivpd', 'vdivps', 'vdivsd', 'vdivss', 'vdppd', 'vdpps',
  'vextractf128', 'vextractps', 'vhaddpd', 'vhaddps', 'vhsubpd', 'vhsubps',
  'vinsertf128', 'vinsertps', 'vlddqu', 'vldqqu', 'vlddqu', 'vldmxcsr',
  'vmaskmovdqu', 'vmaskmovps', 'vmaskmovpd', 'vmaxpd', 'vmaxps', 'vmaxsd',
  'vmaxss', 'vminpd', 'vminps', 'vminsd', 'vminss', 'vmovapd', 'vmovaps',
  'vmovd', 'vmovq', 'vmovddup', 'vmovdqa', 'vmovqqa', 'vmovdqa', 'vmovdqu',
  'vmovqqu', 'vmovdqu', 'vmovhlps', 'vmovhpd', 'vmovhps', 'vmovlhps',
  'vmovlpd', 'vmovlps', 'vmovmskpd', 'vmovmskps', 'vmovntdq', 'vmovntqq',
  'vmovntdq', 'vmovntdqa', 'vmovntpd', 'vmovntps', 'vmovsd', 'vmovshdup',
  'vmovsldup', 'vmovss', 'vmovupd', 'vmovups', 'vmpsadbw', 'vmulpd',
  'vmulps', 'vmulsd', 'vmulss', 'vorpd', 'vorps', 'vpabsb', 'vpabsw',
  'vpabsd', 'vpacksswb', 'vpackssdw', 'vpackuswb', 'vpackusdw', 'vpaddb',
  'vpaddw', 'vpaddd', 'vpaddq', 'vpaddsb', 'vpaddsw', 'vpaddusb', 'vpaddusw',
  'vpalignr', 'vpand', 'vpandn', 'vpavgb', 'vpavgw', 'vpblendvb', 'vpblendw',
  'vpcmpestri', 'vpcmpestrm', 'vpcmpistri', 'vpcmpistrm', 'vpcmpeqb',
  'vpcmpeqw', 'vpcmpeqd', 'vpcmpeqq', 'vpcmpgtb', 'vpcmpgtw', 'vpcmpgtd',
  'vpcmpgtq', 'vpermilpd', 'vpermilps', 'vperm2f128', 'vpextrb', 'vpextrw',
  'vpextrd', 'vpextrq', 'vphaddw', 'vphaddd', 'vphaddsw', 'vphminposuw',
  'vphsubw', 'vphsubd', 'vphsubsw', 'vpinsrb', 'vpinsrw', 'vpinsrd',
  'vpinsrq', 'vpmaddwd', 'vpmaddubsw', 'vpmaxsb', 'vpmaxsw', 'vpmaxsd',
  'vpmaxub', 'vpmaxuw', 'vpmaxud', 'vpminsb', 'vpminsw', 'vpminsd', 'vpminub',
  'vpminuw', 'vpminud', 'vpmovmskb', 'vpmovsxbw', 'vpmovsxbd', 'vpmovsxbq',
  'vpmovsxwd', 'vpmovsxwq', 'vpmovsxdq', 'vpmovzxbw', 'vpmovzxbd', 'vpmovzxbq',
  'vpmovzxwd', 'vpmovzxwq', 'vpmovzxdq', 'vpmulhuw', 'vpmulhrsw', 'vpmulhw',
  'vpmullw', 'vpmulld', 'vpmuludq', 'vpmuldq', 'vpor', 'vpsadbw', 'vpshufb',
  'vpshufd', 'vpshufhw', 'vpshuflw', 'vpsignb', 'vpsignw', 'vpsignd',
  'vpslldq', 'vpsrldq', 'vpsllw', 'vpslld', 'vpsllq', 'vpsraw', 'vpsrad',
  'vpsrlw', 'vpsrld', 'vpsrlq', 'vptest', 'vpsubb', 'vpsubw', 'vpsubd',
  'vpsubq', 'vpsubsb', 'vpsubsw', 'vpsubusb', 'vpsubusw', 'vpunpckhbw',
  'vpunpckhwd', 'vpunpckhdq', 'vpunpckhqdq', 'vpunpcklbw', 'vpunpcklwd',
  'vpunpckldq', 'vpunpcklqdq', 'vpxor', 'vrcpps', 'vrcpss', 'vrsqrtps',
  'vrsqrtss', 'vroundpd', 'vroundps', 'vroundsd', 'vroundss', 'vshufpd',
  'vshufps', 'vsqrtpd', 'vsqrtps', 'vsqrtsd', 'vsqrtss', 'vstmxcsr', 'vsubpd',
  'vsubps', 'vsubsd', 'vsubss', 'vtestps', 'vtestpd', 'vucomisd', 'vucomiss',
  'vunpckhpd', 'vunpckhps', 'vunpcklpd', 'vunpcklps', 'vxorpd', 'vxorps',
  'vzeroall', 'vzeroupper',
  -- Intel Carry-Less Multiplication instructions (CLMUL).
  'pclmullqlqdq', 'pclmulhqlqdq', 'pclmullqhqdq', 'pclmulhqhqdq', 'pclmulqdq',
  -- Intel AVX Carry-Less Multiplication instructions (CLMUL).
  'vpclmullqlqdq', 'vpclmulhqlqdq', 'vpclmullqhqdq', 'vpclmulhqhqdq',
  'vpclmulqdq',
  -- Intel Fused Multiply-Add instructions (FMA).
  'vfmadd132ps', 'vfmadd132pd', 'vfmadd312ps', 'vfmadd312pd', 'vfmadd213ps',
  'vfmadd213pd', 'vfmadd123ps', 'vfmadd123pd', 'vfmadd231ps', 'vfmadd231pd',
  'vfmadd321ps', 'vfmadd321pd', 'vfmaddsub132ps', 'vfmaddsub132pd',
  'vfmaddsub312ps', 'vfmaddsub312pd', 'vfmaddsub213ps', 'vfmaddsub213pd',
  'vfmaddsub123ps', 'vfmaddsub123pd', 'vfmaddsub231ps', 'vfmaddsub231pd',
  'vfmaddsub321ps', 'vfmaddsub321pd', 'vfmsub132ps', 'vfmsub132pd',
  'vfmsub312ps', 'vfmsub312pd', 'vfmsub213ps', 'vfmsub213pd', 'vfmsub123ps',
  'vfmsub123pd', 'vfmsub231ps', 'vfmsub231pd', 'vfmsub321ps', 'vfmsub321pd',
  'vfmsubadd132ps', 'vfmsubadd132pd', 'vfmsubadd312ps', 'vfmsubadd312pd',
  'vfmsubadd213ps', 'vfmsubadd213pd', 'vfmsubadd123ps', 'vfmsubadd123pd',
  'vfmsubadd231ps', 'vfmsubadd231pd', 'vfmsubadd321ps', 'vfmsubadd321pd',
  'vfnmadd132ps', 'vfnmadd132pd', 'vfnmadd312ps', 'vfnmadd312pd',
  'vfnmadd213ps', 'vfnmadd213pd', 'vfnmadd123ps', 'vfnmadd123pd',
  'vfnmadd231ps', 'vfnmadd231pd', 'vfnmadd321ps', 'vfnmadd321pd',
  'vfnmsub132ps', 'vfnmsub132pd', 'vfnmsub312ps', 'vfnmsub312pd',
  'vfnmsub213ps', 'vfnmsub213pd', 'vfnmsub123ps', 'vfnmsub123pd',
  'vfnmsub231ps', 'vfnmsub231pd', 'vfnmsub321ps', 'vfnmsub321pd',
  'vfmadd132ss', 'vfmadd132sd', 'vfmadd312ss', 'vfmadd312sd', 'vfmadd213ss',
  'vfmadd213sd', 'vfmadd123ss', 'vfmadd123sd', 'vfmadd231ss', 'vfmadd231sd',
  'vfmadd321ss', 'vfmadd321sd', 'vfmsub132ss', 'vfmsub132sd', 'vfmsub312ss',
  'vfmsub312sd', 'vfmsub213ss', 'vfmsub213sd', 'vfmsub123ss', 'vfmsub123sd',
  'vfmsub231ss', 'vfmsub231sd', 'vfmsub321ss', 'vfmsub321sd', 'vfnmadd132ss',
  'vfnmadd132sd', 'vfnmadd312ss', 'vfnmadd312sd', 'vfnmadd213ss',
  'vfnmadd213sd', 'vfnmadd123ss', 'vfnmadd123sd', 'vfnmadd231ss',
  'vfnmadd231sd', 'vfnmadd321ss', 'vfnmadd321sd', 'vfnmsub132ss',
  'vfnmsub132sd', 'vfnmsub312ss', 'vfnmsub312sd', 'vfnmsub213ss',
  'vfnmsub213sd', 'vfnmsub123ss', 'vfnmsub123sd', 'vfnmsub231ss',
  'vfnmsub231sd', 'vfnmsub321ss', 'vfnmsub321sd',
  -- Intel post-32 nm processor instructions.
  'rdfsbase', 'rdgsbase', 'rdrand', 'wrfsbase', 'wrgsbase', 'vcvtph2ps',
  'vcvtps2ph', 'adcx', 'adox', 'rdseed', 'clac', 'stac',
  -- VIA (Centaur) security instructions.
  'xstore', 'xcryptecb', 'xcryptcbc', 'xcryptctr', 'xcryptcfb', 'xcryptofb',
  'montmul', 'xsha1', 'xsha256',
  -- AMD Lightweight Profiling (LWP) instructions.
  'llwpcb', 'slwpcb', 'lwpval', 'lwpins',
  -- AMD XOP and FMA4 instructions (SSE5).
  'vfmaddpd', 'vfmaddps', 'vfmaddsd', 'vfmaddss', 'vfmaddsubpd',
  'vfmaddsubps', 'vfmsubaddpd', 'vfmsubaddps', 'vfmsubpd', 'vfmsubps',
  'vfmsubsd', 'vfmsubss', 'vfnmaddpd', 'vfnmaddps', 'vfnmaddsd', 'vfnmaddss',
  'vfnmsubpd', 'vfnmsubps', 'vfnmsubsd', 'vfnmsubss', 'vfrczpd', 'vfrczps',
  'vfrczsd', 'vfrczss', 'vpcmov', 'vpcomb', 'vpcomd', 'vpcomq', 'vpcomub',
  'vpcomud', 'vpcomuq', 'vpcomuw', 'vpcomw', 'vphaddbd', 'vphaddbq',
  'vphaddbw', 'vphadddq', 'vphaddubd', 'vphaddubq', 'vphaddubw', 'vphaddudq',
  'vphadduwd', 'vphadduwq', 'vphaddwd', 'vphaddwq', 'vphsubbw', 'vphsubdq',
  'vphsubwd', 'vpmacsdd', 'vpmacsdqh', 'vpmacsdql', 'vpmacssdd', 'vpmacssdqh',
  'vpmacssdql', 'vpmacsswd', 'vpmacssww', 'vpmacswd', 'vpmacsww', 'vpmadcsswd',
  'vpmadcswd', 'vpperm', 'vprotb', 'vprotd', 'vprotq', 'vprotw', 'vpshab',
  'vpshad', 'vpshaq', 'vpshaw', 'vpshlb', 'vpshld', 'vpshlq', 'vpshlw',
  -- Intel AVX2 instructions.
  'vmpsadbw', 'vpabsb', 'vpabsw', 'vpabsd', 'vpacksswb', 'vpackssdw',
  'vpackusdw', 'vpackuswb', 'vpaddb', 'vpaddw', 'vpaddd', 'vpaddq',
  'vpaddsb', 'vpaddsw', 'vpaddusb', 'vpaddusw', 'vpalignr', 'vpand',
  'vpandn', 'vpavgb', 'vpavgw', 'vpblendvb', 'vpblendw', 'vpcmpeqb',
  'vpcmpeqw', 'vpcmpeqd', 'vpcmpeqq', 'vpcmpgtb', 'vpcmpgtw', 'vpcmpgtd',
  'vpcmpgtq', 'vphaddw', 'vphaddd', 'vphaddsw', 'vphsubw', 'vphsubd',
  'vphsubsw', 'vpmaddubsw', 'vpmaddwd', 'vpmaxsb', 'vpmaxsw', 'vpmaxsd',
  'vpmaxub', 'vpmaxuw', 'vpmaxud', 'vpminsb', 'vpminsw', 'vpminsd', 'vpminub',
  'vpminuw', 'vpminud', 'vpmovmskb', 'vpmovsxbw', 'vpmovsxbd', 'vpmovsxbq',
  'vpmovsxwd', 'vpmovsxwq', 'vpmovsxdq', 'vpmovzxbw', 'vpmovzxbd', 'vpmovzxbq',
  'vpmovzxwd', 'vpmovzxwq', 'vpmovzxdq', 'vpmuldq', 'vpmulhrsw', 'vpmulhuw',
  'vpmulhw', 'vpmullw', 'vpmulld', 'vpmuludq', 'vpor', 'vpsadbw', 'vpshufb',
  'vpshufd', 'vpshufhw', 'vpshuflw', 'vpsignb', 'vpsignw', 'vpsignd',
  'vpslldq', 'vpsllw', 'vpslld', 'vpsllq', 'vpsraw', 'vpsrad', 'vpsrldq',
  'vpsrlw', 'vpsrld', 'vpsrlq', 'vpsubb', 'vpsubw', 'vpsubd', 'vpsubq',
  'vpsubsb', 'vpsubsw', 'vpsubusb', 'vpsubusw', 'vpunpckhbw', 'vpunpckhwd',
  'vpunpckhdq', 'vpunpckhqdq', 'vpunpcklbw', 'vpunpcklwd', 'vpunpckldq',
  'vpunpcklqdq', 'vpxor', 'vmovntdqa', 'vbroadcastss', 'vbroadcastsd',
  'vbroadcasti128', 'vpblendd', 'vpbroadcastb', 'vpbroadcastw', 'vpbroadcastd',
  'vpbroadcastq', 'vpermd', 'vpermpd', 'vpermps', 'vpermq', 'vperm2i128',
  'vextracti128', 'vinserti128', 'vpmaskmovd', 'vpmaskmovq', 'vpmaskmovd',
  'vpmaskmovq', 'vpsllvd', 'vpsllvq', 'vpsllvd', 'vpsllvq', 'vpsravd',
  'vpsrlvd', 'vpsrlvq', 'vpsrlvd', 'vpsrlvq', 'vgatherdpd', 'vgatherqpd',
  'vgatherdpd', 'vgatherqpd', 'vgatherdps', 'vgatherqps', 'vgatherdps',
  'vgatherqps', 'vpgatherdd', 'vpgatherqd', 'vpgatherdd', 'vpgatherqd',
  'vpgatherdq', 'vpgatherqq', 'vpgatherdq', 'vpgatherqq',
  -- Transactional Synchronization Extensions (TSX).
  'xabort', 'xbegin', 'xend', 'xtest',
  -- Intel BMI1 and BMI2 instructions, AMD TBM instructions.
  'andn', 'bextr', 'blci', 'blcic', 'blsi', 'blsic', 'blcfill', 'blsfill',
  'blcmsk', 'blsmsk', 'blsr', 'blcs', 'bzhi', 'mulx', 'pdep', 'pext', 'rorx',
  'sarx', 'shlx', 'shrx', 'tzcnt', 'tzmsk', 't1mskc',
  -- Systematic names for the hinting nop instructions.
  'hint_nop0', 'hint_nop1', 'hint_nop2', 'hint_nop3', 'hint_nop4',
  'hint_nop5', 'hint_nop6', 'hint_nop7', 'hint_nop8', 'hint_nop9',
  'hint_nop10', 'hint_nop11', 'hint_nop12', 'hint_nop13', 'hint_nop14',
  'hint_nop15', 'hint_nop16', 'hint_nop17', 'hint_nop18', 'hint_nop19',
  'hint_nop20', 'hint_nop21', 'hint_nop22', 'hint_nop23', 'hint_nop24',
  'hint_nop25', 'hint_nop26', 'hint_nop27', 'hint_nop28', 'hint_nop29',
  'hint_nop30', 'hint_nop31', 'hint_nop32', 'hint_nop33', 'hint_nop34',
  'hint_nop35', 'hint_nop36', 'hint_nop37', 'hint_nop38', 'hint_nop39',
  'hint_nop40', 'hint_nop41', 'hint_nop42', 'hint_nop43', 'hint_nop44',
  'hint_nop45', 'hint_nop46', 'hint_nop47', 'hint_nop48', 'hint_nop49',
  'hint_nop50', 'hint_nop51', 'hint_nop52', 'hint_nop53', 'hint_nop54',
  'hint_nop55', 'hint_nop56', 'hint_nop57', 'hint_nop58', 'hint_nop59',
  'hint_nop60', 'hint_nop61', 'hint_nop62', 'hint_nop63',
})

-- Types.
local sizes = word_match{
  'byte', 'word', 'dword', 'qword', 'tword', 'oword', 'yword',
  'a16', 'a32', 'a64', 'o16', 'o32', 'o64' -- instructions
}
local wrt_types = '..' * word_match{
  'start', 'gotpc', 'gotoff', 'gottpoff', 'got', 'plt', 'sym', 'tlsie'
}
local type = token(l.TYPE, sizes + wrt_types)

-- Registers.
local register = token('register', word_match{
  -- 32-bit registers.
  'ah', 'al', 'ax', 'bh', 'bl', 'bp', 'bx', 'ch', 'cl', 'cx', 'dh', 'di', 'dl',
  'dx', 'eax', 'ebx', 'ebx', 'ecx', 'edi', 'edx', 'esi', 'esp', 'fs', 'mm0',
  'mm1', 'mm2', 'mm3', 'mm4', 'mm5', 'mm6', 'mm7', 'si', 'st0', 'st1', 'st2',
  'st3', 'st4', 'st5', 'st6', 'st7', 'xmm0', 'xmm1', 'xmm2', 'xmm3', 'xmm4',
  'xmm5', 'xmm6', 'xmm7', 'ymm0', 'ymm1', 'ymm2', 'ymm3', 'ymm4', 'ymm5',
  'ymm6', 'ymm7',
  -- 64-bit registers.
  'bpl', 'dil', 'gs', 'r8', 'r8b', 'r8w', 'r9', 'r9b', 'r9w', 'r10', 'r10b',
  'r10w', 'r11', 'r11b', 'r11w', 'r12', 'r12b', 'r12w', 'r13', 'r13b', 'r13w',
  'r14', 'r14b', 'r14w', 'r15', 'r15b', 'r15w', 'rax', 'rbp', 'rbx', 'rcx',
  'rdi', 'rdx', 'rsi', 'rsp', 'sil', 'xmm8', 'xmm9', 'xmm10', 'xmm11', 'xmm12',
  'xmm13', 'xmm14', 'xmm15', 'ymm8', 'ymm9', 'ymm10', 'ymm11', 'ymm12', 'ymm13',
  'ymm14', 'ymm15'
})

local word = (l.alpha + S('$._?')) * (l.alnum + S('$._?#@~'))^0

-- Labels.
local label = token(l.LABEL, word * ':')

-- Identifiers.
local identifier = token(l.IDENTIFIER, word)

-- Constants.
local constants = word_match{
  '__float8__', '__float16__', '__float32__', '__float64__', '__float80m__',
  '__float80e__', '__float128l__', '__float128h__', '__Infinity__', '__QNaN__',
  '__NaN__', '__SNaN__'
}
local constant = token(l.CONSTANT, constants + '$' * P('$')^-1 * -identifier)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|~:,()[]'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'instruction', instruction},
  {'register', register},
  {'type', type},
  {'constant', constant},
  {'label', label},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
}

M._tokenstyles = {
  instruction = l.STYLE_FUNCTION,
  register = l.STYLE_CONSTANT,
}

M._foldsymbols = {
  _patterns = {'%l+', '//'},
  [l.PREPROCESSOR] = {
    ['if'] = 1, endif = -1, macro = 1, endmacro = -1, rep = 1, endrep = -1,
    ['while'] = 1, endwhile = -1,
  },
  [l.KEYWORD] = {struc = 1, endstruc = -1},
  [l.COMMENT] = {['//'] = l.fold_line_comments('//')}
}

return M
