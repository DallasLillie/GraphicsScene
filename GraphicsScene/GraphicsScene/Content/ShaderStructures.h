#pragma once

namespace GraphicsScene
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	struct ModelConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
	};

	struct ViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 view[2];
		DirectX::XMFLOAT4X4 projection[2];
	};

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct MInstancedConstantBuffer
	{
		DirectX::XMFLOAT4X4 model[10];
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct RobustVertex
	{
		DirectX::XMFLOAT3 pos;
		//DirectX::XMFLOAT3 wPos;
		DirectX::XMFLOAT2 texCoords;
		DirectX::XMFLOAT4 tangent;
		DirectX::XMFLOAT3 normal;
	};

	//Structure to hold data for a light
	struct Light
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT4 color;
		//OuterConeRatio, InnerConeRatio, LightRadius, On/Off
		DirectX::XMFLOAT4 ratio;
	};

	struct LightConstantBuffer
	{
		Light lights[3];
	};

	struct SpecularBufferCam
	{
		DirectX::XMFLOAT4 cameraPosition;
	};
}