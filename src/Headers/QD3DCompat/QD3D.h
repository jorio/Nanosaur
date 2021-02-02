#pragma once

#include <SDL_opengl.h>
#include <stdbool.h>


typedef float		TQ3Float32;
typedef uint32_t	TQ3Uns32;
typedef int32_t		TQ3ObjectType;

// junk types
typedef void*		TQ3TransformObject;
typedef void*		TQ3Object;
typedef void*		TQ3GroupPosition;
typedef void*		TQ3GeometryObject;
typedef void*		TQ3ViewObject;
typedef void*		TQ3SurfaceShaderObject;
typedef void*		TQ3StorageObject;
typedef void*		TQ3ShaderObject;
typedef void*		TQ3FileObject;
typedef void*		TQ3TriMeshAttributeData;

typedef enum
{
	kQ3False                                    = 0,
	kQ3True                                     = 1,
	kQ3BooleanSize32                            = 0xFFFFFFFF
} TQ3Boolean;

typedef enum
{
	kQ3Off                                      = 0,
	kQ3On                                       = 1,
	kQ3SwitchSize32                             = 0xFFFFFFFF
} TQ3Switch;

typedef enum
{
	kQ3Failure                                  = 0,
	kQ3Success                                  = 1,
	kQ3StatusSize32                             = 0xFFFFFFFF
} TQ3Status;


enum TQ3AttributeTypes
{
	kQ3AttributeTypeNone                        = 0,            // N/A
	kQ3AttributeTypeSurfaceUV                   = 1,            // TQ3Param2D
	kQ3AttributeTypeShadingUV                   = 2,            // TQ3Param2D
	kQ3AttributeTypeNormal                      = 3,            // TQ3Vector3D
	kQ3AttributeTypeAmbientCoefficient          = 4,            // float
	kQ3AttributeTypeDiffuseColor                = 5,            // TQ3ColorRGB
	kQ3AttributeTypeSpecularColor               = 6,            // TQ3ColorRGB
	kQ3AttributeTypeSpecularControl             = 7,            // float
	kQ3AttributeTypeTransparencyColor           = 8,            // TQ3ColorRGB
	kQ3AttributeTypeSurfaceTangent              = 9,            // TQ3Tangent2D
	kQ3AttributeTypeHighlightState              = 10,           // TQ3Switch
	kQ3AttributeTypeSurfaceShader               = 11,           // TQ3SurfaceShaderObject
	kQ3AttributeTypeEmissiveColor               = 12,           // TQ3ColorRGB
	kQ3AttributeTypeNumTypes                    = 13,           // N/A
	kQ3AttributeTypeSize32                      = 0xFFFFFFFF
};

typedef enum
{
	/// 8 bits for red, green, and blue. High-order byte ignored.
	kQ3PixelTypeRGB32                           = 0,

	/// 8 bits for alpha, red, green, and blue.
	kQ3PixelTypeARGB32                          = 1,

	/// 5 bits for red, green, and blue. High-order bit ignored.
	kQ3PixelTypeRGB16                           = 2,

	/// 1 bit for alpha. 5 bits for red, green, and blue.
	kQ3PixelTypeARGB16                          = 3,

	/// 5 bits for red, 6 bits for green, 5 bits for blue.
	kQ3PixelTypeRGB16_565                       = 4,

	/// 8 bits for red, green, and blue. No alpha byte.
	kQ3PixelTypeRGB24                           = 5,

	kQ3PixelTypeUnknown							= 200,
	kQ3PixelTypeSize32                          = 0xFFFFFFFF
} TQ3PixelType;

typedef enum TQ3InterpolationStyle
{
	kQ3InterpolationStyleNone                   = 0,
	kQ3InterpolationStyleVertex                 = 1,
	kQ3InterpolationStylePixel                  = 2,
	kQ3InterpolationSize32                      = 0xFFFFFFFF
} TQ3InterpolationStyle;

typedef enum TQ3BackfacingStyle
{
	kQ3BackfacingStyleBoth                      = 0,
	kQ3BackfacingStyleRemove                    = 1,
	kQ3BackfacingStyleFlip                      = 2,
	kQ3BackfacingStyleRemoveFront				= 3,
	kQ3BackfacingStyleSize32                    = 0xFFFFFFFF
} TQ3BackfacingStyle;

