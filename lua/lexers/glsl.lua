-- Copyright 2006-2025 Mitchell. See LICENSE.
-- GLSL LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {inherit = lexer.load('c')})

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'attribute', 'const', 'uniform', 'varying', 'buffer', 'shared', 'coherent', 'volatile',
	'restrict', 'readonly', 'writeonly', 'layout', 'centroid', 'flat', 'smooth', 'noperspective',
	'patch', 'sample', 'break', 'continue', 'do', 'for', 'while', 'switch', 'case', 'default', 'if',
	'else', 'subroutine', 'in', 'inout', 'out', 'true', 'false', 'invariant', 'precise', 'discard',
	'return', 'lowp', 'mediump', 'highp', 'precision', 'struct', --
	-- Reserved.
	'common', 'partition', 'active', 'asm', 'class', 'union', 'enum', 'typedef', 'template', 'this',
	'resource', 'goto', 'inline', 'noinline', 'public', 'static', 'extern', 'external', 'interface',
	'superp', 'input', 'output', 'filter', 'sizeof', 'cast', 'namespace', 'using'
})

lex:set_word_list(lexer.TYPE, {
	'atomic_uint', 'float', 'double', 'int', 'void', 'bool', 'mat2', 'mat3', 'mat4', 'dmat2', 'dmat3',
	'dmat4', 'mat2x2', 'mat2x3', 'mat2x4', 'dmat2x2', 'dmat2x3', 'dmat2x4', 'mat3x2', 'mat3x3',
	'mat3x4', 'dmat3x2', 'dmat3x3', 'dmat3x4', 'mat4x2', 'mat4x3', 'mat4x4', 'dmat4x2', 'dmat4x3',
	'dmat4x4', 'vec2', 'vec3', 'vec4', 'ivec2', 'ivec3', 'ivec4', 'bvec2', 'bvec3', 'bvec4', 'dvec2',
	'dvec3', 'dvec4', 'uint', 'uvec2', 'uvec3', 'uvec4', 'sampler1D', 'sampler2D', 'sampler3D',
	'samplerCube', 'sampler1DShadow', 'sampler2DShadow', 'samplerCubeShadow', 'sampler1DArray',
	'sampler2DArray', 'sampler1DArrayShadow', 'sampler2DArrayShadow', 'isampler1D', 'isampler2D',
	'isampler3D', 'isamplerCube', 'isampler1DArray', 'isampler2DArray', 'usampler1D', 'usampler2D',
	'usampler3D', 'usamplerCube', 'usampler1DArray', 'usampler2DArray', 'sampler2DRect',
	'sampler2DRectShadow', 'isampler2DRect', 'usampler2DRect', 'samplerBuffer', 'isamplerBuffer',
	'usamplerBuffer', 'sampler2DMS', 'isampler2DMS', 'usampler2DMS', 'sampler2DMSArray',
	'isampler2DMSArray', 'usampler2DMSArray', 'samplerCubeArray', 'samplerCubeArrayShadow',
	'isamplerCubeArray', 'usamplerCubeArray', 'image1D', 'iimage1D', 'uimage1D', 'image2D',
	'iimage2D', 'uimage2D', 'image3D', 'iimage3D', 'uimage3D', 'image2DRect', 'iimage2DRect',
	'uimage2DRect', 'imageCube', 'iimageCube', 'uimageCube', 'imageBuffer', 'iimageBuffer',
	'uimageBuffer', 'image1DArray', 'iimage1DArray', 'uimage1DArray', 'image2DArray', 'iimage2DArray',
	'uimage2DArray', 'imageCubeArray', 'iimageCubeArray', 'uimageCubeArray', 'image2DMS',
	'iimage2DMS', 'uimage2DMS', 'image2DMSArray', 'iimage2DMSArray', 'uimage2DMSArray',
	-- Reserved.
	'long', 'short', 'half', 'fixed', 'unsigned', 'hvec2', 'hvec3', 'hvec4', 'fvec2', 'fvec3',
	'fvec4', 'sampler3DRect'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'radians', 'degrees', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'sinh', 'cosh', 'tanh',
	'asinh', 'acosh', 'atanh', 'pow', 'exp', 'log', 'exp2', 'log2', 'sqrt', 'inversesqrt', 'abs',
	'sign', 'floor', 'trunc', 'round', 'roundEven', 'ceil', 'fract', 'mod', 'modf', 'min', 'max',
	'clamp', 'mix', 'step', 'smoothstep', 'isnan', 'isinf', 'floatBitsToInt', 'floatBitsToUint',
	'intBitsToFloat', 'uintBitsToFloat', 'fma', 'frexp', 'ldexp', 'packUnorm2x16', 'packUnorm4x8',
	'packSnorm4x8', 'unpackUnorm2x16', 'unpackUnorm4x8', 'unpackSnorm4x8', 'packDouble2x32',
	'unpackDouble2x32', 'length', 'distance', 'dot', 'cross', 'normalize', 'ftransform',
	'faceforward', 'reflect', 'refract', 'matrixCompMult', 'outerProduct', 'transpose', 'determinant',
	'inverse', 'lessThan', 'lessThanEqual', 'greaterThan', 'greaterThanEqual', 'equal', 'notEqual',
	'any', 'all', 'not', 'uaddCarry', 'usubBorrow', 'umulExtended', 'imulExtended', 'bitfieldExtract',
	'bitfildInsert', 'bitfieldReverse', 'bitCount', 'findLSB', 'findMSB', 'textureSize',
	'textureQueryLOD', 'texture', 'textureProj', 'textureLod', 'textureOffset', 'texelFetch',
	'texelFetchOffset', 'textureProjOffset', 'textureLodOffset', 'textureProjLod',
	'textureProjLodOffset', 'textureGrad', 'textureGradOffset', 'textureProjGrad',
	'textureProjGradOffset', 'textureGather', 'textureGatherOffset', 'texture1D', 'texture2D',
	'texture3D', 'texture1DProj', 'texture2DProj', 'texture3DProj', 'texture1DLod', 'texture2DLod',
	'texture3DLod', 'texture1DProjLod', 'texture2DProjLod', 'texture3DProjLod', 'textureCube',
	'textureCubeLod', 'shadow1D', 'shadow2D', 'shadow1DProj', 'shadow2DProj', 'shadow1DLod',
	'shadow2DLod', 'shadow1DProjLod', 'shadow2DProjLod', 'dFdx', 'dFdy', 'fwidth',
	'interpolateAtCentroid', 'interpolateAtSample', 'interpolateAtOffset', 'noise1', 'noise2',
	'noise3', 'noise4', 'EmitStreamVertex', 'EndStreamPrimitive', 'EmitVertex', 'EndPrimitive',
	'barrier'
})

