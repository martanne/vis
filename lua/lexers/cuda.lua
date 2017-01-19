-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- CUDA LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S
local table = _G.table

local M = {_NAME = 'cuda'}

-- Whitespace
local ws = token(l.WHITESPACE, l.space^1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  '__global__', '__host__', '__device__', '__constant__', '__shared__'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  -- Atom.
  'atomicAdd', 'atomicAnd', 'atomicCAS', 'atomicDec', 'atomicExch', 'atomicInc',
  'atomicMax', 'atomicMin', 'atomicOr', 'atomicSub', 'atomicXor',
  -- Dev.
  'tex1D', 'tex1Dfetch', 'tex2D', '__float_as_int', '__int_as_float',
  '__float2int_rn', '__float2int_rz', '__float2int_ru', '__float2int_rd',
  '__float2uint_rn', '__float2uint_rz', '__float2uint_ru', '__float2uint_rd',
  '__int2float_rn', '__int2float_rz', '__int2float_ru', '__int2float_rd',
  '__uint2float_rn', '__uint2float_rz', '__uint2float_ru', '__uint2float_rd',
  '__fadd_rz', '__fmul_rz', '__fdividef', '__mul24', '__umul24', '__mulhi',
  '__umulhi', '__mul64hi', '__umul64hi', 'min', 'umin', 'fminf', 'fmin', 'max',
  'umax', 'fmaxf', 'fmax', 'abs', 'fabsf', 'fabs', 'sqrtf', 'sqrt', 'sinf',
  '__sinf', 'sin', 'cosf', '__cosf', 'cos', 'sincosf', '__sincosf', 'expf',
  '__expf', 'exp', 'logf', '__logf', 'log',
  -- Runtime.
  'cudaBindTexture', 'cudaBindTextureToArray', 'cudaChooseDevice',
  'cudaConfigureCall', 'cudaCreateChannelDesc', 'cudaD3D10GetDevice',
  'cudaD3D10MapResources', 'cudaD3D10RegisterResource',
  'cudaD3D10ResourceGetMappedArray', 'cudaD3D10ResourceGetMappedPitch',
  'cudaD3D10ResourceGetMappedPointer', 'cudaD3D10ResourceGetMappedSize',
  'cudaD3D10ResourceGetSurfaceDimensions', 'cudaD3D10ResourceSetMapFlags',
  'cudaD3D10SetDirect3DDevice', 'cudaD3D10UnmapResources',
  'cudaD3D10UnregisterResource', 'cudaD3D9GetDevice',
  'cudaD3D9GetDirect3DDevice', 'cudaD3D9MapResources',
  'cudaD3D9RegisterResource', 'cudaD3D9ResourceGetMappedArray',
  'cudaD3D9ResourceGetMappedPitch', 'cudaD3D9ResourceGetMappedPointer',
  'cudaD3D9ResourceGetMappedSize', 'cudaD3D9ResourceGetSurfaceDimensions',
  'cudaD3D9ResourceSetMapFlags', 'cudaD3D9SetDirect3DDevice',
  'cudaD3D9UnmapResources', 'cudaD3D9UnregisterResource', 'cudaEventCreate',
  'cudaEventDestroy', 'cudaEventElapsedTime', 'cudaEventQuery',
  'cudaEventRecord', 'cudaEventSynchronize', 'cudaFree', 'cudaFreeArray',
  'cudaFreeHost', 'cudaGetChannelDesc', 'cudaGetDevice', 'cudaGetDeviceCount',
  'cudaGetDeviceProperties', 'cudaGetErrorString', 'cudaGetLastError',
  'cudaGetSymbolAddress', 'cudaGetSymbolSize', 'cudaGetTextureAlignmentOffset',
  'cudaGetTextureReference', 'cudaGLMapBufferObject',
  'cudaGLRegisterBufferObject', 'cudaGLSetGLDevice', 'cudaGLUnmapBufferObject',
  'cudaGLUnregisterBufferObject', 'cudaLaunch', 'cudaMalloc', 'cudaMalloc3D',
  'cudaMalloc3DArray', 'cudaMallocArray', 'cudaMallocHost', 'cudaMallocPitch',
  'cudaMemcpy', 'cudaMemcpy2D', 'cudaMemcpy2DArrayToArray',
  'cudaMemcpy2DFromArray', 'cudaMemcpy2DToArray', 'cudaMemcpy3D',
  'cudaMemcpyArrayToArray', 'cudaMemcpyFromArray', 'cudaMemcpyFromSymbol',
  'cudaMemcpyToArray', 'cudaMemcpyToSymbol', 'cudaMemset', 'cudaMemset2D',
  'cudaMemset3D', 'cudaSetDevice', 'cudaSetupArgument', 'cudaStreamCreate',
  'cudaStreamDestroy', 'cudaStreamQuery', 'cudaStreamSynchronize',
  'cudaThreadExit', 'cudaThreadSynchronize', 'cudaUnbindTexture'
})

-- Types.
local type = token(l.TYPE, word_match{
  'uint', 'int1', 'uint1', 'int2', 'uint2', 'int3', 'uint3', 'int4', 'uint4',
  'float1', 'float2', 'float3', 'float4', 'char1', 'char2', 'char3', 'char4',
  'uchar1', 'uchar2', 'uchar3', 'uchar4', 'short1', 'short2', 'short3',
  'short4', 'dim1', 'dim2', 'dim3', 'dim4'
})

-- Variables.
local variable = token(l.VARIABLE, word_match{
  'gridDim', 'blockIdx', 'blockDim', 'threadIdx'
})

-- Extend cpp lexer to include CUDA elements.
local cpp = l.load('cpp')
local _rules = cpp._rules
_rules[1] = {'whitespace', ws}
table.insert(_rules, 2, {'cuda_keyword', keyword})
table.insert(_rules, 3, {'cuda_function', func})
table.insert(_rules, 4, {'cuda_type', type})
table.insert(_rules, 5, {'cuda_variable', variable})
M._rules = _rules
M._foldsymbols = cpp._foldsymbols

return M