typedef enum TQ3FillStyle
{
	kQ3FillStyleFilled                          = 0,
	kQ3FillStyleEdges                           = 1,
	kQ3FillStylePoints                          = 2,
	kQ3FillStyleSize32                          = 0xFFFFFFFF
} TQ3FillStyle;

typedef enum TQ3FogMode
{
	kQ3FogModeLinear                            = 0,
	kQ3FogModeExponential                       = 1,
	kQ3FogModeExponentialSquared                = 2,
	kQ3FogModeAlpha                             = 3,
	kQ3FogModePlaneBasedLinear                  = 4,
	kQ3FogModeSize32                            = 0xFFFFFFFF
} TQ3FogMode;

typedef enum
{
	kQ3EndianBig                                = 0,
	kQ3EndianLittle                             = 1,
	kQ3EndianSize32                             = 0xFFFFFFFF
} TQ3Endian;

typedef struct TQ3Param2D
{
	float					u;
	float					v;
} TQ3Param2D;

typedef struct TQ3Point2D
{
	float					x;
	float					y;
} TQ3Point2D;

typedef struct TQ3Area
{
	TQ3Point2D				min;
	TQ3Point2D				max;
} TQ3Area;

typedef struct TQ3Point3D
{
	float					x;
	float					y;
	float					z;
} TQ3Point3D;

typedef struct TQ3Vector2D
{
	float					x;
	float					y;
} TQ3Vector2D;

typedef struct TQ3Vector3D
{
	float					x;
	float					y;
	float					z;
} TQ3Vector3D;

typedef struct TQ3RationalPoint3D
{
	float					x;
	float					y;
	float					w;
} TQ3RationalPoint3D;

typedef struct TQ3RationalPoint4D
{
	float					x;
	float					y;
	float					z;
	float					w;
} TQ3RationalPoint4D;

typedef struct TQ3ColorARGB
{
	float					r;
	float					g;
	float					b;
	float					a;
} TQ3ColorARGB;

typedef struct TQ3ColorRGBA
{
	float					r;
	float					g;
	float					b;
	float					a;
} TQ3ColorRGBA;

typedef struct TQ3ColorRGB
{
	float					r;
	float					g;
	float					b;
} TQ3ColorRGB;

typedef struct TQ3Vertex3D
{
	TQ3Point3D				point;
} TQ3Vertex3D;

typedef struct TQ3BoundingBox
{
	TQ3Point3D				min;
	TQ3Point3D				max;
	TQ3Boolean				isEmpty;
} TQ3BoundingBox;

typedef struct TQ3Matrix3x3
{
	float					value[3][3];
} TQ3Matrix3x3;

typedef struct TQ3Matrix4x4
{
	float					value[4][4];
} TQ3Matrix4x4;

typedef struct TQ3PlaneEquation
{
	TQ3Vector3D                                 normal;
	float                                       constant;
} TQ3PlaneEquation;

typedef struct TQ3TriMeshTriangleData
{
	uint32_t				pointIndices[3];
} TQ3TriMeshTriangleData;

// This structure differs from QD3D.
typedef struct TQ3TriMeshData
{
	uint32_t									numTriangles;
	TQ3TriMeshTriangleData						*triangles;

	uint32_t									numPoints;
	TQ3Point3D									*points;
	TQ3Param2D									*vertexUVs;
	TQ3Vector3D									*vertexNormals;
	TQ3ColorRGBA								*vertexColors;

	TQ3BoundingBox								bBox;

	bool										hasTransparency;
	bool										hasTexture;
	uint32_t									internalTextureID;
} TQ3TriMeshData;

// This structure does not exist in QD3D.
typedef struct TQ3TriMeshFlatGroup
{
	int											numMeshes;
	TQ3TriMeshData**							meshes;
} TQ3TriMeshFlatGroup;

typedef struct TQ3FogStyleData
{
	TQ3Switch                                   state;
	TQ3FogMode                                  mode;
	float                                       fogStart;
	float                                       fogEnd;
	float                                       density;
	TQ3ColorARGB                                color;
} TQ3FogStyleData;

typedef struct TQ3CameraPlacement
{
	TQ3Point3D                                  cameraLocation;
	TQ3Point3D                                  pointOfInterest;
	TQ3Vector3D                                 upVector;
} TQ3CameraPlacement;
