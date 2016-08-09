#pragma once
#include "DDSTextureLoader.h"
#include "..\Common\DeviceResources.h"
#include "Content\ShaderStructures.h"
using namespace GraphicsScene;
using namespace DirectX;

class Model
{
	DX::DeviceResources* m_deviceResources;

	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_NormalMapSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_SpecularMapSRV;

	const char* m_modelFile;
	const wchar_t* m_textureFile;
	const wchar_t* m_normalMapFile;
	const wchar_t* m_specularMapFile;

	std::vector<RobustVertex> m_vertices;
	std::vector<unsigned int> m_indices;
public:
	//Constructors
	Model();
	Model(const std::shared_ptr<DX::DeviceResources>& deviceResources);
	Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename);
	Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename, const wchar_t * normalMapName);
	Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename, const wchar_t * normalMapName, const wchar_t * specularMapName);


	//Mutators
	void SetDeviceResources(const std::shared_ptr<DX::DeviceResources>& _deviceResources);
	void SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer);
	void SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer);
	void SetSRV(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _SRV);
	void SetNormalMap(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _NormalMapSRV);
	void SetSpecularMap(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _SpecularMapSRV);
	void SetSampler(const Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler);
	void SetModelFile(const char *_modelFile);
	void SetTextureFile(const wchar_t * _textureFile);
	void SetNormalMapFile(const wchar_t * _normalMapFile);
	void SetSpecularMapFile(const wchar_t * _specularMapFile);
	void SetVerts(const std::vector<RobustVertex> _vertices);
	void SetIndices(const std::vector<unsigned int> _indices);

	//Accessors
	const Microsoft::WRL::ComPtr<ID3D11Buffer>& GetVertexBuffer() { return m_vertexBuffer; }
	const Microsoft::WRL::ComPtr<ID3D11Buffer>& GetIndexBuffer() { return m_indexBuffer; }
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetSRV() { return m_SRV; }
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetNormalMap() { return m_NormalMapSRV; }
	const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& GetSpecularMap() { return m_SpecularMapSRV; }
	const Microsoft::WRL::ComPtr<ID3D11SamplerState>& GetSampler() { return m_sampler; }
	const char* GetModelFile() const { return m_modelFile; }
	const wchar_t* GetTextureFile() const { return m_textureFile; }
	const wchar_t* GetNormalMapFile() const { return m_normalMapFile; }
	const wchar_t* GetSpecularMapFile() const { return m_specularMapFile; }
	const std::vector<RobustVertex>& GetVerts() const { return m_vertices; }
	const std::vector<unsigned int>& GetIndices() const { return m_indices; }

	void CreateModel();
	void CreateModel(const char * _filename, const wchar_t * _texturename);
	void CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename);
	void CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename, const wchar_t * normalMapName);
	void CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename, const wchar_t * normalMapName, const wchar_t * specularMapName);
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	//TODO: Create Set and Load Functions
	bool LoadOBJ();
	bool LoadDDSTexture();
	bool LoadNormalMap();
	bool LoadSpecularMap();

	void CalculateTangents();

	void Release();

};