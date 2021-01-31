#pragma once

#include <SDL_opengl.h>


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

/*
struct
{
	TQ3ObjectType	objectType;
	union
	{
		TQ3
	};
};
 */

enum {
    kQ3ObjectTypeInvalid                        = ((TQ3ObjectType) 0),
    kQ3ObjectTypeView                           = 'view',
    kQ3ObjectTypeViewer                         = 'vwer',
    kQ3ObjectTypeSlab                           = 'slab',
    kQ3ObjectTypeElement                        = 'elmn',
        kQ3ElementTypeAttribute                 = 'eatt',
    kQ3ObjectTypePick                           = 'pick',
        kQ3PickTypeWindowPoint                  = 'pkwp',
        kQ3PickTypeWindowRect                   = 'pkwr',
        kQ3PickTypeWorldRay                     = 'pkry',
    kQ3ObjectTypeShared                         = 'shrd',
        kQ3SharedTypeRenderer                   = 'rddr',
            kQ3RendererTypeWireFrame            = 'wrfr',
            kQ3RendererTypeGeneric              = 'gnrr',
            kQ3RendererTypeInteractive          = 'ctwn',
            kQ3RendererTypeOpenGL               = 'oglr',
            kQ3RendererTypeCartoon              = 'toon',
            kQ3RendererTypeHiddenLine           = 'hdnl',
        kQ3SharedTypeShape                      = 'shap',
            kQ3ShapeTypeGeometry                = 'gmtr',
                kQ3GeometryTypeBox              = 'box ',
                kQ3GeometryTypeGeneralPolygon   = 'gpgn',
                kQ3GeometryTypeLine             = 'line',
                kQ3GeometryTypeMarker           = 'mrkr',
                kQ3GeometryTypePixmapMarker     = 'mrkp',
                kQ3GeometryTypeMesh             = 'mesh',
                kQ3GeometryTypeNURBCurve        = 'nrbc',
                kQ3GeometryTypeNURBPatch        = 'nrbp',
                kQ3GeometryTypePoint            = 'pnt ',
                kQ3GeometryTypePolygon          = 'plyg',
                kQ3GeometryTypePolyLine         = 'plyl',
                kQ3GeometryTypeTriangle         = 'trng',
                kQ3GeometryTypeTriGrid          = 'trig',
                kQ3GeometryTypeCone             = 'cone',
                kQ3GeometryTypeCylinder         = 'cyln',
                kQ3GeometryTypeDisk             = 'disk',
                kQ3GeometryTypeEllipse          = 'elps',
                kQ3GeometryTypeEllipsoid        = 'elpd',
                kQ3GeometryTypePolyhedron       = 'plhd',
                kQ3GeometryTypeTorus            = 'tors',
                kQ3GeometryTypeTriMesh          = 'tmsh',
                kQ3GeometryTypeNakedTriMesh     = 'ntms',
            kQ3ShapeTypeShader                  = 'shdr',
                kQ3ShaderTypeSurface            = 'sush',
                    kQ3SurfaceShaderTypeTexture = 'txsu',
                kQ3ShaderTypeIllumination       = 'ilsh',
                    kQ3IlluminationTypePhong    = 'phil',
                    kQ3IlluminationTypeLambert  = 'lmil',
                    kQ3IlluminationTypeNULL     = 'nuil',
                    kQ3IlluminationTypeNondirectional = 'ndil',
            kQ3ShapeTypeStyle                   = 'styl',
                kQ3StyleTypeBackfacing          = 'bckf',
                kQ3StyleTypeInterpolation       = 'intp',
                kQ3StyleTypeFill                = 'fist',
                kQ3StyleTypePickID              = 'pkid',
//#if QUESA_ALLOW_QD3D_EXTENSIONS
                kQ3StyleTypeCastShadows         = 'cash',
                kQ3StyleTypeLineWidth           = 'lnwd',
				kQ3StyleTypeBlending			= 'blnd',
				kQ3StyleTypeZWriteTransparency	= 'zwrt',
//#endif // QUESA_ALLOW_QD3D_EXTENSIONS
                kQ3StyleTypeReceiveShadows      = 'rcsh',
                kQ3StyleTypeHighlight           = 'high',
                kQ3StyleTypeSubdivision         = 'sbdv',
                kQ3StyleTypeOrientation         = 'ofdr',
                kQ3StyleTypePickParts           = 'pkpt',
                kQ3StyleTypeAntiAlias           = 'anti',
                kQ3StyleTypeFog                 = 'fogg',
                kQ3StyleTypeFogExtended         = 'fogx',
           kQ3ShapeTypeTransform                = 'xfrm',
                kQ3TransformTypeMatrix          = 'mtrx',
                kQ3TransformTypeScale           = 'scal',
                kQ3TransformTypeTranslate       = 'trns',
                kQ3TransformTypeRotate          = 'rott',
                kQ3TransformTypeRotateAboutPoint= 'rtap',
                kQ3TransformTypeRotateAboutAxis = 'rtaa',
                kQ3TransformTypeQuaternion      = 'qtrn',
                kQ3TransformTypeReset           = 'rset',
//#if QUESA_ALLOW_QD3D_EXTENSIONS
                kQ3TransformTypeCamera          = 'camt',
                kQ3TransformTypeCameraRasterize = 'rast',
//#endif // QUESA_ALLOW_QD3D_EXTENSIONS
            kQ3ShapeTypeLight                   = 'lght',
                kQ3LightTypeAmbient             = 'ambn',
                kQ3LightTypeDirectional         = 'drct',
                kQ3LightTypePoint               = 'pntl',
                kQ3LightTypeSpot                = 'spot',
            kQ3ShapeTypeCamera                  = 'cmra',
                kQ3CameraTypeOrthographic       = 'orth',
                kQ3CameraTypeViewPlane          = 'vwpl',
                kQ3CameraTypeViewAngleAspect    = 'vana',
                kQ3CameraTypeAllSeeing          = 'alse',
                kQ3CameraTypeFisheye            = 'fish',
            kQ3ShapeTypeStateOperator           = 'stop',
                kQ3StateOperatorTypePush        = 'push',
                kQ3StateOperatorTypePop         = 'pop ',
            kQ3ShapeTypeGroup                   = 'grup',
                kQ3GroupTypeDisplay             = 'dspg',
                    kQ3DisplayGroupTypeOrdered  = 'ordg',
                    kQ3DisplayGroupTypeIOProxy  = 'iopx',
                kQ3GroupTypeLight               = 'lghg',
                kQ3GroupTypeInfo                = 'info',
            kQ3ShapeTypeUnknown                 = 'unkn',
                kQ3UnknownTypeText              = 'uktx',
                kQ3UnknownTypeBinary            = 'ukbn',
            kQ3ShapeTypeReference               = 'rfrn',
                kQ3ReferenceTypeExternal        = 'rfex',
        kQ3SharedTypeSet                        = 'set ',
            kQ3SetTypeAttribute                 = 'attr',
        kQ3SharedTypeDrawContext                = 'dctx',
            kQ3DrawContextTypePixmap            = 'dpxp',
            kQ3DrawContextTypeMacintosh         = 'dmac',
            kQ3DrawContextTypeCocoa             = 'dcco',
            kQ3DrawContextTypeWin32DC           = 'dw32',
            kQ3DrawContextTypeSDL               = 'dsdl',
            kQ3DrawContextTypeDDSurface         = 'ddds',
            kQ3DrawContextTypeX11               = 'dx11',
        kQ3SharedTypeTexture                    = 'txtr',
            kQ3TextureTypePixmap                = 'txpm',
            kQ3TextureTypeMipmap                = 'txmm',
            kQ3TextureTypeCompressedPixmap      = 'txcp',
        kQ3SharedTypeFile                       = 'file',
        kQ3SharedTypeStorage                    = 'strg',
            kQ3StorageTypeMemory                = 'mems',
                kQ3MemoryStorageTypeHandle      = 'hndl',
            kQ3StorageTypePath                  = 'Qstp',
            kQ3StorageTypeFileStream            = 'Qsfs',
            kQ3StorageTypeUnix                  = 'uxst',
                kQ3UnixStorageTypePath          = 'unix',
            kQ3StorageTypeMacintosh             = 'macn',
                kQ3MacintoshStorageTypeFSSpec   = 'macp',
            kQ3StorageTypeWin32                 = 'wist',
        kQ3SharedTypeString                     = 'strn',
            kQ3StringTypeCString                = 'strc',
        kQ3SharedTypeShapePart                  = 'sprt',
            kQ3ShapePartTypeMeshPart            = 'spmh',
                kQ3MeshPartTypeMeshFacePart     = 'mfac',
                kQ3MeshPartTypeMeshEdgePart     = 'medg',
                kQ3MeshPartTypeMeshVertexPart   = 'mvtx',
        kQ3SharedTypeControllerState            = 'ctst',
        kQ3SharedTypeTracker                    = 'trkr',
        kQ3SharedTypeViewHints                  = 'vwhn',
        kQ3SharedTypeEndGroup                   = 'endg'
};


