#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "Model.h"
#include "ShadowMap.h"

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

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_inputLayoutParticle;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_vertexFloor;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_indexFloor;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShaderM;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShaderSky;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShaderSnow;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShaderInstanced;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_geometryShader;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_geometryShaderVP;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_geometryShaderSky;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		m_geometryShaderSnow;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShaderN;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShaderNS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShaderSky;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShaderShadow;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShaderSnow;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>			m_computeShaderSnow;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBufferM;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBufferVP;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_lightBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_lightBufferVP;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_camBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantInstanceBuffer;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_rasterizerStateCW;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		m_rasterizerStateCCW;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView1>		m_RTTRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		m_RTTDepthStencilView;
		
		Microsoft::WRL::ComPtr<ID3D11Texture2D>			m_RTTBackBuffer;
		D3D11_VIEWPORT										m_RTTViewport;


		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		ModelViewProjectionConstantBuffer	m_RTTCBufferData;
		ModelConstantBuffer					m_constantBufferDataM;
		ViewProjectionConstantBuffer		m_constantBufferDataVP;
		ViewProjectionLightBuffer			m_lightBufferDataVP;
		MInstancedConstantBuffer			m_constantInstanceData;
		LightConstantBuffer					m_lightBufferData;
		SpecularBufferCam					m_specBufferCamData;
		uint32								m_indexCount;
		uint32								m_indexFloorCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		//Samplers
		Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerFloor;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerPCF;

		//SRVs
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeNSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorNSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> flatNormalMapSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SkyCubeSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> RTTSRV;

		//ShadowMaps
		ShadowMap *shadowMap;

		//Models
		Model cube;
		Model pyramid;
		Model Goomba;
		Model GunTurret;
		Model Sphere;
		Model Wolf;

		XMFLOAT4X4 camera;
		XMFLOAT4X4 camera2;
		//world , proj;
	};
}

