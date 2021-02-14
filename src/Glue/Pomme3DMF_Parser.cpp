#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"
#include "PommeDebug.h"
#include "Pomme.h"

#define printf(...) do{}while(0)

class Q3MetaFile_EarlyEOFException : public std::exception
{
public:
	const char *what() const noexcept override
	{
		return "Early EOF in 3DMF";
	}
};

static void Assert(bool condition, const char* message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

template<typename T>
static void ReadTriangleVertexIndices(Pomme::BigEndianIStream& f, int numTriangles, TQ3TriMeshData* currentMesh)
{
	for (int i = 0; i < numTriangles; i++)
	{
		T v0 = f.Read<T>();
		T v1 = f.Read<T>();
		T v2 = f.Read<T>();
		currentMesh->triangles[i] = {v0, v1, v2};
	}
}

Q3MetaFileParser::Q3MetaFileParser(std::istream& theBaseStream, TQ3MetaFile& dest)
		: metaFile(dest)
		, baseStream(theBaseStream)
		, f(baseStream)
		, currentMesh(nullptr)
{
}

uint32_t Q3MetaFileParser::Parse1Chunk()
{
	Assert(currentDepth >= 0, "depth underflow");

	//-----------------------------------------------------------
	// Get current chunk offset, type, size

	uint32_t chunkOffset	= f.Tell();
	uint32_t chunkType		= f.Read<uint32_t>();
	uint32_t chunkSize		= f.Read<uint32_t>();

	std::string myChunk = Pomme::FourCCString(chunkType);
	const char* myChunkC = myChunk.c_str();

	printf("\n%d-%08x ", currentDepth, chunkOffset);
	for (int i = 0; i < 1 + currentDepth - (chunkType=='endg'?1:0); i++)
		printf("\t");
	printf("%s\t", Pomme::FourCCString(chunkType).c_str());
	fflush(stdout);

	//-----------------------------------------------------------
	// Process current chunk

	switch (chunkType)
	{
		case 0:		// Happens in Diloph_Fin.3df at 0x00014233 -- signals early EOF? or corrupted file? either way, stop parsing.
			throw Q3MetaFile_EarlyEOFException();

		case 'cntr':    // Container
		{
			if (currentDepth == 1)
				__Q3EnlargeArray<TQ3TriMeshFlatGroup>(metaFile.topLevelGroups, metaFile.numTopLevelGroups, 'GLST');

			currentDepth++;
			auto limit = f.Tell() + (std::streamoff) chunkSize;
			while (f.Tell() != limit)
				Parse1Chunk();
			currentDepth--;
			currentMesh = nullptr;
			break;
		}

		case 'bgng':
			if (currentDepth == 1)
				__Q3EnlargeArray<TQ3TriMeshFlatGroup>(metaFile.topLevelGroups, metaFile.numTopLevelGroups, 'GLST');
			currentDepth++;
			f.Skip(chunkSize);		// bgng itself typically contains dspg, dgst
			while ('endg' != Parse1Chunk())
				;
			currentDepth--;
			currentMesh = nullptr;
			break;

		case 'endg':
			Assert(chunkSize == 0, "illegal endg size");
//			Assert(containerEnd == 0, "stray endg");
//			containerEnd = f.Tell();  // force while loop to stop
			break;

		case 'tmsh':    // TriMesh
		{
			Assert(!currentMesh, "nested meshes not supported");
			Parse_tmsh(chunkSize);
			Assert(currentMesh, "currentMesh wasn't get set by Parse_tmsh?");

			if (metaFile.numTopLevelGroups == 0)
				__Q3EnlargeArray<TQ3TriMeshFlatGroup>(metaFile.topLevelGroups, metaFile.numTopLevelGroups, 'GLST');

			TQ3TriMeshFlatGroup* group = &metaFile.topLevelGroups[metaFile.numTopLevelGroups-1];
			__Q3EnlargeArray(group->meshes, group->numMeshes, 'GMSH');
			group->meshes[group->numMeshes-1] = currentMesh;
			break;
		}

		case 'atar':    // AttributeArray
			Parse_atar(chunkSize);
			break;

		case 'attr':    // AttributeSet
			Assert(chunkSize == 0, "illegal attr size");
			break;

		case 'kdif':    // Diffuse Color
			Assert(chunkSize == 12, "illegal kdif size");
			Assert(currentMesh, "stray kdif");
			{
				static_assert(sizeof(float) == 4);
				currentMesh->diffuseColor.r = f.Read<float>();
				currentMesh->diffuseColor.g = f.Read<float>();
				currentMesh->diffuseColor.b = f.Read<float>();
			}
			break;

		case 'kxpr':    // Transparency Color
			Assert(chunkSize == 12, "illegal kxpr size");
			Assert(currentMesh, "stray kxpr");
			{
				static_assert(sizeof(float) == 4);
				float r	= f.Read<float>();
				float g	= f.Read<float>();
				float b	= f.Read<float>();
				float a = r;
				printf("%.3f %.3f %.3f\t", r, g, b);
				Assert(r == g && g == b, "kxpr: expecting all components to be equal");
				currentMesh->diffuseColor.a = a;
			}
			break;

		case 'txsu':    // TextureShader
			Assert(chunkSize == 0, "illegal txsu size");
			break;

		case 'txmm':    // MipmapTexture
		{
			uint32_t internalTextureID;
			if (knownTextures.find(chunkOffset) != knownTextures.end())
			{
				printf("Texture already seen!");
				internalTextureID = knownTextures[chunkOffset];
				f.Skip(chunkSize);
			}
			else
			{
				internalTextureID = Parse_txmm(chunkSize);
				knownTextures[chunkOffset] = internalTextureID;
			}

			if (currentMesh)
			{
				Assert(!currentMesh->hasTexture, "txmm: current mesh already has a texture");
				currentMesh->internalTextureID = internalTextureID;
				currentMesh->hasTexture = true;
			}

			break;
		}

		case 'rfrn':    // Reference (into TOC)
		{
			Assert(chunkSize == 4, "illegal rfrn size");
			uint32_t target = f.Read<uint32_t>();
			printf("TOC#%d -----> %08lx", target, referenceTOC.at(target));
			auto jumpBackTo = f.Tell();
			f.Goto(referenceTOC.at(target));
			Parse1Chunk();
			f.Goto(jumpBackTo);
			break;
		}

		case 'toc ':
			// Already read TOC at beginning
			f.Skip(chunkSize);
			break;

		default:
			throw std::runtime_error("unrecognized 3DMF chunk");
	}

	return chunkType;
}

void Q3MetaFileParser::Parse3DMF()
{
	baseStream.seekg(0, std::ios::end);
	std::streampos fileLength = baseStream.tellg();
	baseStream.seekg(0, std::ios::beg);

	Assert(f.Read<uint32_t>() == '3DMF', "Not a 3DMF file");
	Assert(f.Read<uint32_t>() == 16, "Bad header length");

	uint16_t versionMajor = f.Read<uint16_t>();
	uint16_t versionMinor = f.Read<uint16_t>();

	uint32_t flags = f.Read<uint32_t>();
	Assert(flags == 0, "Database or Stream aren't supported");

	uint64_t tocOffset = f.Read<uint64_t>();
	printf("Version %d.%d    tocOffset %08lx\n", versionMajor, versionMinor, tocOffset);

	// Read TOC
	if (tocOffset != 0)
	{
		Pomme::StreamPosGuard rewindAfterTOC(baseStream);

		f.Goto(tocOffset);

		Assert('toc ' == f.Read<uint32_t>(), "Expecting toc magic here");
		uint32_t tocSize = f.Read<uint32_t>();
		uint64_t nextToc = f.Read<uint64_t>();
		uint32_t refSeed = f.Read<uint32_t>();
		uint32_t typeSeed = f.Read<uint32_t>();
		uint32_t tocEntryType = f.Read<uint32_t>();
		uint32_t tocEntrySize = f.Read<uint32_t>();
		uint32_t nEntries = f.Read<uint32_t>();

		Assert(tocEntryType == 1, "only QD3D 1.5 3DMF TOCs are recognized");
		Assert(tocEntrySize == 16, "incorrect tocEntrySize");

		for (uint32_t i = 0; i < nEntries;i++)
		{
			uint32_t refID = f.Read<uint32_t>();
			uint64_t objLocation = f.Read<uint64_t>();
			uint32_t objType = f.Read<uint32_t>();

			printf("TOC: refID %d '%s' at %08lx\n", refID, Pomme::FourCCString(objType).c_str(), objLocation);

			referenceTOC[refID] = objLocation;
		}
	}


	// Chunk Loop
	try
	{
		while (f.Tell() != fileLength)
		{
			Parse1Chunk();
		}
	}
	catch (Q3MetaFile_EarlyEOFException&)
	{
		// Stop parsing
		printf("Early EOF");
	}

	printf("\n");
}


void Q3MetaFileParser::Parse_tmsh(uint32_t chunkSize)
{
	Assert(chunkSize >= 52, "Illegal tmsh size");
	Assert(!currentMesh, "current mesh already set");

	uint32_t	numTriangles			= f.Read<uint32_t>();
	uint32_t	numTriangleAttributes	= f.Read<uint32_t>();
	uint32_t	numEdges				= f.Read<uint32_t>();
	uint32_t	numEdgeAttributes		= f.Read<uint32_t>();
	uint32_t	numVertices				= f.Read<uint32_t>();
	uint32_t	numVertexAttributes		= f.Read<uint32_t>();
	printf("%d tris, %d vertices\t", numTriangles, numVertices);
	Assert(0 == numEdges, "edges are not supported");
	Assert(0 == numEdgeAttributes, "edge attributes are not supported");

	currentMesh = Q3TriMeshData_New(numTriangles, numVertices);

	__Q3EnlargeArray(metaFile.meshes, metaFile.numMeshes, 'MLST');
	metaFile.meshes[metaFile.numMeshes-1] = currentMesh;

	// Triangles
	if (numVertices <= 0xFF)
	{
		ReadTriangleVertexIndices<uint8_t>(f, numTriangles, currentMesh);
	}
	else if (numVertices <= 0xFFFF)
	{
		ReadTriangleVertexIndices<uint16_t>(f, numTriangles, currentMesh);
	}
	else
	{
		static_assert(sizeof(TQ3TriMeshTriangleData::pointIndices[0]) == 2);
		Assert(false, "Meshes exceeding 65535 vertices are not supported");
		//ReadTriangleVertexIndices<uint32_t>(f, numTriangles, currentMesh);
	}

	// Ensure all vertex indices are in the expected range
	for (uint32_t i = 0; i < numTriangles; i++)
	{
		for (auto index : currentMesh->triangles[i].pointIndices)
		{
			Assert(index < numVertices, "3DMF parser: vertex index out of range");
		}
	}


	// Edges
	// (not supported yet)


	// Vertices
	for (uint32_t i = 0; i < numVertices; i++)
	{
		float x = f.Read<float>();
		float y = f.Read<float>();
		float z = f.Read<float>();
		//printf("%f %f %f\n", vertexX, vertexY, vertexZ);
		currentMesh->points[i] = {x, y, z};
	}

	// Bounding box
	{
		float xMin = f.Read<float>();
		float yMin = f.Read<float>();
		float zMin = f.Read<float>();
		float xMax = f.Read<float>();
		float yMax = f.Read<float>();
		float zMax = f.Read<float>();
		uint32_t emptyFlag = f.Read<uint32_t>();
		currentMesh->bBox.min = {xMin, yMin, zMin};
		currentMesh->bBox.max = {xMax, yMax, zMax};
		currentMesh->bBox.isEmpty = emptyFlag? kQ3True: kQ3False;
		//printf("%f %f %f - %f %f %f (empty? %d)\n", xMin, yMin, zMin, xMax, yMax, zMax, emptyFlag);
	}
}


void Q3MetaFileParser::Parse_atar(uint32_t chunkSize)
{
	Assert(chunkSize >= 20, "Illegal atar size");
	Assert(currentMesh, "no current mesh");

	uint32_t attributeType = f.Read<uint32_t>();
	Assert(0 == f.Read<uint32_t>(), "expected zero here");
	uint32_t positionOfArray = f.Read<uint32_t>();
	uint32_t positionInArray = f.Read<uint32_t>();		// what's that?
	uint32_t attributeUseFlag = f.Read<uint32_t>();

	Assert(attributeType >= 1 && attributeType < kQ3AttributeTypeNumTypes, "illegal attribute type");
	Assert(positionOfArray <= 2, "illegal position of array");
	Assert(attributeUseFlag <= 1, "unrecognized attribute use flag");


	bool isTriangleAttribute = positionOfArray == 0;
	bool isVertexAttribute = positionOfArray == 2;

	Assert(isTriangleAttribute || isVertexAttribute, "only face or vertex attributes are supported");

	if (isVertexAttribute && attributeType == kQ3AttributeTypeShadingUV)
	{
		printf("vertex UVs");
		Assert(currentMesh->vertexUVs, "current mesh has no vertex UV array");
		for (int i = 0; i < currentMesh->numPoints; i++)
		{
			float u = f.Read<float>();
			float v = f.Read<float>();
			currentMesh->vertexUVs[i] = {u, 1-v};
		}
	}
	else if (isVertexAttribute && attributeType == kQ3AttributeTypeNormal)
	{
		printf("vertex normals");
		Assert(positionInArray == 0, "PIA must be 0 for normals");
		Assert(currentMesh->vertexNormals, "current mesh has no vertex normal array");
		for (int i = 0; i < currentMesh->numPoints; i++)
		{
			currentMesh->vertexNormals[i].x = f.Read<float>();
			currentMesh->vertexNormals[i].y = f.Read<float>();
			currentMesh->vertexNormals[i].z = f.Read<float>();
		}
	}
	else if (isVertexAttribute && attributeType == kQ3AttributeTypeDiffuseColor)	// used in Bugdom's Global_Models2.3dmf
	{
		Assert(false, "per-vertex diffuse color not supported in Nanosaur");
#if 0
		printf("vertex diffuse");
//		Assert(positionInArray == 0, "PIA must be 0 for colors");
		Assert(currentMesh->vertexNormals, "current mesh has no vertex color array");
		for (int i = 0; i < currentMesh->numPoints; i++)
		{
			currentMesh->vertexColors[i].r = f.Read<float>();
			currentMesh->vertexColors[i].g = f.Read<float>();
			currentMesh->vertexColors[i].b = f.Read<float>();
		}
#endif
	}
	else if (isTriangleAttribute && attributeType == kQ3AttributeTypeNormal)		// face normals
	{
		printf("face normals (ignore)");
		f.Skip(currentMesh->numTriangles * 3 * 4);
	}
	else
	{
		Assert(false, "unsupported combo");
	}
}

uint32_t Q3MetaFileParser::Parse_txmm(uint32_t chunkSize)
{
	Assert(chunkSize >= 8*4, "incorrect chunk header size");

	uint32_t useMipmapping	= f.Read<uint32_t>();
	uint32_t pixelType		= f.Read<uint32_t>();
	uint32_t bitOrder		= f.Read<uint32_t>();
	uint32_t byteOrder		= f.Read<uint32_t>();
	uint32_t width			= f.Read<uint32_t>();
	uint32_t height			= f.Read<uint32_t>();
	uint32_t rowBytes		= f.Read<uint32_t>();
	uint32_t offset			= f.Read<uint32_t>();

	uint32_t imageSize = rowBytes * height;
	if ((imageSize & 3) != 0)
		imageSize = (imageSize & 0xFFFFFFFC) + 4;

	Assert(chunkSize == 8*4 + imageSize, "incorrect chunk size");


	Assert(!useMipmapping, "mipmapping not supported");
	printf("%d*%d rb=%d", width, height, rowBytes);

	static const char* pixelTypeDescriptions[] = { "RGB32", "ARGB32", "RGB16", "ARGB16", "RGB16_565", "RGB24" };
	if (pixelType < kQ3PixelTypeRGB24)
		printf(" %s", pixelTypeDescriptions[pixelType]);
	else
		printf(" UNKNOWN_PIXELTYPE");

	Assert(offset == 0, "unsupported texture offset");
	Assert(bitOrder == kQ3EndianBig, "unsupported bit order");

	// Find bytes per pixel
	int bytesPerPixel = 0;
	if (pixelType == kQ3PixelTypeRGB16 || pixelType == kQ3PixelTypeARGB16)
		bytesPerPixel = 2;
	else if (pixelType == kQ3PixelTypeRGB32 || pixelType == kQ3PixelTypeARGB32)
		bytesPerPixel = 4;
	else
		Assert(false, "unrecognized pixel type");

	int trimmedRowBytes = bytesPerPixel * width;



	uint32_t newTextureID = metaFile.numTextures;

	__Q3EnlargeArray<TQ3Pixmap*>(metaFile.textures, metaFile.numTextures, 'TLST');

	metaFile.textures[newTextureID] = __Q3Alloc<TQ3Pixmap>(1, 'PXMP');
	TQ3Pixmap& texture = *metaFile.textures[newTextureID];

	texture.glTextureName	= 0;
	texture.pixelType		= pixelType;
	texture.bitOrder		= bitOrder;
	texture.byteOrder		= byteOrder;
	texture.width			= width;
	texture.height			= height;
	texture.pixelSize		= bytesPerPixel * 8;
	texture.rowBytes		= trimmedRowBytes;
	texture.image			= __Q3Alloc<uint8_t>(trimmedRowBytes * height, 'IMAG');

	// Trim padding at end of rows
	for (uint32_t y = 0; y < height; y++)
	{
		f.Read((Ptr) texture.image + y*texture.rowBytes, texture.rowBytes);
		f.Skip(rowBytes - width * bytesPerPixel);
	}

	// Make every pixel little-endian (especially to avoid breaking 16-bit 1-5-5-5 ARGB textures)
	if (byteOrder == kQ3EndianBig)
	{
		ByteswapInts(bytesPerPixel, width*height, texture.image);
		texture.byteOrder = kQ3EndianLittle;
	}


	Q3Pixmap_ApplyEdgePadding(&texture);

	return newTextureID;
}