lex:set_word_list(lexer.VARIABLE, {
	'gl_VertexID', 'gl_InstanceID', 'gl_Position', 'gl_PointSize', 'gl_ClipDistance',
	'gl_PrimitiveIDIn', 'gl_InvocationID', 'gl_PrimitiveID', 'gl_Layer', 'gl_PatchVerticesIn',
	'gl_TessLevelOuter', 'gl_TessLevelInner', 'gl_TessCoord', 'gl_FragCoord', 'gl_FrontFacing',
	'gl_PointCoord', 'gl_SampleID', 'gl_SamplePosition', 'gl_FragColor', 'gl_FragData',
	'gl_FragDepth', 'gl_SampleMask', 'gl_ClipVertex', 'gl_FrontColor', 'gl_BackColor',
	'gl_FrontSecondaryColor', 'gl_BackSecondaryColor', 'gl_TexCoord', 'gl_FogFragCoord', 'gl_Color',
	'gl_SecondaryColor', 'gl_Normal', 'gl_Vertex', 'gl_MultiTexCoord0', 'gl_MultiTexCoord1',
	'gl_MultiTexCoord2', 'gl_MultiTexCoord3', 'gl_MultiTexCoord4', 'gl_MultiTexCoord5',
	'gl_MultiTexCoord6', 'gl_MultiTexCoord7', 'gl_FogCoord'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'__LINE__', '__FILE__', '__VERSION__', --
	'gl_MaxVertexAttribs', 'gl_MaxVertexUniformComponents', 'gl_MaxVaryingFloats',
	'gl_MaxVaryingComponents', 'gl_MaxVertexOutputComponents', 'gl_MaxGeometryInputComponents',
	'gl_MaxGeometryOutputComponents', 'gl_MaxFragmentInputComponents',
	'gl_MaxVertexTextureImageUnits', 'gl_MaxCombinedTextureImageUnits', 'gl_MaxTextureImageUnits',
	'gl_MaxFragmentUniformComponents', 'gl_MaxDrawBuffers', 'gl_MaxClipDistances',
	'gl_MaxGeometryTextureImageUnits', 'gl_MaxGeometryOutputVertices',
	'gl_MaxGeometryTotalOutputComponents', 'gl_MaxGeometryUniformComponents',
	'gl_MaxGeometryVaryingComponents', 'gl_MaxTessControlInputComponents',
	'gl_MaxTessControlOutputComponents', 'gl_MaxTessControlTextureImageUnits',
	'gl_MaxTessControlUniformComponents', 'gl_MaxTessControlTotalOutputComponents',
	'gl_MaxTessEvaluationInputComponents', 'gl_MaxTessEvaluationOutputComponents',
	'gl_MaxTessEvaluationTextureImageUnits', 'gl_MaxTessEvaluationUniformComponents',
	'gl_MaxTessPatchComponents', 'gl_MaxPatchVertices', 'gl_MaxTessGenLevel', 'gl_MaxTextureUnits',
	'gl_MaxTextureCoords', 'gl_MaxClipPlanes', --
	'gl_DepthRange', 'gl_ModelViewMatrix', 'gl_ProjectionMatrix', 'gl_ModelViewProjectionMatrix',
	'gl_TextureMatrix', 'gl_NormalMatrix', 'gl_ModelViewMatrixInverse', 'gl_ProjectionMatrixInverse',
	'gl_ModelViewProjectionMatrixInverse', 'gl_TextureMatrixInverse', 'gl_ModelViewMatrixTranspose',
	'gl_ProjectionMatrixTranspose', 'gl_ModelViewProjectionMatrixTranspose',
	'gl_TextureMatrixTranspose', 'gl_ModelViewMatrixInverseTranspose',
	'gl_ProjectionMatrixInverseTranspose', 'gl_ModelViewProjectionMatrixInverseTranspose',
	'gl_TextureMatrixInverseTranspose', 'gl_NormalScale', 'gl_ClipPlane', 'gl_Point',
	'gl_FrontMaterial', 'gl_BackMaterial', 'gl_LightSource', 'gl_LightModel',
	'gl_FrontLightModelProduct', 'gl_BackLightModelProduct', 'gl_FrontLightProduct',
	'gl_BackLightProduct', 'gl_TextureEnvColor', 'gl_EyePlaneS', 'gl_EyePlaneT', 'gl_EyePlaneR',
	'gl_EyePlaneQ', 'gl_ObjectPlaneS', 'gl_ObjectPlaneT', 'gl_ObjectPlaneR', 'gl_ObjectPlaneQ',
	'gl_Fog'
})

lex:set_word_list(lexer.PREPROCESSOR, 'extension version', true)

lexer.property['scintillua.comment'] = '//'

return lex
