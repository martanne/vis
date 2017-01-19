-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- GLSL LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S
local table = _G.table

local M = {_NAME = 'glsl'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'attribute', 'const', 'in', 'inout', 'out', 'uniform', 'varying', 'invariant',
  'centroid', 'flat', 'smooth', 'noperspective', 'layout', 'patch', 'sample',
  'subroutine', 'lowp', 'mediump', 'highp', 'precision',
  -- Macros.
  '__VERSION__', '__LINE__', '__FILE__',
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'radians', 'degrees', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'sinh',
  'cosh', 'tanh', 'asinh', 'acosh', 'atanh', 'pow', 'exp', 'log', 'exp2',
  'log2', 'sqrt', 'inversesqrt', 'abs', 'sign', 'floor', 'trunc', 'round',
  'roundEven', 'ceil', 'fract', 'mod', 'modf', 'min', 'max', 'clamp', 'mix',
  'step', 'smoothstep', 'isnan', 'isinf', 'floatBitsToInt', 'floatBitsToUint',
  'intBitsToFloat', 'uintBitsToFloat', 'fma', 'frexp', 'ldexp', 'packUnorm2x16',
  'packUnorm4x8', 'packSnorm4x8', 'unpackUnorm2x16', 'unpackUnorm4x8',
  'unpackSnorm4x8', 'packDouble2x32', 'unpackDouble2x32', 'length', 'distance',
  'dot', 'cross', 'normalize', 'ftransform', 'faceforward', 'reflect',
  'refract', 'matrixCompMult', 'outerProduct', 'transpose', 'determinant',
  'inverse', 'lessThan', 'lessThanEqual', 'greaterThan', 'greaterThanEqual',
  'equal', 'notEqual', 'any', 'all', 'not', 'uaddCarry', 'usubBorrow',
  'umulExtended', 'imulExtended', 'bitfieldExtract', 'bitfildInsert',
  'bitfieldReverse', 'bitCount', 'findLSB', 'findMSB', 'textureSize',
  'textureQueryLOD', 'texture', 'textureProj', 'textureLod', 'textureOffset',
  'texelFetch', 'texelFetchOffset', 'textureProjOffset', 'textureLodOffset',
  'textureProjLod', 'textureProjLodOffset', 'textureGrad', 'textureGradOffset',
  'textureProjGrad', 'textureProjGradOffset', 'textureGather',
  'textureGatherOffset', 'texture1D', 'texture2D', 'texture3D', 'texture1DProj',
  'texture2DProj', 'texture3DProj', 'texture1DLod', 'texture2DLod',
  'texture3DLod', 'texture1DProjLod', 'texture2DProjLod', 'texture3DProjLod',
  'textureCube', 'textureCubeLod', 'shadow1D', 'shadow2D',  'shadow1DProj',
  'shadow2DProj', 'shadow1DLod', 'shadow2DLod', 'shadow1DProjLod',
  'shadow2DProjLod', 'dFdx', 'dFdy', 'fwidth', 'interpolateAtCentroid',
  'interpolateAtSample', 'interpolateAtOffset', 'noise1', 'noise2', 'noise3',
  'noise4', 'EmitStreamVertex', 'EndStreamPrimitive', 'EmitVertex',
  'EndPrimitive', 'barrier'
})

-- Types.
local type = token(l.TYPE,
                   S('bdiu')^-1 * 'vec' * R('24') +
                   P('d')^-1 * 'mat' * R('24') * ('x' * R('24')^-1) +
                   S('iu')^-1 * 'sampler' * R('13') * 'D' +
                   'sampler' * R('12') * 'D' * P('Array')^-1 * 'Shadow' +
                   S('iu')^-1 * 'sampler' * (R('12') * 'DArray' + word_match{
                     'Cube', '2DRect', 'Buffer', '2DMS', '2DMSArray',
                     '2DMSCubeArray'
                   }) +
                   word_match{
                     'samplerCubeShadow', 'sampler2DRectShadow',
                     'samplerCubeArrayShadow'
                   })

