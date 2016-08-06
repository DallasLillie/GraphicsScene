#pragma once
#include "..\Common\DeviceResources.h"

#include "Content\ShaderStructures.h"
using namespace DirectX;
using namespace GraphicsScene;

class Model
{
	//TODO: accessors/mutators
	std::shared_ptr<DX::DeviceResources> m_deviceResources;

public:

	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;

	uint32	m_indexCount;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;

	const char* filename;

	std::vector<RobustVertex> m_vertices;
	std::vector<unsigned int> m_indices;

	Model();

	void Model::CreateIndexBuffer();
	void Model::CreateVertexBuffer();

	bool loadOBJ(const char * path);
};

