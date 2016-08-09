#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "Model.h"

#include <DirectXMath.h>
using namespace DirectX;

#define NUM_LIGHTS 3


namespace GraphicsScene
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		void Sample3DSceneRenderer::InitializeLights();
		bool IsTracking() { return m_tracking; }


	private:
		void Rotate(float radians);
		void Sample3DSceneRenderer::Translate(float xOffset, float yOffset, float zOffset);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexFloor;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexFloor;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShaderN;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShaderNS;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_lightBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_camBuffer;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		LightConstantBuffer m_lightBufferData;
		SpecularBufferCam m_specBufferCamData;
		uint32	m_indexCount;
		uint32	m_indexFloorCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		//TODO:Memory
		//Samplers
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerFloor;

		//SRVs
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flatNormalMapSRV;


		//Models
		Model pyramid;
		Model Goomba;
		Model GunTurret;

		XMFLOAT4X4 camera;//world , proj;
	};
}

