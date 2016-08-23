#pragma once
using namespace DirectX;

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

	//struct WorldViewProjectionConstantBuffer
	//{
	//	DirectX::XMFLOAT4X4 world;
	//	DirectX::XMFLOAT4X4 view[2];
	//	DirectX::XMFLOAT4X4 projection[2];
	//};

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct MInstancedConstantBuffer
	{
		DirectX::XMFLOAT4X4 model[12];
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Used to send per-vertex data to the vertex shader.
	struct RobustVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 texCoords;
		DirectX::XMFLOAT4 tangent;
		DirectX::XMFLOAT3 normal;
	};

	struct Particle
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT2 size;
		float age;
		unsigned int type;
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

	struct ScreenEffect
	{
		DirectX::XMFLOAT4 lossColor;
	};

	struct ViewProjectionLightBuffer
	{
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;

		void UpdateView(Light& _light, float _width, float _height,float _smSize,DirectX::XMFLOAT4X4 rotation)
		{
			DirectX::XMMATRIX tView = DirectX::XMMatrixIdentity();

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
				DirectX::XMVECTOR pos = XMLoadFloat4(&_light.position);
				pos.m128_f32[3] = 1.0f;

				DirectX::XMMATRIX XMRotation = DirectX::XMLoadFloat4x4(&rotation);
				//XMRotation = DirectX::XMMatrixMultiply(XMRotation,DirectX::XMMatrixTranslation(pos.m128_f32[0],pos.m128_f32[1],pos.m128_f32[2]));



				tView = DirectX::XMMATRIX(xAxis, yAxis, zAxis, pos);
				break;
			}
			case 1:
			{
				//Assuming w identifier for point light stays as 1
				//DirectX::XMVECTOR pos = XMLoadFloat4(&_light.position);

				//DirectX::XMVECTOR xAxis0 = { 0.0f, 0.0f, -1.0f, 0.0f };
				//DirectX::XMVECTOR yAxis0 = { 0.0f, 1.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR zAxis0 = { 1.0f, 0.0f, 0.0f, 0.0f };

				//DirectX::XMVECTOR xAxis1 = { 0.0f, 0.0f, 1.0f, 0.0f };
				//DirectX::XMVECTOR yAxis1 = { 0.0f, 1.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR zAxis1 = { -1.0f, 0.0f, 1.0f, 0.0f };
				//tView = DirectX::XMMATRIX(xAxis1, yAxis1, zAxis1, pos);
				//XMStoreFloat4x4(&view[1], tView);

				//DirectX::XMVECTOR xAxis2 = { 1.0f, 0.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR yAxis2 = { 0.0f, 0.0f, -1.0f, 0.0f };
				//DirectX::XMVECTOR zAxis2 = { 0.0f, 1.0f, 0.0f, 0.0f };
				//tView = DirectX::XMMATRIX(xAxis2, yAxis2, zAxis2, pos);
				//XMStoreFloat4x4(&view[2], tView);

				//DirectX::XMVECTOR xAxis3 = { 1.0f, 0.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR yAxis3 = { 0.0f, 0.0f, 1.0f, 0.0f };
				//DirectX::XMVECTOR zAxis3 = { 0.0f, -1.0f, 0.0f, 0.0f };
				//tView = DirectX::XMMATRIX(xAxis3, yAxis3, zAxis3, pos);
				//XMStoreFloat4x4(&view[3], tView);

				//DirectX::XMVECTOR xAxis4 = { 1.0f, 0.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR yAxis4 = { 0.0f, 1.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR zAxis4 = { 0.0f, 0.0f, 1.0f, 0.0f };
				//tView = DirectX::XMMATRIX(xAxis4, yAxis4, zAxis4, pos);
				//XMStoreFloat4x4(&view[4], tView);

				//DirectX::XMVECTOR xAxis5 = { -1.0f, 0.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR yAxis5 = { 0.0f, 1.0f, 0.0f, 0.0f };
				//DirectX::XMVECTOR zAxis5 = { 0.0f, 0.0f, -1.0f, 0.0f };
				//tView = DirectX::XMMATRIX(xAxis5, yAxis5, zAxis5, pos);
				//XMStoreFloat4x4(&view[5], tView);

				//Set tView for the storeFloat outside of the switch
				//tView = DirectX::XMMATRIX(xAxis0, yAxis0, zAxis0, pos);
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
				pos.m128_f32[3] = 1;
				tView = DirectX::XMMATRIX(xAxis, yAxis, zAxis, pos);
				break;
			}
			default:
				break;
			}


			FixView(_smSize);
			XMStoreFloat4x4(&view, DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(0,tView)));
		}

		void UpdateProjection(Light& _light, float _width, float _height)
		{
			DirectX::XMMATRIX tProjection = DirectX::XMMatrixIdentity();

			switch ((int)_light.position.w)
			{
			case 0:
			{
				tProjection = DirectX::XMMatrixOrthographicLH(_width, _height, NEAR_PLANE, _light.ratio.z);
				break;
			}
			case 1:
			{
				//tProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90), 1, NEAR_PLANE, _light.ratio.z);
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

			XMStoreFloat4x4(&projection, DirectX::XMMatrixTranspose(tProjection));
		}

		void FixView(float _smSize)
		{
			//ShimmerShadow View Fix

			XMFLOAT4 point = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			

			XMVECTOR lightspacePoint = XMVector4Transform(XMLoadFloat4(&point),XMLoadFloat4x4(&view));
			lightspacePoint = XMVector4Transform(XMLoadFloat4(&point), XMLoadFloat4x4(&projection));

			//assumes shadow map is square
			XMVECTOR textSpacePoint = lightspacePoint * _smSize * 0.5f;
			int wholeSpaceX = textSpacePoint.m128_f32[0];
			int wholeSpaceY = textSpacePoint.m128_f32[1];

			float errorX;
			float errorY;
			errorX = textSpacePoint.m128_f32[0] - wholeSpaceX;
			errorY = textSpacePoint.m128_f32[1] - wholeSpaceY;
			float isErrorX = errorX / (_smSize*0.5f);
			float isErrorY = errorY / (_smSize*0.5f);
			XMMATRIX round = XMMatrixTranslation(1.0f - isErrorX, 1.0f - isErrorY, 0);

			//XMStoreFloat4x4(&view, XMMatrixMultiply(round, XMLoadFloat4x4(&view)));
			//XMStoreFloat4x4(&projection, XMMatrixMultiply(round,XMMatrixInverse(0,XMMatrixTranspose(XMLoadFloat4x4(&projection)))));

		}
	};

	struct LightConstantBuffer
	{
		Light lights[7];
	};

	struct SpecularBufferCam
	{
		DirectX::XMFLOAT4 cameraPosition;
	};
}