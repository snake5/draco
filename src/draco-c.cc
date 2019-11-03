
#include "draco/compression/encode.h"
#include "draco/core/cycle_timer.h"
#include "draco/io/mesh_io.h"

using namespace draco;

struct MyOptions {
  MyOptions();

  int pos_quantization_bits;
  int tex_coords_quantization_bits;
  int normals_quantization_bits;
  int generic_quantization_bits;
  int compression_level;
};

MyOptions::MyOptions()
	: pos_quantization_bits(14),
	  tex_coords_quantization_bits(12),
	  normals_quantization_bits(10),
	  generic_quantization_bits(8),
	  compression_level(7) {}

struct compression_info
{
	// input
	float* positions; // 3, non-optional
	float* normals; // 3
	float* tangents; // 4
	float* texcoords0; // 2
	float* texcoords1; // 2
	float* colors0; // 4
	unsigned* indices;
	unsigned numverts;
	unsigned numindices;

	// output
	unsigned char* cmesh; // init to 0
	float* cmpositions; // init to 0
	unsigned* cmindices; // init to 0
	unsigned cmsize;
	unsigned cmnumverts;
	unsigned cmnumindices;
};

extern "C" __declspec(dllexport) void draco_free_compressed_mesh(compression_info& info)
{
	free(info.cmesh);
	info.cmesh = nullptr;
	delete[] info.cmpositions;
	info.cmpositions = nullptr;
	delete[] info.cmindices;
	info.cmindices = nullptr;
}

extern "C" __declspec(dllexport) int draco_compress_mesh(compression_info& info)
{
	Mesh mesh;
	const bool use_identity_mapping = true;
	if (info.positions) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::POSITION, nullptr, 3, DT_FLOAT32, false, sizeof(float) * 3, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.positions, 4 * 3 * info.numverts);
	}
	if (info.normals) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::NORMAL, nullptr, 3, DT_FLOAT32, false, sizeof(float) * 3, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.normals, 4 * 3 * info.numverts);
	}
	if (info.tangents) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::GENERIC, nullptr, 4, DT_FLOAT32, false, sizeof(float) * 4, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.tangents, 4 * 4 * info.numverts);
	}
	if (info.texcoords0) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::TEX_COORD, nullptr, 2, DT_FLOAT32, false, sizeof(float) * 2, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.texcoords0, 4 * 2 * info.numverts);
	}
	if (info.texcoords1) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::TEX_COORD, nullptr, 2, DT_FLOAT32, false, sizeof(float) * 2, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.texcoords1, 4 * 2 * info.numverts);
	}
	if (info.colors0) {
		GeometryAttribute va;
		va.Init(GeometryAttribute::GENERIC, nullptr, 4, DT_FLOAT32, false, sizeof(float) * 4, 0);
		mesh.attribute(mesh.AddAttribute(va, use_identity_mapping, info.numverts))->buffer()->Update(info.colors0, 4 * 4 * info.numverts);
	}
	if (info.indices)
	{
		for (unsigned i = 0; i + 2 < info.numindices; i += 3)
		{
			Mesh::Face fff;
			fff[0] = info.indices[i];
			fff[1] = info.indices[i + 1];
			fff[2] = info.indices[i + 2];
			mesh.AddFace(fff);
		}
	}

	Encoder encoder;
	MyOptions options;
	if (options.pos_quantization_bits > 0) {
		encoder.SetAttributeQuantization(GeometryAttribute::POSITION, options.pos_quantization_bits);
	}
	if (options.tex_coords_quantization_bits > 0) {
		encoder.SetAttributeQuantization(GeometryAttribute::TEX_COORD, options.tex_coords_quantization_bits);
	}
	if (options.normals_quantization_bits > 0) {
		encoder.SetAttributeQuantization(GeometryAttribute::NORMAL, options.normals_quantization_bits);
	}
	if (options.generic_quantization_bits > 0) {
		encoder.SetAttributeQuantization(GeometryAttribute::GENERIC, options.generic_quantization_bits);
	}
	// Convert compression level to speed (that 0 = slowest, 10 = fastest).
	const int speed = 10 - options.compression_level;
	encoder.SetSpeedOptions(speed, speed);
	// crashes :(
	//encoder.SetEncodingMethod(MESH_SEQUENTIAL_ENCODING);
	  
	EncoderBuffer buffer;
	if (!encoder.EncodeMeshToBuffer(mesh, &buffer).ok())
		return -1;

	Decoder testdec;
	DecoderBuffer decbuf;
	decbuf.Init(buffer.data(), buffer.size());
	Mesh decmesh;
	if (!testdec.DecodeBufferToGeometry(&decbuf, &decmesh).ok())
		return -2;

	draco_free_compressed_mesh(info);
	info.cmesh = (unsigned char*) malloc(buffer.size());
	memcpy(info.cmesh, buffer.data(), buffer.size());
	info.cmsize = buffer.size();

	auto* pa = decmesh.GetNamedAttribute(GeometryAttribute::POSITION);
	info.cmnumverts = pa->size();
	info.cmnumindices = decmesh.num_faces() * 3;
	info.cmpositions = new float[info.cmnumverts * 3];
	info.cmindices = new unsigned[info.cmnumindices];
	memcpy(info.cmpositions, pa->GetAddress(AttributeValueIndex(0)), info.cmnumverts * 3 * sizeof(float));
	for (unsigned i = 0; i < decmesh.num_faces(); i++)
	{
		auto& F = decmesh.face(FaceIndex(i));
		info.cmindices[i * 3 + 0] = pa->mapped_index(F[0]).value();
		info.cmindices[i * 3 + 1] = pa->mapped_index(F[1]).value();
		info.cmindices[i * 3 + 2] = pa->mapped_index(F[2]).value();
	}

	return 0;
}