-- Variables.
local variable = token(l.VARIABLE, word_match{
  'gl_VertexID', 'gl_InstanceID', 'gl_Position', 'gl_PointSize',
  'gl_ClipDistance', 'gl_PrimitiveIDIn', 'gl_InvocationID', 'gl_PrimitiveID',
  'gl_Layer', 'gl_PatchVerticesIn', 'gl_TessLevelOuter', 'gl_TessLevelInner',
  'gl_TessCoord', 'gl_FragCoord', 'gl_FrontFacing', 'gl_PointCoord',
  'gl_SampleID', 'gl_SamplePosition', 'gl_FragColor', 'gl_FragData',
  'gl_FragDepth', 'gl_SampleMask', 'gl_ClipVertex', 'gl_FrontColor',
  'gl_BackColor', 'gl_FrontSecondaryColor', 'gl_BackSecondaryColor',
  'gl_TexCoord', 'gl_FogFragCoord', 'gl_Color', 'gl_SecondaryColor',
  'gl_Normal', 'gl_Vertex', 'gl_MultiTexCoord0', 'gl_MultiTexCoord1',
  'gl_MultiTexCoord2', 'gl_MultiTexCoord3', 'gl_MultiTexCoord4',
  'gl_MultiTexCoord5', 'gl_MultiTexCoord6', 'gl_MultiTexCoord7', 'gl_FogCoord'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'gl_MaxVertexAttribs', 'gl_MaxVertexUniformComponents', 'gl_MaxVaryingFloats',
  'gl_MaxVaryingComponents', 'gl_MaxVertexOutputComponents',
  'gl_MaxGeometryInputComponents', 'gl_MaxGeometryOutputComponents',
  'gl_MaxFragmentInputComponents', 'gl_MaxVertexTextureImageUnits',
  'gl_MaxCombinedTextureImageUnits', 'gl_MaxTextureImageUnits',
  'gl_MaxFragmentUniformComponents', 'gl_MaxDrawBuffers', 'gl_MaxClipDistances',
  'gl_MaxGeometryTextureImageUnits', 'gl_MaxGeometryOutputVertices',
  'gl_MaxGeometryTotalOutputComponents', 'gl_MaxGeometryUniformComponents',
  'gl_MaxGeometryVaryingComponents', 'gl_MaxTessControlInputComponents',
  'gl_MaxTessControlOutputComponents', 'gl_MaxTessControlTextureImageUnits',
  'gl_MaxTessControlUniformComponents',
  'gl_MaxTessControlTotalOutputComponents',
  'gl_MaxTessEvaluationInputComponents', 'gl_MaxTessEvaluationOutputComponents',
  'gl_MaxTessEvaluationTextureImageUnits',
  'gl_MaxTessEvaluationUniformComponents', 'gl_MaxTessPatchComponents',
  'gl_MaxPatchVertices', 'gl_MaxTessGenLevel', 'gl_MaxTextureUnits',
  'gl_MaxTextureCoords', 'gl_MaxClipPlanes',

  'gl_DepthRange', 'gl_ModelViewMatrix', 'gl_ProjectionMatrix',
  'gl_ModelViewProjectionMatrix', 'gl_TextureMatrix', 'gl_NormalMatrix',
  'gl_ModelViewMatrixInverse', 'gl_ProjectionMatrixInverse',
  'gl_ModelViewProjectionMatrixInverse', 'gl_TextureMatrixInverse',
  'gl_ModelViewMatrixTranspose', 'gl_ProjectionMatrixTranspose',
  'gl_ModelViewProjectionMatrixTranspose', 'gl_TextureMatrixTranspose',
  'gl_ModelViewMatrixInverseTranspose', 'gl_ProjectionMatrixInverseTranspose',
  'gl_ModelViewProjectionMatrixInverseTranspose',
  'gl_TextureMatrixInverseTranspose', 'gl_NormalScale', 'gl_ClipPlane',
  'gl_Point', 'gl_FrontMaterial', 'gl_BackMaterial', 'gl_LightSource',
  'gl_LightModel', 'gl_FrontLightModelProduct', 'gl_BackLightModelProduct',
  'gl_FrontLightProduct', 'gl_BackLightProduct', 'gl_TextureEnvColor',
  'gl_EyePlaneS', 'gl_EyePlaneT', 'gl_EyePlaneR', 'gl_EyePlaneQ',
  'gl_ObjectPlaneS', 'gl_ObjectPlaneT', 'gl_ObjectPlaneR', 'gl_ObjectPlaneQ',
  'gl_Fog'
})

-- Extend cpp lexer to include GLSL elements.
local cpp = l.load('cpp')
local _rules = cpp._rules
_rules[1] = {'whitespace', ws}
table.insert(_rules, 2, {'glsl_keyword', keyword})
table.insert(_rules, 3, {'glsl_function', func})
table.insert(_rules, 4, {'glsl_type', type})
table.insert(_rules, 5, {'glsl_variable', variable})
M._rules = _rules
M._foldsymbols = cpp._foldsymbols

return M
