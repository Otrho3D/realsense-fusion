#include <stdio.h>
#include "model.h"
#include "implicit.h"




void Model::Init(int resolutionX, int resolutionY, int resolutionZ, float cellSize)
{
	if (resolutionX % 2 != 0 || resolutionY % 2 != 0 || resolutionZ % 2 != 0)
	{
		std::cout << "WARNING : resolution is not odd, center does not lie on grid" ;
	}
	this->resolutionX = resolutionX;
	this->resolutionY = resolutionY;
	this->resolutionZ = resolutionZ;

	this->cellSize = cellSize;

}

// Constructor with default origin placement in camera centered coordinate system
// Origin is located at min (-resX/2, -resY/2 , -resZ/2)
Model::Model(int resolutionX, int resolutionY, int resolutionZ, float cellSize)
{
	Init(resolutionX, resolutionY, resolutionZ, cellSize);

	const float cornerX = - static_cast<float>(resolutionX) / 2.0f * cellSize;
	const float cornerY = - static_cast<float>(resolutionY) / 2.0f * cellSize;
	const float cornerZ = - static_cast<float>(resolutionZ) / 2.0f * cellSize;

	this->modelOrigin = Eigen::Vector3f(cornerX, cornerY, cornerZ);

}

Model::Model(int resolutionX, int resolutionY, int resolutionZ, float cellSize, Eigen::Vector3f modelOrigin)
{
	Init(resolutionX, resolutionY, resolutionZ, cellSize);

	//assert model in grid

	this->modelOrigin = modelOrigin;
}

Model::~Model()
{
}


// these must be exactly the same as in glsl_common_grid.inl

Eigen::Vector3f Model::GridToWorld(Eigen::Vector3f pos)
{
	return pos.cwiseProduct(Eigen::Vector3f(resolutionX, resolutionY, resolutionZ)) * cellSize + modelOrigin;

}

Eigen::Vector3f Model::WorldToGrid(Eigen::Vector3f pos)
{
	return ((pos - modelOrigin) / cellSize).cwiseProduct(
		Eigen::Vector3f(1.0f / (float)resolutionX, 1.0f / (float)resolutionY, 1.0f / (float)resolutionZ));

}

Eigen::Vector3i Model::GridToTexel(Eigen::Vector3f pos)
{
	return Eigen::Vector3f(resolutionX, resolutionY, resolutionZ).cwiseProduct(pos).cast<int>();
}

Eigen::Vector3f Model::TexelToGrid(Eigen::Vector3i pos)
{
	return pos.cast<float>().cwiseProduct(
			Eigen::Vector3f(1.0f / (float)resolutionX, 1.0f / (float)resolutionY, 1.0f / (float)resolutionZ));
}



CPUModel::CPUModel(int resolutionX, int resolutionY, int resolutionZ, float cellSize)
	: Model(resolutionX, resolutionY, resolutionZ, cellSize)
{
	Init();
}

CPUModel::CPUModel(int resolutionX, int resolutionY, int resolutionZ, float cellSize, Eigen::Vector3f modelOrigin)
	: Model(resolutionX, resolutionY, resolutionZ, cellSize, modelOrigin)
{
	Init();
}

CPUModel::~CPUModel()
{
	delete [] tsdf;
	delete [] weigths;
}

void CPUModel::Init()
{
	tsdf = new float[resolutionX*resolutionY*resolutionZ];
	weigths = new float[resolutionX*resolutionY*resolutionZ];

	for (int x = 0; x < resolutionX*resolutionY*resolutionZ; x++)
	{
		tsdf[x] = 1;
		weigths[x] = 1;
	}
	// init with -inf missing here
}

void CPUModel::GenerateSphere(float radius, Eigen::Vector3f center)
{
	_IMPLICIT_H::Sphere sphere(radius, center.x(), center.y(), center.z());
	//iterate over all sdf fields
	for (int z = 0; z < resolutionZ; z++)
	{
		for (int y = 0; y < resolutionY; y++)
		{
			for (int x = 0; x < resolutionX; x++)
			{
				Eigen::Vector3f grid_pos = TexelToGrid(Eigen::Vector3i(x, y, z));
				Eigen::Vector3f world_pos = GridToWorld(grid_pos);
				float eval = sphere.sdf(world_pos.x(), world_pos.y(), world_pos.z());
				int cellIndex = IDX(x, y, z);
				tsdf[cellIndex] = eval;
			}
		}
	}
}

void CPUModel::DebugToLog()
{

	int x = resolutionX ;
	int y = resolutionY ;
	int z = resolutionZ ;

	std::cout << "Write contents of Model to cout \n";
	std::cout << "resolutionX :" << x << "\n";
	std::cout << "resolutionY :" << y << "\n";
	std::cout << "resolutionZ :" << z << "\n";

	std::cout << "tsdf of size :" << x * y * z ;

	std::cout << "grid with cellSize : " << cellSize << "\n";
	std::cout << "total length in x : " << x * cellSize << "\n";
	std::cout << "total length in y : " << y * cellSize << "\n";
	std::cout << "total length in z : " << z * cellSize << "\n";

	std::cout << "model Origin at : " << modelOrigin << "\n";

	std::cout << "Print contends of sdf : " << "\n";

	if (resolutionX * resolutionY < 16*16)
	{
		for (int z = 0; z < resolutionZ; z++)
		{
			std::cout << "Frame " << z << "\n";

			for (int y = 0; y < resolutionY; y++)
			{
				for (int x = 0; x < resolutionX; x++)
				{
					std::cout << tsdf[IDX(x,y,z)] << " ";
				}
				std::cout << "\n";
			}
		}
	}
}
