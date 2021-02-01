#pragma once

#ifdef __cplusplus

#include <vector>
#include <istream>
#include <map>
#include <QD3D.h>
#include "Utilities/BigEndianIStream.h"

struct Q3MetaFile_Texture
{
	static const uint32_t			COOKIE = 'TXTR';

	uint32_t						cookie = COOKIE;
	uint32_t						useMipMapping;
	uint32_t						pixelType;
	uint32_t						bitOrder;
	uint32_t						byteOrder;
	uint32_t						width;
	uint32_t						height;
	uint32_t						rowBytes;
	uint32_t						offset;
	std::vector<unsigned char>		buffer;
	uint32_t						textureName;

	~Q3MetaFile_Texture()
	{
		cookie = 'DEAD';
	}
};

/*
struct Q3MetaFile_TriMesh
{
	static const uint32_t			COOKIE = 'TMSH';

	uint32_t						cookie = COOKIE;
	std::vector<TQ3TriMeshTriangleData> triangles;
	std::vector<TQ3Point3D>			points;
	std::vector<TQ3Param2D>			vertexUVs;
	std::vector<TQ3Vector3D>		vertexNormals;
	std::vector<TQ3ColorRGBA>		vertexColors;
	TQ3BoundingBox					boundingBox;
	std::vector<uint32_t>			textures;
	bool							hasTransparency;

	~Q3MetaFile_TriMesh()
	{
		cookie = 'DEAD';
	}
};
*/

struct Q3MetaFile
{
	static const uint32_t			COOKIE = '3DMF';

	uint32_t						cookie = COOKIE;
	std::vector<Q3MetaFile_Texture>	textures;
	std::vector<TQ3TriMeshData*>	meshes;
	std::vector<std::vector<TQ3TriMeshData*> > topLevelMeshGroups;

	~Q3MetaFile()
	{
		cookie = 'DEAD';
	}
};

class Q3MetaFileParser
{
public:
	Q3MetaFileParser(std::istream& input, Q3MetaFile& dest);
	void Parse3DMF();

private:
	// Returns fourcc of parsed chunk
	uint32_t Parse1Chunk();

	// Parse trimesh chunk
	void Parse_tmsh(uint32_t chunkSize);

	// Parse attribute array chunk
	void Parse_atar(uint32_t chunkSize);

	// Parse mipmap texture chunk
	uint32_t Parse_txmm(uint32_t chunkSize);

	Q3MetaFile& metaFile;
	std::istream& baseStream;
	Pomme::BigEndianIStream f;

	int currentDepth = 0;

	TQ3TriMeshData* currentMesh;
	int numTexturesDumpedToTGA = 0;

	std::map<uint32_t, uint64_t> referenceTOC;
	std::map<std::streampos, uint32_t> knownTextures;
};

#endif // __cplusplus
