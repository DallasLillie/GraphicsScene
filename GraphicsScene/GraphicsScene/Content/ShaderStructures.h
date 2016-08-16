#pragma once

namespace GraphicsScene
{
#define NEAR_PLANE 0.01f
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

	struct ViewProjectionLightBuffer
	{
		DirectX::XMFLOAT4X4 view[6];
		DirectX::XMFLOAT4X4 projection;

		void UpdateView(Light& _light, float _width, float _height)
		{
			DirectX::XMMATRIX tView;


			switch ((int)_light.position.w)
			{
			case 0:
			{
				DirectX::XMVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis = XMLoadFloat4(&_light.normal);
				zAxis = DirectX::XMVector3Normalize(zAxis);
				DirectX::XMVECTOR xAxis = DirectX::XMVector3Cross(up, zAxis);
				xAxis = DirectX::XMVector3Normalize(xAxis);
				DirectX::XMVECTOR yAxis = DirectX::XMVector3Cross(zAxis, xAxis);
				yAxis = DirectX::XMVector3Normalize(yAxis);
				DirectX::XMVECTOR pos;
				pos.m128_f32[0] = ((_light.ratio.z - NEAR_PLANE)*0.5f) - (zAxis.m128_f32[0] * _width *0.5f);
				pos.m128_f32[1] = ((_light.ratio.z - NEAR_PLANE)*0.5f) - (zAxis.m128_f32[1] * _width *0.5f);
				pos.m128_f32[2] = ((_light.ratio.z - NEAR_PLANE)*0.5f) - (zAxis.m128_f32[2] * _width *0.5f);
				pos.m128_f32[3] = 1;
				tView = DirectX::XMMATRIX(xAxis, yAxis, zAxis, pos);
				break;
			}
			case 1:
			{
				//Assuming w identifier for point light stays as 1
				DirectX::XMVECTOR pos = XMLoadFloat4(&_light.position);

				DirectX::XMVECTOR xAxis0 = { 0.0f, 0.0f, -1.0f, 0.0f };
				DirectX::XMVECTOR yAxis0 = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis0 = { 1.0f, 0.0f, 0.0f, 0.0f };

				DirectX::XMVECTOR xAxis1 = { 0.0f, 0.0f, 1.0f, 0.0f };
				DirectX::XMVECTOR yAxis1 = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis1 = { -1.0f, 0.0f, 1.0f, 0.0f };
				tView = DirectX::XMMATRIX(xAxis1, yAxis1, zAxis1, pos);
				XMStoreFloat4x4(&view[1], tView);

				DirectX::XMVECTOR xAxis2 = { 1.0f, 0.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR yAxis2 = { 0.0f, 0.0f, -1.0f, 0.0f };
				DirectX::XMVECTOR zAxis2 = { 0.0f, 1.0f, 0.0f, 0.0f };
				tView = DirectX::XMMATRIX(xAxis2, yAxis2, zAxis2, pos);
				XMStoreFloat4x4(&view[2], tView);

				DirectX::XMVECTOR xAxis3 = { 1.0f, 0.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR yAxis3 = { 0.0f, 0.0f, 1.0f, 0.0f };
				DirectX::XMVECTOR zAxis3 = { 0.0f, -1.0f, 0.0f, 0.0f };
				tView = DirectX::XMMATRIX(xAxis3, yAxis3, zAxis3, pos);
				XMStoreFloat4x4(&view[3], tView);

				DirectX::XMVECTOR xAxis4 = { 1.0f, 0.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR yAxis4 = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis4 = { 0.0f, 0.0f, 1.0f, 0.0f };
				tView = DirectX::XMMATRIX(xAxis4, yAxis4, zAxis4, pos);
				XMStoreFloat4x4(&view[4], tView);

				DirectX::XMVECTOR xAxis5 = { -1.0f, 0.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR yAxis5 = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis5 = { 0.0f, 0.0f, -1.0f, 0.0f };
				tView = DirectX::XMMATRIX(xAxis5, yAxis5, zAxis5, pos);
				XMStoreFloat4x4(&view[5], tView);

				//Set tView for the storeFloat outside of the switch
				tView = DirectX::XMMATRIX(xAxis0, yAxis0, zAxis0, pos);
				break;
			}
			case 2:
			{
				DirectX::XMVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };
				DirectX::XMVECTOR zAxis = XMLoadFloat4(&_light.normal);
				zAxis = DirectX::XMVector3Normalize(zAxis);
				DirectX::XMVECTOR xAxis = DirectX::XMVector3Cross(up, zAxis);
				xAxis = DirectX::XMVector3Normalize(xAxis);
				DirectX::XMVECTOR yAxis = DirectX::XMVector3Cross(zAxis, xAxis);
				yAxis = DirectX::XMVector3Normalize(yAxis);
				DirectX::XMVECTOR pos = XMLoadFloat4(&_light.position);
				tView = DirectX::XMMATRIX(xAxis, yAxis, zAxis, pos);
				break;
			}
			default:
				break;
			}

			XMStoreFloat4x4(&view[0], tView);
		}

		void UpdateProjection(Light& _light, float _width, float _height)
		{
			DirectX::XMMATRIX tProjection;

			switch ((int)_light.position.w)
			{
			case 0:
			{
				tProjection = DirectX::XMMatrixOrthographicLH(_width, _height, NEAR_PLANE, _light.ratio.z);
				break;
			}
			case 1:
			{
				tProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90), 1, NEAR_PLANE, _light.ratio.z);
				break;
			}
			case 2:
			{
				tProjection = DirectX::XMMatrixPerspectiveFovLH(_light.ratio.y, 1, NEAR_PLANE, _light.ratio.z);
				break;
			}
			default:
				break;
			}

			XMStoreFloat4x4(&projection, tProjection);
		}

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