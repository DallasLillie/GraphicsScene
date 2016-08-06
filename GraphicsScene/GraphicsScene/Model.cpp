#include "pch.h"
#include "Model.h"


Model::Model()
{
	m_deviceResources = std::make_shared<DX::DeviceResources>();
}

void Model::CreateVertexBuffer()
{
	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = m_vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(m_vertices.size() * sizeof(RobustVertex), D3D11_BIND_VERTEX_BUFFER);
	HRESULT result = m_deviceResources->GetD3DDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexBufferData,
		m_vertexBuffer.GetAddressOf()
	);
}

void Model::CreateIndexBuffer()
{
	m_indexCount = m_indices.size();

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

bool Model::loadOBJ(const char * path)
{
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<XMFLOAT3> vertices;
	std::vector<XMFLOAT2> uvs;
	std::vector<XMFLOAT3> normals;

	FILE * file;
	fopen_s(&file,path, "r");
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
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
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

		m_vertices.push_back(tempVertex);
		m_indices.push_back(i);
	}
}

