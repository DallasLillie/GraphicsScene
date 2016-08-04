#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace GraphicsScene;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

	//XMStoreFloat4x4(&view, XMMatrixIdentity());

	static const XMVECTORF32 eye = { 0.0f, 0.0f, -2.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&camera, XMMatrixLookAtLH(eye, at, up));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, XMMatrixLookAtLH(eye, at, up))));
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
		);
}

using namespace Windows::UI::Core;
extern CoreWindow^ gwindow;
#include <atomic>
extern bool mouse_move;
extern float diffx;
extern float diffy;
extern bool w_down;
extern bool a_down;
extern bool s_down;
extern bool d_down;
extern bool left_click;

extern char buttons[256];

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}


	XMMATRIX newcamera = XMLoadFloat4x4(&camera);

	if (buttons['W'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * -timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons['A'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[0] * -timer.GetElapsedSeconds() *5.0;
	}

	if (buttons['S'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons['D'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[0] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons['R'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[1] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons['F'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[1] * -timer.GetElapsedSeconds() * 5.0;
	}

	Windows::UI::Input::PointerPoint^ point = nullptr;

	//if(mouse_move)/*This crashes unless a mouse event actually happened*/
	//point = Windows::UI::Input::PointerPoint::GetCurrentPoint(pointerID);

	if (mouse_move)
	{
		// Updates the application state once per frame.
		if (left_click)
		{
			XMVECTOR pos = newcamera.r[3];
			newcamera.r[3] = XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1));
			newcamera = XMMatrixRotationX(-diffy*0.01f) * newcamera * XMMatrixRotationY(-diffx*0.01f);
			newcamera.r[3] = pos;
		}
	}

	XMStoreFloat4x4(&camera, newcamera);

	/*Be sure to inverse the camera & Transpose because they don't use pragma pack row major in shaders*/
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newcamera)));

	mouse_move = false;/*Reset*/
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
		);

	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColorTexture);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		0
		);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->IASetInputLayout(m_inputLayout.Get());

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(
		0,
		1,
		m_constantBuffer.GetAddressOf(),
		nullptr,
		nullptr
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	ID3D11ShaderResourceView *SRV[]{ cubeSRV };
	context->PSSetShaderResources(0, 1, SRV);

	context->PSSetSamplers(0, 1, &sampler);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "WPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer) , D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {

		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColorTexture cubeVertices[] = 
		{
			//LEFT
			{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f),XMFLOAT2(1.0f,1.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f),XMFLOAT2(0.0f,1.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f),XMFLOAT2(1.0f,0.0f)},
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) ,XMFLOAT2(0.0f,0.0f) },

			//BACK
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f),XMFLOAT3(0.0f, 0.0f, 1.0f),XMFLOAT2(1.0f,1.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f),XMFLOAT2(0.0f,1.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) ,XMFLOAT2(1.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f),XMFLOAT2(0.0f,0.0f) },

			//RIGHT
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f),XMFLOAT2(1.0f,1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f),XMFLOAT2(0.0f,1.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f),XMFLOAT2(1.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f),XMFLOAT2(0.0f,0.0f) },

			//FRONT
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f),XMFLOAT2(1.0f,0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f),XMFLOAT2(0.0f,0.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) ,XMFLOAT2(1.0f,1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) ,XMFLOAT2(0.0f,1.0f) },

			//TOP
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f),XMFLOAT2(0.0f,1.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f),XMFLOAT2(0.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) ,XMFLOAT2(1.0f,1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) ,XMFLOAT2(1.0f,0.0f) },


			//BOTTOM
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) ,XMFLOAT2(1.0f,1.0f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f),XMFLOAT2(0.0f,1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) ,XMFLOAT2(1.0f,0.0f) },
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) ,XMFLOAT2(0.0f,0.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices [] =
		{
			2,1,0,
			2,3,1,

			6,5,4,
			6,7,5,

			10,9,8,
			10,11,9,

			14,13,12,
			14,15,13,

			18,17,16,
			18,19,17,

			22,21,20,
			22,23,21,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);


		D3D11_TEXTURE2D_DESC cubeTexture;
		D3D11_SUBRESOURCE_DATA texSubResData[Box_Wood02Dark_numlevels];
		ZeroMemory(&texSubResData, sizeof(texSubResData));
		ZeroMemory(&cubeTexture, sizeof(cubeTexture));

		cubeTexture.Width = Box_Wood02Dark_width;
		cubeTexture.Height = Box_Wood02Dark_height;
		cubeTexture.MipLevels = Box_Wood02Dark_numlevels;
		cubeTexture.ArraySize = 1;
		cubeTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		cubeTexture.SampleDesc.Count = 1;
		cubeTexture.Usage = D3D11_USAGE_IMMUTABLE;
		cubeTexture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		//cubeTexture.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		//cubeTexture.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		for (unsigned int i = 0; i < Box_Wood02Dark_numlevels; ++i)
		{
			ZeroMemory(&texSubResData[i], sizeof(texSubResData[i]));
			texSubResData[i].pSysMem = &Box_Wood02Dark_pixels[Box_Wood02Dark_leveloffsets[i]];
			texSubResData[i].SysMemPitch = (Box_Wood02Dark_width >> i) * sizeof(unsigned int);
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = cubeTexture.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;

		HRESULT result;
		//result = m_deviceResources->GetD3DDevice()->CreateTexture2D(&cubeTexture, texSubResData, &diffuseCubeTexture);
		//result = m_deviceResources->GetD3DDevice()->CreateShaderResourceView((ID3D11Resource*)&diffuseCubeTexture, &srvDesc, &cubeSRV);

		result =  CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Box_Wood02Dark.dds",
								(ID3D11Resource**)&diffuseCubeTexture, &cubeSRV);

		//HRESULT result =  CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"SkyboxOcean.dds",
		//						(ID3D11Resource**)&diffuseSkyBoxTexture, &SkyBoxSRV);
		
	});

	CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());

	m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &sampler);

	// Once the cube is loaded, the object is ready to be rendered.
	createCubeTask.then([this] () {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}