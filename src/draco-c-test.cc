
#include <stdio.h>

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

extern "C" __declspec(dllimport) void draco_free_compressed_mesh(compression_info& info);
extern "C" __declspec(dllimport) int draco_compress_mesh(compression_info& info);


int main()
{
	float pos[] = { 1, 1, 1,  2, 2, 2,  3, 3, 3 };
	float nrm[] = { -1, 0, 0,  0, 1, 0,  0, 0, 1 };
	float tx0[] = { 0, 0,   0, 1,  1, 1 };
	unsigned idcs[] = { 0, 1, 2, 2, 1, 0 };
	compression_info cinfo =
	{
		pos,
		nrm,
		nullptr,
		tx0,
		nullptr,
		nullptr,
		idcs,
		3, 6,
		nullptr, nullptr, nullptr, 0, 0, 0,
	};
	printf("draco compression returned: %d\n", draco_compress_mesh(cinfo));
	printf("compressed mesh ptr=%p size=%u\n", cinfo.cmesh, cinfo.cmsize);
	draco_free_compressed_mesh(cinfo);
	return 0;
}
