#include "Pomme3DMF.h"
#include "Pomme3DMF_Internal.h"
#include "PommeDebug.h"

#include <stack>
#include <fstream>

//#define printf(...) do{}while(0)

static void Assert(bool condition, const char* message)
{
	if (!condition)
	{
		throw std::runtime_error(message);
	}
}

static void DumpTGA_RGBA(const char* path, short width, short height, const char* rgbaData)
{
	std::ofstream tga(path);
	uint16_t tgaHdr[] = {0, 2, 0, 0, 0, 0, (uint16_t) width, (uint16_t) height, 0x2820};
	tga.write((const char*) tgaHdr, sizeof(tgaHdr));
	for (int i = 0; i < 4 * width * height; i += 4)
	{
		tga.put(rgbaData[i + 2]); //b
		tga.put(rgbaData[i + 1]); //g
		tga.put(rgbaData[i + 0]); //r
		tga.put(rgbaData[i + 3]); //a
	}
	tga.close();
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

Q3MetaFileParser::Q3MetaFileParser(std::istream& theBaseStream, Q3MetaFile& dest)
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
		case 'cntr':    // Container
		{
			if (currentDepth == 1)
				metaFile.topLevelMeshGroups.push_back({});
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
				metaFile.topLevelMeshGroups.push_back({});
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
			if (!metaFile.topLevelMeshGroups.empty())
				metaFile.topLevelMeshGroups.back().push_back(currentMesh);
//				currentMeshContainerDepth = containerEnds.size();
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
				float r = f.Read<float>();
				float g = f.Read<float>();
				float b = f.Read<float>();
				printf("%.3f %.3f %.3f\t", r, g, b);
				for (uint32_t i = 0; i < currentMesh->numPoints; i++)
				{
					currentMesh->vertexColors[i].r = r;
					currentMesh->vertexColors[i].g = g;
					currentMesh->vertexColors[i].b = b;
				}
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
				for (uint32_t i = 0; i < currentMesh->numPoints; i++)
				{
					currentMesh->vertexColors[i].a = a;
				}
				currentMesh->hasTransparency = true;
			}
			break;

		case 'txsu':    // TextureShader
			Assert(chunkSize == 0, "illegal txsu size");
			break;

		case 'txmm':    // MipmapTexture
			if (knownTextures.contains(chunkOffset))
			{
				printf("Texture already seen!");
				f.Skip(chunkSize);
				Assert(!currentMesh->hasTexture, "txmm: current mesh already has a texture");
				currentMesh->hasTexture = true;
				currentMesh->internalTextureID = knownTextures[chunkOffset];
			}
			else
			{
				uint32_t internalTextureID = Parse_txmm(chunkSize);
				knownTextures[chunkOffset] = internalTextureID;
			}
			break;

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

		default:
			printf("??????????????????? size=%d", chunkSize);
			//Assert(false, "Unknown chunkType");
			//return -1;
			f.Skip(chunkSize);
			break;
	}

	return chunkType;
}

