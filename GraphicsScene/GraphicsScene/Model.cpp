#include "pch.h"
#include "Model.h"

Model::Model()
{
	
}
Model::Model(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources.get())
{
	CreateModel();
}
Model::Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename) :
	m_deviceResources(deviceResources.get()),
	m_modelFile(filename),
	m_textureFile(texturename)
{
	this->CreateModel();
}
Model::Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename, const wchar_t * normalMapName):
	m_deviceResources(deviceResources.get()),
	m_modelFile(filename),
	m_textureFile(texturename),
	m_normalMapFile(normalMapName)
{
	this->CreateModel();
}
Model::Model(const std::shared_ptr<DX::DeviceResources>& deviceResources, const char * filename, const wchar_t * texturename, const wchar_t * normalMapName, const wchar_t * specularMapName):
	m_deviceResources(deviceResources.get()),
	m_modelFile(filename),
	m_textureFile(texturename),
	m_normalMapFile(normalMapName),
	m_specularMapFile(specularMapName)
{
	this->CreateModel();
}

void Model::SetDeviceResources(const std::shared_ptr<DX::DeviceResources>& _deviceResources)
{
	m_deviceResources = _deviceResources.get();
}

void Model::SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer)
{
	m_vertexBuffer.Reset();
	m_vertexBuffer = _vertexBuffer;
}

void Model::SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer)
{
	m_indexBuffer.Reset();
	m_indexBuffer = _indexBuffer;
}

void Model::SetSRV(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _SRV)
{
	m_SRV.Reset();
	m_SRV = _SRV;
}

void Model::SetNormalMap(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _NormalMapSRV)
{
	m_NormalMapSRV.Reset();
	m_NormalMapSRV = _NormalMapSRV;
}

void Model::SetSpecularMap(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _SpecularMapSRV)
{
	m_SpecularMapSRV.Reset();
	m_SpecularMapSRV = _SpecularMapSRV;
}

void Model::SetSampler(const Microsoft::WRL::ComPtr<ID3D11SamplerState> _sampler)
{
	m_sampler.Reset();
	m_sampler = _sampler;
}

void Model::SetModelFile(const char *_modelFile)
{
	m_modelFile = _modelFile;
}

void Model::SetTextureFile(const wchar_t * _textureFile)
{
	m_textureFile = _textureFile;
}

void Model::SetNormalMapFile(const wchar_t * _normalMapFile)
{
	m_normalMapFile = _normalMapFile;
}

void Model::SetSpecularMapFile(const wchar_t * _specularMapFile)
{
	m_specularMapFile = _specularMapFile;
}

void Model::SetVerts(std::vector<RobustVertex> _vertices)
{
	m_vertices.clear();
	m_vertices = _vertices;
}

void Model::SetIndices(std::vector<unsigned int> _indices)
{
	m_indices.clear();
	m_indices = _indices;
}

void Model::SetWorldMatrix(const XMFLOAT4X4 _worldMatrix)
{
	m_worldMatrix = _worldMatrix;
}

void Model::CreateModel()
{
	this->LoadOBJ();
	this->CalculateTangents();
	this->CreateVertexBuffer();
	this->CreateIndexBuffer();
	this->LoadDDSTexture();
	this->LoadNormalMap();
}
void Model::CreateModel(const char * _filename, const wchar_t * _texturename)
{
	SetModelFile(_filename);
	SetTextureFile(_texturename);

	this->LoadOBJ();
	this->CalculateTangents();
	this->CreateVertexBuffer();
	this->CreateIndexBuffer();
	this->LoadDDSTexture();
	this->LoadNormalMap();
}
void Model::CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename)
{
	SetDeviceResources(_deviceResources);
	SetModelFile(_filename);
	SetTextureFile(_texturename);

	this->LoadOBJ();
	this->CalculateTangents();
	this->CreateVertexBuffer();
	this->CreateIndexBuffer();
	this->LoadDDSTexture();
	this->LoadNormalMap();
}
void Model::CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename, const wchar_t * normalMapName)
{
	SetDeviceResources(_deviceResources);
	SetModelFile(_filename);
	SetTextureFile(_texturename);
	SetNormalMapFile(normalMapName);

	this->LoadOBJ();
	this->CalculateTangents();
	this->CreateVertexBuffer();
	this->CreateIndexBuffer();
	this->LoadDDSTexture();
	this->LoadNormalMap();
}
void Model::CreateModel(const std::shared_ptr<DX::DeviceResources>& _deviceResources, const char * _filename, const wchar_t * _texturename, const wchar_t * normalMapName, const wchar_t * specularMapName)
{
	SetDeviceResources(_deviceResources);
	SetModelFile(_filename);
	SetTextureFile(_texturename);
	SetNormalMapFile(normalMapName);
	SetSpecularMapFile(specularMapName);

	this->LoadOBJ();
	this->CalculateTangents();
	this->CreateVertexBuffer();
	this->CreateIndexBuffer();
	this->LoadDDSTexture();
	this->LoadNormalMap();
	this->LoadSpecularMap();
}