typedef enum {
	kQ3False                                    = 0,
	kQ3True                                     = 1,
	kQ3BooleanSize32                            = 0xFFFFFFFF
} TQ3Boolean;

typedef enum {
	kQ3Off                                      = 0,
	kQ3On                                       = 1,
	kQ3SwitchSize32                             = 0xFFFFFFFF
} TQ3Switch;

typedef enum {
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

typedef enum {
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

typedef enum TQ3InterpolationStyle {
	kQ3InterpolationStyleNone                   = 0,
	kQ3InterpolationStyleVertex                 = 1,
	kQ3InterpolationStylePixel                  = 2,
	kQ3InterpolationSize32                      = 0xFFFFFFFF
} TQ3InterpolationStyle;

typedef enum TQ3BackfacingStyle {
	kQ3BackfacingStyleBoth                      = 0,
	kQ3BackfacingStyleRemove                    = 1,
	kQ3BackfacingStyleFlip                      = 2,
	kQ3BackfacingStyleRemoveFront				= 3,
	kQ3BackfacingStyleSize32                    = 0xFFFFFFFF
} TQ3BackfacingStyle;

typedef enum TQ3FillStyle {
	kQ3FillStyleFilled                          = 0,
	kQ3FillStyleEdges                           = 1,
	kQ3FillStylePoints                          = 2,
	kQ3FillStyleSize32                          = 0xFFFFFFFF
} TQ3FillStyle;

typedef enum TQ3FogMode {
	kQ3FogModeLinear                            = 0,
	kQ3FogModeExponential                       = 1,
	kQ3FogModeExponentialSquared                = 2,
	kQ3FogModeAlpha                             = 3,
	kQ3FogModePlaneBasedLinear                  = 4,
	kQ3FogModeSize32                            = 0xFFFFFFFF
} TQ3FogMode;

typedef enum {
	kQ3EndianBig                                = 0,
	kQ3EndianLittle                             = 1,
	kQ3EndianSize32                             = 0xFFFFFFFF
} TQ3Endian;



typedef struct
{
	float					u;
	float					v;
} TQ3Param2D;

typedef struct
{
	float					x;
	float					y;
} TQ3Point2D;

typedef struct TQ3Area {
	TQ3Point2D				min;
	TQ3Point2D				max;
} TQ3Area;

typedef struct
{
	float					x;
	float					y;
	float					z;
} TQ3Point3D;

typedef struct
{
	float					x;
	float					y;
} TQ3Vector2D;

typedef struct
{
	float					x;
	float					y;
	float					z;
} TQ3Vector3D;

typedef struct TQ3RationalPoint3D {
	float					x;
	float					y;
	float					w;
} TQ3RationalPoint3D;

typedef struct TQ3RationalPoint4D {
	float					x;
	float					y;
	float					z;
	float					w;
} TQ3RationalPoint4D;

typedef struct
{
	float					r;
	float					g;
	float					b;
	float					a;
} TQ3ColorARGB;

typedef struct
{
	float					r;
	float					g;
	float					b;
	float					a;
} TQ3ColorRGBA;

typedef struct
{
	float					r;
	float					g;
	float					b;
} TQ3ColorRGB;

typedef struct
{
	TQ3Point3D				point;
} TQ3Vertex3D;

typedef struct
{
	TQ3Point3D				min;
	TQ3Point3D				max;
	TQ3Boolean				isEmpty;
} TQ3BoundingBox;

typedef struct
{
	float					value[3][3];
} TQ3Matrix3x3;

typedef struct
{
	float					value[4][4];
} TQ3Matrix4x4;

typedef struct
{
	TQ3Vector3D                                 normal;
	float                                       constant;
} TQ3PlaneEquation;

typedef struct
{
	uint32_t				pointIndices[3];
} TQ3TriMeshTriangleData;

typedef struct
{
	// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//	TQ3AttributeSet _Nullable                   triMeshAttributeSet;

	uint32_t                                    numTriangles;
	TQ3TriMeshTriangleData                      * triangles;

	uint32_t                                    numTriangleAttributeTypes;
//	TQ3TriMeshAttributeData                     * _Nullable triangleAttributeTypes;

	uint32_t                                    numEdges;
//	TQ3TriMeshEdgeData                          * _Nullable edges;

	uint32_t                                    numEdgeAttributeTypes;
//	TQ3TriMeshAttributeData                     * _Nullable edgeAttributeTypes;

	uint32_t                                    numPoints;
	TQ3Point3D                                  * points;

	uint32_t                                    numVertexAttributeTypes;
//	TQ3TriMeshAttributeData                     * _Nullable vertexAttributeTypes;

	TQ3BoundingBox                              bBox;
} TQ3TriMeshData;

typedef struct TQ3FogStyleData {
	TQ3Switch                                   state;
	TQ3FogMode                                  mode;
	float                                       fogStart;
	float                                       fogEnd;
	float                                       density;
	TQ3ColorARGB                                color;
} TQ3FogStyleData;

typedef struct TQ3CameraPlacement {
	TQ3Point3D                                  cameraLocation;
	TQ3Point3D                                  pointOfInterest;
	TQ3Vector3D                                 upVector;
} TQ3CameraPlacement;