void Q3MetaFileParser::Parse3DMF()
{
	baseStream.seekg(0, std::ios::end);
	std::streampos fileLength = baseStream.tellg();
	baseStream.seekg(0, std::ios::beg);

	//printf("========== %s ============\n", path);

	Assert(f.Read<uint32_t>() == '3DMF', "Not a 3DMF file");
	Assert(f.Read<uint32_t>() == 16, "Bad header length");

	uint16_t versionMajor = f.Read<uint16_t>();
	uint16_t versionMinor = f.Read<uint16_t>();

	uint32_t flags = f.Read<uint32_t>();
	Assert(flags == 0, "Database or Stream aren't supported");

	uint64_t tocOffset = f.Read<uint64_t>();
	printf("Version %d.%d    tocOffset %08lx\n", versionMajor, versionMinor, tocOffset);

	//Assert(tocOffset == 0, "Can't handle files with a TOC yet");

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


	size_t currentMeshContainerDepth = 0;


	// Chunk Loop
	while (f.Tell() != fileLength)
	{
		Parse1Chunk();
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
	//Assert(0 == numTriangleAttributes, "triangle attributes are not supported");
	Assert(0 == numEdges, "edges are not supported");
	Assert(0 == numEdgeAttributes, "edge attributes are not supported");
	//Assert(0 == numVertexAttributes, "vertex attributes are not supported");

	currentMesh = Q3TriMeshData_New(numTriangles, numVertices);

	metaFile.meshes.push_back(currentMesh);

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
		ReadTriangleVertexIndices<uint32_t>(f, numTriangles, currentMesh);
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
		for (uint32_t i = 0; i < currentMesh->numPoints; i++)
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
		for (uint32_t i = 0; i < currentMesh->numPoints; i++)
		{
			currentMesh->vertexNormals[i].x = f.Read<float>();
			currentMesh->vertexNormals[i].y = f.Read<float>();
			currentMesh->vertexNormals[i].z = f.Read<float>();
		}
	}
	else if (isVertexAttribute && attributeType == kQ3AttributeTypeDiffuseColor)	// used in Bugdom's Global_Models2.3dmf
	{
		printf("vertex diffuse");
//		Assert(positionInArray == 0, "PIA must be 0 for colors");
		Assert(currentMesh->vertexNormals, "current mesh has no vertex color array");
		for (uint32_t i = 0; i < currentMesh->numPoints; i++)
		{
			currentMesh->vertexColors[i].r = f.Read<float>();
			currentMesh->vertexColors[i].g = f.Read<float>();
			currentMesh->vertexColors[i].b = f.Read<float>();
		}
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
	Assert(currentMesh, "no current mesh");
	Assert(!currentMesh->hasTexture, "current mesh already has a texture");
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


	uint32_t internalTextureID = metaFile.textures.size();
	metaFile.textures.push_back({});
	Q3MetaFile_Texture& texture = metaFile.textures[internalTextureID];
	currentMesh->internalTextureID = internalTextureID;
	currentMesh->hasTexture = true;

	texture.textureName		= 0;
	texture.useMipMapping	= useMipmapping;
	texture.pixelType		= pixelType;
	texture.bitOrder		= bitOrder;
	texture.byteOrder		= byteOrder;
	texture.width			= width;
	texture.height			= height;
	texture.rowBytes		= rowBytes;
	texture.offset			= offset;

	Assert(offset == 0, "unsupported texture offset");
	Assert(bitOrder == kQ3EndianBig, "unsupported bit order");
	Assert(byteOrder == kQ3EndianBig, "unsupported byte order");

	//texture.buffer			= f.ReadBytes(texture.rowBytes * height);

	// Convert it to ARGB
	//uint8_t* inData = texture.buffer.data();
	texture.buffer.clear();
	texture.buffer.reserve(4 * width * height);
	switch (pixelType)
	{
		case kQ3PixelTypeRGB16:
		{
			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x++)
				{
					uint16_t pixel = f.Read<uint16_t>();

					// 0 RRRRR GGGGG BBBBB
					uint8_t r = ((pixel & 0b0111110000000000) >> 10) * 255 / 31;
					uint8_t g = ((pixel & 0b0000001111100000) >>  5) * 255 / 31;
					uint8_t b = ((pixel & 0b0000000000011111)      ) * 255 / 31;

					texture.buffer.push_back(r);
					texture.buffer.push_back(g);
					texture.buffer.push_back(b);
					texture.buffer.push_back(255);
				}

				f.Skip(rowBytes - width * 2);
			}
			break;
		}

		case kQ3PixelTypeARGB16:
		{
			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x++)
				{
					uint16_t pixel = f.Read<uint16_t>();

					// A RRRRR GGGGG BBBBB
					uint8_t r = ((pixel & 0b0111110000000000) >> 10) * 255 / 31;
					uint8_t g = ((pixel & 0b0000001111100000) >>  5) * 255 / 31;
					uint8_t b = ((pixel & 0b0000000000011111)      ) * 255 / 31;

					texture.buffer.push_back(r);
					texture.buffer.push_back(g);
					texture.buffer.push_back(b);
					texture.buffer.push_back(pixel == 0? 0 : 255);
				}

				f.Skip(rowBytes - width * 2);
			}
			break;
		}

		case kQ3PixelTypeARGB32:
		{
			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x++)
				{
					texture.buffer.push_back(f.Read<uint8_t>()); // r
					texture.buffer.push_back(f.Read<uint8_t>()); // g
					texture.buffer.push_back(f.Read<uint8_t>()); // b
					texture.buffer.push_back(f.Read<uint8_t>()); // a
				}

				f.Skip(rowBytes - width * 4);
			}
			break;
		}

		default:
			Assert(false, "unrecognized pixel type");
	}

	texture.pixelType = kQ3PixelTypeARGB32;
	texture.rowBytes = 4 * width;
	texture.offset = 0;


	char outname[256];
	snprintf(outname, sizeof(outname), "/tmp/3dmftex%d.tga", numTexturesDumpedToTGA);
	DumpTGA_RGBA(outname, width, height, (const char*)texture.buffer.data());
	printf("   (wrote %s)", outname);
	numTexturesDumpedToTGA++;


	return internalTextureID;
}