void Model::CreateVertexBuffer()
{
	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = this->m_vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(this->m_vertices.size() * sizeof(RobustVertex), D3D11_BIND_VERTEX_BUFFER);
	HRESULT result = this->m_deviceResources->GetD3DDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		this->m_vertexBuffer.GetAddressOf()
	);
}

void Model::CreateIndexBuffer()
{
	//m_indexCount = m_indices.size();

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData = { 0 };
	indexBufferData.pSysMem = m_indices.data();
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(m_indices.size() * sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
	HRESULT result =
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			m_indexBuffer.GetAddressOf()
		);
}

bool Model::LoadOBJ()
{
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<XMFLOAT3> vertices;
	std::vector<XMFLOAT2> uvs;
	std::vector<XMFLOAT3> normals;

	FILE * file;
	fopen_s(&file, m_modelFile, "r");
	if (file == NULL)
	{
		printf("Impossible to open the file! \n");
		return false;
	}

	while (1)
	{
		char lineHeader[128];

		int res = fscanf_s(file, "%s", lineHeader,128);

		if (res == EOF)
		{
			break;
		}

		if (strcmp(lineHeader, "v") == 0)
		{
			XMFLOAT3 vertex;
			fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertex.x *= -1;
			vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0)
		{
			XMFLOAT2 uv;
			fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = 1 - uv.y;
			uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0)
		{
			XMFLOAT3 normal;
			fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normal.x *= -1;
			normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0)
		{
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf_s(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9)
			{
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[2]);
			vertexIndices.push_back(vertexIndex[1]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[2]);
			uvIndices.push_back(uvIndex[1]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[2]);
			normalIndices.push_back(normalIndex[1]);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++)
	{
		RobustVertex tempVertex;
		unsigned int vertexIndex = vertexIndices[i];
		XMFLOAT3 vertex = vertices[vertexIndex - 1];
		tempVertex.pos = vertex;

		unsigned int uvIndex = uvIndices[i];
		XMFLOAT2 tex = uvs[uvIndex - 1];
		tempVertex.texCoords = tex;

		unsigned int normalIndex = normalIndices[i];
		XMFLOAT3 norm = normals[normalIndex - 1];
		tempVertex.normal = norm;

		tempVertex.tangent = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

		//tempVertex.pos.x *= -1;
		//tempVertex.normal.x *= -1;

		m_vertices.push_back(tempVertex);
		m_indices.push_back(i);
	}
}

bool Model::LoadDDSTexture()
{
	HRESULT result = CreateDDSTextureFromFile(this->m_deviceResources->GetD3DDevice(), m_textureFile, NULL, m_SRV.GetAddressOf());
	return (result == S_OK) ? true : false;
}

bool Model::LoadNormalMap()
{
	HRESULT result = CreateDDSTextureFromFile(this->m_deviceResources->GetD3DDevice(), m_normalMapFile, NULL, m_NormalMapSRV.GetAddressOf());
	return (result == S_OK) ? true : false;
}

bool Model::LoadSpecularMap()
{
	HRESULT result = CreateDDSTextureFromFile(this->m_deviceResources->GetD3DDevice(), m_specularMapFile, NULL, m_SpecularMapSRV.GetAddressOf());
	return (result == S_OK) ? true : false;
}

void Model::CalculateTangents()
{

	for (unsigned int  i = 0; i < m_indices.size(); i+=3)
	{
		//TODO:: Average Tangents
		//TODO:: calc tangents for objects other than models
		XMFLOAT3 tempVert1 = m_vertices[m_indices[i]].pos;
		XMFLOAT3 tempVert2 = m_vertices[m_indices[i+1]].pos;
		XMFLOAT3 tempVert3 = m_vertices[m_indices[i+2]].pos;
		XMVECTOR vertEdge1 = XMVectorSubtract(XMLoadFloat3(&tempVert2), XMLoadFloat3(&tempVert1));
		XMVECTOR vertEdge2 = XMVectorSubtract(XMLoadFloat3(&tempVert3), XMLoadFloat3(&tempVert1));
		XMFLOAT3 vertEdges1;
		XMFLOAT3 vertEdges2;
		XMStoreFloat3(&vertEdges1, vertEdge1);
		XMStoreFloat3(&vertEdges2, vertEdge2);


		XMFLOAT2 texCoord1 = m_vertices[m_indices[i]].texCoords;
		XMFLOAT2 texCoord2 = m_vertices[m_indices[i + 1]].texCoords;
		XMFLOAT2 texCoord3 = m_vertices[m_indices[i + 2]].texCoords;

		XMVECTOR texEdge1 = XMVectorSubtract(XMLoadFloat2(&texCoord2), XMLoadFloat2(&texCoord1));
		XMVECTOR texEdge2 = XMVectorSubtract(XMLoadFloat2(&texCoord3), XMLoadFloat2(&texCoord1));
		XMFLOAT2 texEdges1;
		XMFLOAT2 texEdges2;
		XMStoreFloat2(&texEdges1, texEdge1);
		XMStoreFloat2(&texEdges2, texEdge2);

		float ratio = 1.0f / (texEdges1.x * texEdges2.y - texEdges2.x * texEdges1.y);

		XMFLOAT3 uDirection = XMFLOAT3((texEdges2.y * vertEdges1.x - texEdges1.y * vertEdges2.x) * ratio,
							  (texEdges2.y * vertEdges1.y - texEdges1.y * vertEdges2.y) * ratio,
							  (texEdges2.y * vertEdges1.z - texEdges1.y * vertEdges2.z) * ratio );
		XMFLOAT3 vDirection = XMFLOAT3((texEdges1.x * vertEdges1.x - texEdges2.x * vertEdges2.x) * ratio,
									   (texEdges1.x * vertEdges1.y - texEdges2.x * vertEdges2.y) * ratio,
									   (texEdges1.x * vertEdges1.z - texEdges2.x * vertEdges2.z) * ratio);

		XMVECTOR uDirec = XMLoadFloat3(&uDirection);
		XMVECTOR vDirec = XMLoadFloat3(&vDirection);
		uDirec = XMVector3Normalize(uDirec);
		vDirec = XMVector3Normalize(vDirec);

		for (unsigned int j = 0; j < 3; ++j)
		{
			XMFLOAT3 dotResult;
			XMStoreFloat3(&dotResult, XMVector3Dot(XMLoadFloat3(&m_vertices[m_indices[i+j]].normal), uDirec));
			XMVECTOR tangent;
			XMFLOAT4 prevTangent = m_vertices[m_indices[i + j]].tangent;
			tangent = uDirec - XMLoadFloat3(&m_vertices[m_indices[i + j]].normal) * dotResult.x;
			tangent = XMVector3Normalize(tangent);
			XMStoreFloat4(&m_vertices[m_indices[i + j]].tangent, tangent);


			XMVECTOR cross = XMVector3Cross(XMLoadFloat3(&m_vertices[m_indices[i + j]].normal), uDirec);
			XMVECTOR handedness = vDirec;
			XMStoreFloat3(&dotResult, XMVector3Dot(cross, handedness));
			m_vertices[m_indices[i + j]].tangent.w = (dotResult.x < 0.0f) ? -1.0f : 1.0f;


			//TODO: AVERAGE?????
			//if (m_indices[i + j] != i + j)
			//{
			//	break;
			//}

			//if (prevTangent.x != 0 && prevTangent.y != 0 && prevTangent.z != 0)
			//{
			//	prevTangent.x = (prevTangent.x + m_vertices[m_indices[i + j]].tangent.x)*0.5f;
			//	prevTangent.y = (prevTangent.y + m_vertices[m_indices[i + j]].tangent.y)*0.5f;
			//	prevTangent.z = (prevTangent.z + m_vertices[m_indices[i + j]].tangent.z)*0.5f;
			//}
		}
	}
}

void Model::Release()
{
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_sampler.Reset();
	m_SRV.Reset();
	m_NormalMapSRV.Reset();
	m_SpecularMapSRV.Reset();
}


