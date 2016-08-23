#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace GraphicsScene;

using namespace DirectX;
using namespace Windows::Foundation;

//TODO: buttons to toggle levels of detail
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
	InitializeLights();
	InitializeScreenEffect();

	static XMVECTORF32 eye = { 0.0f, 5.0f, -8.0f, 1.0f };
	static XMVECTORF32 at = { 0.0f, 2.0f, 0.0f, 1.0f };
	static XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&camera, XMMatrixInverse(0, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

	eye = { 1.5f, 3.0f, -1.0f, 1.0f };
	at = { 0.0f, 1.5f, 0.0f, 1.0f };
	up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&m_RTTCBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

	eye = { 0.0f, 10.0f, -0.01f, 1.0f };
	at = { 0.0f, 0.0f, 0.0f, 1.0f };
	up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&m_constantBufferDataVP.view[1], XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
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

	//Render-To-Texture
	{
	D3D11_TEXTURE2D_DESC tDesc;
	tDesc.ArraySize = 1;
	tDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	tDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	tDesc.Height = lround(m_deviceResources->GetLogicalSize().Height);
	tDesc.MipLevels = 1;
	tDesc.SampleDesc.Count = 1;
	tDesc.SampleDesc.Quality = 0;
	tDesc.Usage = D3D11_USAGE_DEFAULT;
	tDesc.Width = lround(m_deviceResources->GetLogicalSize().Width);
	tDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT result = m_deviceResources->GetD3DDevice()->CreateTexture2D(&tDesc, nullptr, m_RTTBackBuffer.GetAddressOf());
	result = m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_RTTBackBuffer.Get(),nullptr,RTTSRV.GetAddressOf());
	result = m_deviceResources->GetD3DDevice()->CreateRenderTargetView1(m_RTTBackBuffer.Get(), nullptr, m_RTTRenderTargetView.GetAddressOf());

	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(m_deviceResources->GetLogicalSize().Width),
		lround(m_deviceResources->GetLogicalSize().Height),
		1, // This depth stencil view has only one texture.
		1, // Use a single mipmap level.
		D3D11_BIND_DEPTH_STENCIL
	);

	ID3D11Texture2D* depthStencil;
	result = m_deviceResources->GetD3DDevice()->CreateTexture2D(
		&depthStencilDesc,
		nullptr,
		&depthStencil
	);


	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	result = m_deviceResources->GetD3DDevice()->CreateDepthStencilView(
		depthStencil,
		&depthStencilViewDesc,
		&m_RTTDepthStencilView
	);

	depthStencil->Release();
	depthStencil = nullptr;

	m_RTTViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_deviceResources->GetLogicalSize().Width,
		m_deviceResources->GetLogicalSize().Height
	);

	}

	//Render-To-Texture
	{
		D3D11_TEXTURE2D_DESC tDesc;
		tDesc.ArraySize = 1;
		tDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		tDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		tDesc.Height = lround(m_deviceResources->GetLogicalSize().Height);
		tDesc.MipLevels = 1;
		tDesc.SampleDesc.Count = 1;
		tDesc.SampleDesc.Quality = 0;
		tDesc.Usage = D3D11_USAGE_DEFAULT;
		tDesc.Width = lround(m_deviceResources->GetLogicalSize().Width);
		tDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		HRESULT result = m_deviceResources->GetD3DDevice()->CreateTexture2D(&tDesc, nullptr, m_RTTTexture.GetAddressOf());
		result = m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_RTTTexture.Get(), nullptr, RTTSRVScene.GetAddressOf());
		result = m_deviceResources->GetD3DDevice()->CreateRenderTargetView1(m_RTTTexture.Get(), nullptr, m_RTTRTVScene.GetAddressOf());

		CD3D11_TEXTURE2D_DESC depthStencilDesc(
			DXGI_FORMAT_D24_UNORM_S8_UINT,
			lround(m_deviceResources->GetLogicalSize().Width),
			lround(m_deviceResources->GetLogicalSize().Height),
			1, // This depth stencil view has only one texture.
			1, // Use a single mipmap level.
			D3D11_BIND_DEPTH_STENCIL
		);

		ID3D11Texture2D* depthStencil;
		result = m_deviceResources->GetD3DDevice()->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
		);


		CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
		result = m_deviceResources->GetD3DDevice()->CreateDepthStencilView(
			depthStencil,
			&depthStencilViewDesc,
			&m_RTTDSVScene
		);

		depthStencil->Release();
		depthStencil = nullptr;

		m_RTTSceneViewport = CD3D11_VIEWPORT(
			0.0f,
			0.0f,
			m_deviceResources->GetLogicalSize().Width,
			m_deviceResources->GetLogicalSize().Height
		);

	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		NEAR_PLANE,
		500.0f
	);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
	);

	m_RTTCBufferData.projection = m_constantBufferData.projection;
	m_constantBufferDataVP.projection[0] = m_constantBufferData.projection;
	m_constantBufferDataVP.projection[1] = m_constantBufferData.projection;
	//m_snowBufferWVPData.projection[0] = m_constantBufferDataVP.projection[0];
	//m_snowBufferWVPData.projection[1] = m_constantBufferDataVP.projection[1];
}

using namespace Windows::UI::Core;
extern CoreWindow^ gwindow;
extern bool mouse_move;
extern float diffx;
extern float diffy;
extern bool left_click;
XMMATRIX cubeWorld;

extern char buttons[256];

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	// Convert degrees to radians, then convert seconds to rotation angle
	float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
	double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
	float radians = static_cast<float>(fmod(totalRotation, XM_2PI));
	if (!m_tracking)
	{
		Rotate(radians);
	}


	XMMATRIX newcamera = XMLoadFloat4x4(&camera);

	//Camera Input
	if (buttons['W'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons['A'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[0] * -timer.GetElapsedSeconds() *5.0;
	}

	if (buttons['S'])
	{
		newcamera.r[3] = newcamera.r[3] + newcamera.r[2] * -timer.GetElapsedSeconds() * 5.0;
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


	//ScreenEffect Colors
	//TODO: make the buttons add together rather than just toggle which is on
	//Red
	if (buttons['T'])
	{
		m_screenEffectData.lossColor.x = 0.0f;
		m_screenEffectData.lossColor.y = 0.1f;
		m_screenEffectData.lossColor.z = 0.1f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Green
	if (buttons['G'])
	{
		m_screenEffectData.lossColor.x = 0.1f;
		m_screenEffectData.lossColor.y = 0.0f;
		m_screenEffectData.lossColor.z = 0.1f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Blue
	if (buttons['B'])
	{
		m_screenEffectData.lossColor.x = 0.1f;
		m_screenEffectData.lossColor.y = 0.1f;
		m_screenEffectData.lossColor.z = 0.0f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Normal
	if (buttons['N'])
	{
		m_screenEffectData.lossColor.x = 0.0f;
		m_screenEffectData.lossColor.y = 0.0f;
		m_screenEffectData.lossColor.z = 0.0f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Yellow
	if (buttons['Y'])
	{
		m_screenEffectData.lossColor.x = 0.0f;
		m_screenEffectData.lossColor.y = 0.0f;
		m_screenEffectData.lossColor.z = 0.1f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Magenta
	if (buttons['H'])
	{
		m_screenEffectData.lossColor.x = 0.0f;
		m_screenEffectData.lossColor.y = 0.1f;
		m_screenEffectData.lossColor.z = 0.0f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Cyan
	if (buttons['C'])
	{
		m_screenEffectData.lossColor.x = 0.1f;
		m_screenEffectData.lossColor.y = 0.0f;
		m_screenEffectData.lossColor.z = 0.0f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//White
	if (buttons['U'])
	{
		m_screenEffectData.lossColor.x = -0.1f;
		m_screenEffectData.lossColor.y = -0.1f;
		m_screenEffectData.lossColor.z = -0.1f;
		m_screenEffectData.lossColor.w = 0.0f;
	}
	//Black
	if (buttons['I'])
	{
		m_screenEffectData.lossColor.x = 0.1f;
		m_screenEffectData.lossColor.y = 0.1f;
		m_screenEffectData.lossColor.z = 0.1f;
		m_screenEffectData.lossColor.w = 0.0f;
	}

	if (false)
	{
		Size size;
		size.Height = m_deviceResources->GetLogicalSize().Height;
		m_deviceResources->SetLogicalSize(size);
	}


	XMMATRIX pointLight = XMMatrixIdentity();
	XMVECTOR lightpos = XMLoadFloat4(&m_lightBufferData.lights[1].position);

	//PointLight Input
	if (buttons[VK_UP])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[2] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons[VK_LEFT])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[0] * -timer.GetElapsedSeconds() *5.0;
	}

	if (buttons[VK_DOWN])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[2] * -timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons[VK_RIGHT])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[0] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons[VK_OEM_7])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[1] * timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons[VK_OEM_2])
	{
		pointLight.r[3] = pointLight.r[3] + pointLight.r[1] * -timer.GetElapsedSeconds() * 5.0;
	}

	if (buttons[VK_OEM_PERIOD])
	{
		if (m_lightBufferData.lights[1].ratio.w)
		{
			m_lightBufferData.lights[1].ratio.w = 0;
		}
		else
		{
			m_lightBufferData.lights[1].ratio.w = 1;
		}

		buttons[VK_OEM_PERIOD] = false;
	}

	lightpos = XMVector4Transform(lightpos, pointLight);
	XMStoreFloat4(&m_lightBufferData.lights[1].position, lightpos);
	m_lightBufferData.lights[1].position.w = 1;


	//RightClickButton
	if (buttons[VK_RBUTTON])
	{
		if (m_lightBufferData.lights[2].ratio.w)
		{
			m_lightBufferData.lights[2].ratio.w = 0;
		}
		else
		{
			m_lightBufferData.lights[2].ratio.w = 1;
		}

		buttons[VK_RBUTTON] = false;
	}

	Windows::UI::Input::PointerPoint^ point = nullptr;

	//if(mouse_move)/*This crashes unless a mouse event actually happened*/
	//point = Windows::UI::Input::PointerPoint::GetCurrentPoint(pointerID);

	if (mouse_move)
	{
		// Updates the application state once per frame.
		if (buttons[VK_LBUTTON])
		{
			XMVECTOR pos = newcamera.r[3];
			newcamera.r[3] = XMLoadFloat4(&XMFLOAT4(0, 0, 0, 1));
			newcamera = XMMatrixTranslation(-3, 0.5, -3)* newcamera;
			newcamera = XMMatrixRotationX(diffy*0.01f) * newcamera * XMMatrixRotationY(diffx*0.01f);
			newcamera.r[3] = pos;
		}
	}

	XMStoreFloat4x4(&camera, newcamera);

	cubeWorld = XMMatrixIdentity();
	cubeWorld = XMMatrixRotationY(radians);
	cubeWorld = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.5f, 0.0f), cubeWorld);
	cubeWorld = XMMatrixMultiply(XMMatrixScaling(0.125f, 0.125f, 0.125f), cubeWorld);


	//GunMatrix
	XMMATRIX model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.0f, 1.0f, 0.0f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(135)), model);
	//model = XMMatrixMultiply(XMMatrixScaling(0.2f, 0.2f, 0.2f), model);
	model = XMMatrixMultiply(model, cubeWorld);
	XMStoreFloat4x4(&GunTurret.GetWorldMatrix(), XMMatrixTranspose(model));

	//StatueMatrix
	model = XMMatrixIdentity();
	//model = XMMatrixMultiply(XMMatrixTranslation(5.0f, 1.18f, 0.0f), model);
	//model = XMMatrixMultiply(XMMatrixScaling(0.005f, 0.005f, 0.005f), model);
	XMStoreFloat4x4(&Statue.GetWorldMatrix(), XMMatrixTranspose(model));

	//cubeWorld = XMMatrixTranspose(cubeWorld);

	//FountainMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.03f, 0.0f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(-radians), model);
	model = XMMatrixMultiply(XMMatrixScaling(0.5f, 0.5f, 0.5f), model);
	XMMATRIX parent = model;
	//model = XMMatrixMultiply(model, XMMatrixRotationY(radians));
	XMStoreFloat4x4(&fountain.GetWorldMatrix(), XMMatrixTranspose(model));

	//KnightMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.35f, 4.0f+(0.125f*cos(radians*4.0f)), 0.0f), model);
	//model = XMMatrixMultiply(model,XMMatrixTranslation(0.0f, 0.0f, 10.0f));
	//model = XMMatrixMultiply(XMMatrixTranslation(5.0f, 1.18f, 0.0f), model);
	model = XMMatrixMultiply(XMMatrixRotationX(XMConvertToRadians(-90)), model);
	//model = XMMatrixMultiply(XMMatrixRotationZ(radians), model);
	model = XMMatrixMultiply(XMMatrixRotationZ(XMConvertToRadians(90)), model);
	//model = XMMatrixMultiply(XMMatrixRotationZ(XMConvertToRadians(90)), model);
	model = XMMatrixMultiply(XMMatrixScaling(0.005f, 0.005f, 0.005f), model);
	//model = XMMatrixMultiply(model, parent);

	XMStoreFloat4x4(&Knight.GetWorldMatrix(), XMMatrixTranspose(model));

	//GrappleGirlMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(-5.0f, 0.0f, -0.1f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(90)), model);
	//model = XMMatrixMultiply(XMMatrixRotationY(radians), model);
	model = XMMatrixMultiply(XMMatrixScaling(0.5f, 0.5f, 0.5f), model);

	//model = XMMatrixMultiply(model, XMMatrixTranspose(XMLoadFloat4x4(&fountain.GetWorldMatrix())));
	XMStoreFloat4x4(&GrappleGirl.GetWorldMatrix(), XMMatrixTranspose(model));



	//SphereMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.0f, 6.0f, 0.0f), model);
	//model = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(90)), model);
	//model = XMMatrixMultiply(XMMatrixRotationY(radians), model);
	model = XMMatrixMultiply(XMMatrixScaling(0.2f, 0.2f, 0.2f), model);

	//model = XMMatrixMultiply(model, XMMatrixTranspose(XMLoadFloat4x4(&fountain.GetWorldMatrix())));
	XMStoreFloat4x4(&Sphere.GetWorldMatrix(), XMMatrixTranspose(model));



	//WolfMatrix
	for (unsigned int i = 0; i < 12; ++i)
	{
		model = XMMatrixIdentity();
		XMVECTOR position;
		position.m128_f32[0] = cos(360/13 * (i + 1)) * 5;
		position.m128_f32[1] = 0.0f;
		position.m128_f32[2] = sin(360/13 * (i + 1)) * 5;
		position.m128_f32[3] = 1;

		XMVECTOR zAxis =XMVectorSubtract(XMLoadFloat4(&XMFLOAT4(0, 0.0f, 0,0)),position);
		zAxis = XMVector3Normalize(zAxis);
		zAxis.m128_f32[3] = 0.0f;
		XMVECTOR xAxis = XMVector3Cross(XMLoadFloat4(&XMFLOAT4(0, 1, 0,0)), zAxis);
		xAxis = XMVector3Normalize(xAxis);
		XMVECTOR yAxis = XMVector3Cross(zAxis,xAxis);
		yAxis = XMVector3Normalize(yAxis);

		model = XMMATRIX(xAxis, yAxis, zAxis,position);

		//model = XMMatrixMultiply(XMMatrixScaling(0.4f+(i*0.1), 0.4f + (i*0.1), 0.4f + (i*0.1)), model);
		XMStoreFloat4x4(&Wolf.GetWorldMatrix(), XMMatrixTranspose(model));


		m_constantInstanceData.model[i] = Wolf.GetWorldMatrix();

	}


	//HEADLamp Calculations
	XMMATRIX headLamp = XMMatrixIdentity();
	headLamp = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.25f, 0.0f), headLamp);
	headLamp = XMMatrixMultiply(XMMatrixRotationX(XMConvertToRadians(10)), headLamp);
	headLamp = XMMatrixMultiply(headLamp, newcamera);

	XMStoreFloat4(&m_lightBufferData.lights[2].position, headLamp.r[3]);
	XMStoreFloat4(&m_lightBufferData.lights[2].normal, headLamp.r[2]);
	m_lightBufferData.lights[2].position.w = 2;

	for (unsigned int i = 3; i < 7; ++i)
	{
		XMVECTOR pos = XMLoadFloat4(&m_lightBufferData.lights[i].position);
		XMVECTOR norm = XMLoadFloat4(&m_lightBufferData.lights[i].normal);
		headLamp = XMMatrixIdentity();
		headLamp = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(2.5)), headLamp);

		pos = XMVector4Transform(pos, headLamp);
		norm = XMVector4Transform(norm, headLamp);

		XMStoreFloat4(&m_lightBufferData.lights[i].position, pos);
		XMStoreFloat4(&m_lightBufferData.lights[i].normal, norm);
		m_lightBufferData.lights[i].position.w = 2;
	}



	if (buttons['M'])
	{
		if (m_lightBufferData.lights[0].ratio.w)
		{
			m_lightBufferData.lights[0].ratio.w = 0;
		}
		else
		{
			m_lightBufferData.lights[0].ratio.w = 1;
		}

		buttons['M'] = false;
	}

	XMMATRIX directionalLight = XMMatrixIdentity();
	XMVECTOR lightNorm = XMLoadFloat4(&m_lightBufferData.lights[0].normal);
	if (buttons['L'])
	{
		directionalLight = XMMatrixMultiply(directionalLight,XMMatrixRotationAxis(XMVECTOR(XMLoadFloat3(&XMFLOAT3(0.0f,1.0f,0.0f))),XMConvertToRadians(0.5)));
	}
	if (buttons['K'])
	{
		directionalLight = XMMatrixMultiply(directionalLight, XMMatrixRotationAxis(XMVECTOR(XMLoadFloat3(&XMFLOAT3(0.0f, 1.0f, 0.0f))), XMConvertToRadians(-0.5)));
	}
	//directionalLight = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(1)), directionalLight);
	//directionalLight = XMMatrixTranspose(directionalLight);
	lightNorm = XMVector4Transform(lightNorm, directionalLight);
	lightNorm = XMVector3Normalize(lightNorm);
	XMStoreFloat4(&m_lightBufferData.lights[0].normal, lightNorm);

	XMVECTOR lightPos = XMLoadFloat4(&m_lightBufferData.lights[0].position);
	lightPos = XMVector4Transform(lightPos, directionalLight);
	XMStoreFloat4(&m_lightBufferData.lights[0].position, lightPos);
	m_lightBufferData.lights[0].position.w = 0.0f;

	//TODO: both viewports
	m_specBufferCamData.cameraPosition.x = camera._41;
	m_specBufferCamData.cameraPosition.y = camera._42;
	m_specBufferCamData.cameraPosition.z = camera._43;
	m_specBufferCamData.cameraPosition.w = camera._44;


	/*Be sure to inverse the camera & Transpose because they don't use pragma pack row major in shaders*/
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newcamera)));

	m_constantBufferDataVP.view[0] = m_constantBufferData.view;
	m_constantBufferDataVP.view[1] = m_constantBufferData.view;

	//m_snowBufferWVPData.view[0] = m_constantBufferDataVP.view[0];
	//m_snowBufferWVPData.view[1] = m_constantBufferDataVP.view[1];

	float dimension = 30;

	XMFLOAT4X4 rotation;
	XMStoreFloat4x4(&rotation, directionalLight);
	if (buttons['Z'])
	{
		m_lightBufferDataVP.UpdateView(m_lightBufferData.lights[3], dimension, dimension, 2048, rotation);
		m_lightBufferDataVP.UpdateProjection(m_lightBufferData.lights[3], dimension, dimension);
	}
	else
	{
		m_lightBufferDataVP.UpdateView(m_lightBufferData.lights[0], dimension, dimension,2048, rotation);
		m_lightBufferDataVP.UpdateProjection(m_lightBufferData.lights[0], dimension, dimension);
	}
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

	XMMATRIX model = XMMatrixIdentity();
	UINT stride = sizeof(RobustVertex);
	UINT offset = 0;

	//First Pass RTT
	//TODO:: abstract this to be called in places like graphics SceneMain
	context->RSSetViewports(1, &m_RTTViewport);
	ID3D11RenderTargetView *const targets[1] = { m_RTTRenderTargetView.Get() };
	context->OMSetRenderTargets(1, targets, m_RTTDepthStencilView.Get());

	context->ClearRenderTargetView(m_RTTRenderTargetView.Get(), DirectX::Colors::White);
	context->ClearDepthStencilView(m_RTTDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	//TODO: should be done after shadowmap
	//Draw Gunturret
	{
		//RESET GS
		ID3D11GeometryShader *tempGS = nullptr;
		context->GSSetShader(tempGS, nullptr, 0);

		m_RTTCBufferData.model = GunTurret.GetWorldMatrix();

		// Attach our pixel shader.
		context->PSSetShader(
			m_pixelShaderNS.Get(),
			nullptr,
			0
		);

		// Attach our vertex shader.
		context->VSSetShader(
			m_vertexShader.Get(),
			nullptr,
			0
		);

		context->PSSetConstantBuffers1(
			0,
			1,
			m_lightBuffer.GetAddressOf(),
			nullptr,
			nullptr
		);

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource1(
			m_constantBuffer.Get(),
			0,
			NULL,
			&m_RTTCBufferData,
			0,
			0,
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

		context->UpdateSubresource1(
			m_camBuffer.Get(),
			0,
			NULL,
			&m_specBufferCamData,
			0,
			0,
			0
		);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_inputLayout.Get());
		
		context->PSSetShaderResources(0, 1, GunTurret.GetSRV().GetAddressOf());
		context->PSSetShaderResources(1, 1, GunTurret.GetNormalMap().GetAddressOf());
		context->PSSetShaderResources(2, 1, GunTurret.GetSpecularMap().GetAddressOf());

		stride = sizeof(RobustVertex);
		offset = 0;
		context->IASetVertexBuffers(
			0,
			1,
			GunTurret.GetVertexBuffer().GetAddressOf(),
			&stride,
			&offset
		);

		context->IASetIndexBuffer(
			GunTurret.GetIndexBuffer().Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
		);

		context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());
		context->PSSetSamplers(1, 1, samplerPCF.GetAddressOf());


		// Draw the objects.
		context->DrawIndexed(
			GunTurret.GetIndices().size(),
			0,
			0
		);

	}

	//First Pass Shadows

	shadowMap->BindDSVandSetNullRenderTarget(context);
	XMFLOAT4X4 tempView = m_constantBufferDataVP.view[0];
	XMFLOAT4X4 tempView1 = m_constantBufferDataVP.view[1];
	XMFLOAT4X4 tempProj = m_constantBufferDataVP.projection[0];
	XMFLOAT4X4 tempProj1 = m_constantBufferDataVP.projection[1];
	m_constantBufferDataVP.view[0] = m_lightBufferDataVP.view;
	m_constantBufferDataVP.view[1] = m_lightBufferDataVP.view;
	m_constantBufferDataVP.projection[0] = m_lightBufferDataVP.projection;
	m_constantBufferDataVP.projection[1] = m_lightBufferDataVP.projection;

	//Draw Things
	{
		context->RSSetState(m_rasterizerStateCW.Get());

		//DrawCube
		{
			//model = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
			//model = XMMatrixScaling(0.51f, 0.51f, 0.51f)* model;
			XMStoreFloat4x4(&m_constantBufferDataM.model, XMMatrixTranspose(cubeWorld));

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);

			context->UpdateSubresource1(
				m_lightBufferVP.Get(),
				0,
				NULL,
				&m_lightBufferDataVP,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.
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
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->GSSetConstantBuffers1(
				1,
				1,
				m_lightBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderVP.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderShadow.Get(),
				nullptr,
				0
			);

			// Draw the objects.
			context->DrawIndexed(
				m_indexCount,
				0,
				0
			);

		}

		//DRAWPYRAMID
		{
			m_constantBufferDataM.model = fountain.GetWorldMatrix();

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.
			context->IASetVertexBuffers(
				0,
				1,
				fountain.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				fountain.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->IASetInputLayout(m_inputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderVP.Get(),
				nullptr,
				0
			);

			// Draw the objects.
			context->DrawIndexed(
				fountain.GetIndices().size(),
				0,
				0
			);

		}


		//Draw GrappleGirl
		{
			m_constantBufferDataM.model = GrappleGirl.GetWorldMatrix();

			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				GrappleGirl.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				GrappleGirl.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);
			// Draw the objects.
			context->DrawIndexed(
				GrappleGirl.GetIndices().size(),
				0,
				0
			);
		}


		//Draw Wolves
		{
			m_constantInstanceData.view = m_constantBufferData.view;
			m_constantInstanceData.projection = m_constantBufferData.projection;

			context->UpdateSubresource1(
				m_constantInstanceBuffer.Get(),
				0,
				NULL,
				&m_constantInstanceData,
				0,
				0,
				0
			);

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderInstanced.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantInstanceBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				Wolf.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Wolf.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			//Draw the objects.
			context->DrawIndexedInstanced(
				Wolf.GetIndices().size(),
				12,
				0,
				0,
				0
			);

		}

		//Draw Gunturret
		{
			//m_constantBufferDataM.model = GunTurret.GetWorldMatrix();

			//// Attach our vertex shader.
			//context->VSSetShader(
			//	m_vertexShaderM.Get(),
			//	nullptr,
			//	0
			//);

			//// Prepare the constant buffer to send it to the graphics device.
			//context->UpdateSubresource1(
			//	m_constantBufferM.Get(),
			//	0,
			//	NULL,
			//	&m_constantBufferDataM,
			//	0,
			//	0,
			//	0
			//);

			//// Send the constant buffer to the graphics device.
			//context->VSSetConstantBuffers1(
			//	0,
			//	1,
			//	m_constantBufferM.GetAddressOf(),
			//	nullptr,
			//	nullptr
			//);

			//stride = sizeof(RobustVertex);
			//offset = 0;
			//context->IASetVertexBuffers(
			//	0,
			//	1,
			//	GunTurret.GetVertexBuffer().GetAddressOf(),
			//	&stride,
			//	&offset
			//);

			//context->IASetIndexBuffer(
			//	GunTurret.GetIndexBuffer().Get(),
			//	DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			//	0
			//);

			//// Draw the objects.
			//context->DrawIndexed(
			//	GunTurret.GetIndices().size(),
			//	0,
			//	0
			//);

		}

		//Draw Knight
		{
			m_constantBufferDataM.model = Knight.GetWorldMatrix();

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			context->PSSetConstantBuffers1(
				0,
				1,
				m_lightBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->UpdateSubresource1(
				m_camBuffer.Get(),
				0,
				NULL,
				&m_specBufferCamData,
				0,
				0,
				0
			);

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				Knight.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Knight.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


			// Draw the objects.
			context->DrawIndexed(
				Knight.GetIndices().size(),
				0,
				0
			);

		}


		//DrawStatue
		{
			m_constantBufferDataM.model = Statue.GetWorldMatrix();

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);

			context->UpdateSubresource1(
				m_lightBufferVP.Get(),
				0,
				NULL,
				&m_lightBufferDataVP,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.
			context->IASetVertexBuffers(
				0,
				1,
				Statue.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Statue.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->IASetInputLayout(m_inputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->GSSetConstantBuffers1(
				1,
				1,
				m_lightBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderVP.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderShadow.Get(),
				nullptr,
				0
			);

			// Draw the objects.
			context->DrawIndexed(
				Statue.GetIndices().size(),
				0,
				0
			);

		}

}







	//Second Pass
	m_constantBufferDataVP.view[0] = tempView;
	m_constantBufferDataVP.view[1] = tempView1;
	m_constantBufferDataVP.projection[0] = tempProj;
	m_constantBufferDataVP.projection[1] = tempProj1;
	//TODO::better way to to do this
	D3D11_VIEWPORT m_screenViewport[2];
	m_screenViewport[0] = m_deviceResources->GetScreenViewport();
	m_screenViewport[1] = m_deviceResources->GetScreenViewport2();
	context->RSSetViewports(2, m_screenViewport);

	ID3D11RenderTargetView *const target[1] = { m_RTTRTVScene.Get() };
	context->OMSetRenderTargets(1, target, m_RTTDSVScene.Get());
	context->ClearRenderTargetView(m_RTTRTVScene.Get(), DirectX::Colors::White);
	context->ClearDepthStencilView(m_RTTDSVScene.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	
	//RESET GS
	ID3D11GeometryShader *tempGS = nullptr;
	context->GSSetShader(tempGS, nullptr, 0);

	//Draw Things
	{
		//Skybox
		{
			model.r[3].m128_f32[0] = camera._41;
			model.r[3].m128_f32[1] = camera._42;
			model.r[3].m128_f32[2] = camera._43;
			model.r[3].m128_f32[3] = camera._44;
			XMStoreFloat4x4(&m_constantBufferDataM.model, XMMatrixTranspose(model));

			context->RSSetState(m_rasterizerStateCCW.Get());
			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.

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
				m_vertexShaderSky.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderSky.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderSky.Get(),
				nullptr,
				0
			);

			context->PSSetShaderResources(0, 1, SkyCubeSRV.GetAddressOf());

			context->PSSetSamplers(0, 1, sampler.GetAddressOf());

			// Draw the objects.
			context->DrawIndexed(
				m_indexCount,
				0,
				0
			);

		}

		context->ClearDepthStencilView(m_RTTDSVScene.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		context->RSSetState(m_rasterizerStateCW.Get());
		//DrawCube
		{
			model = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
			model = XMMatrixScaling(0.51f, 0.51f, 0.51f)* model;
			XMStoreFloat4x4(&m_constantBufferDataM.model, XMMatrixTranspose(cubeWorld));

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_lightBuffer.Get(),
				0,
				NULL,
				&m_lightBufferData,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.
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
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderVP.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderN.Get(),
				nullptr,
				0
			);

			context->PSSetConstantBuffers1(
				0,
				1,
				m_lightBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->PSSetConstantBuffers1(
				1,
				1,
				m_camBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);


			context->PSSetShaderResources(0, 1, RTTSRV.GetAddressOf());
			context->PSSetShaderResources(1, 1, flatNormalMapSRV.GetAddressOf());
			context->PSSetShaderResources(2, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			context->PSSetSamplers(0, 1, sampler.GetAddressOf());
			context->PSSetSamplers(1, 1, samplerPCF.GetAddressOf());

			// Draw the objects.
			context->DrawIndexed(
				m_indexCount,
				0,
				0
			);

		}



		//DrawFloor
		{
			model = XMMatrixIdentity();
			XMStoreFloat4x4(&m_constantBufferDataM.model, XMMatrixTranspose(model));

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);

			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->PSSetShaderResources(0, 1, floorSRV.GetAddressOf());
			context->PSSetShaderResources(1, 1, floorNSRV.GetAddressOf());
			context->PSSetShaderResources(2, 1, floorSSRV.GetAddressOf());
			context->PSSetShaderResources(3, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				m_vertexFloor.GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				m_indexFloor.Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderNS.Get(),
				nullptr,
				0
			);

			context->GSSetShader(m_geometryShader.Get(), nullptr, 0);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

			context->IASetInputLayout(m_inputLayout.Get());

			context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


			// Draw the objects.
			context->DrawIndexed(
				m_indexFloorCount,
				0,
				0
			);
		}





		//DrawFountain
		{
			m_constantBufferDataM.model = fountain.GetWorldMatrix();

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_constantBufferVP.Get(),
				0,
				NULL,
				&m_constantBufferDataVP,
				0,
				0,
				0
			);
			context->UpdateSubresource1(
				m_lightBuffer.Get(),
				0,
				NULL,
				&m_lightBufferData,
				0,
				0,
				0
			);

			// Each vertex is one instance of the VertexPositionColor struct.
			context->IASetVertexBuffers(
				0,
				1,
				fountain.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				fountain.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->IASetInputLayout(m_inputLayout.Get());

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Send the constant buffer to the graphics device.
			context->GSSetConstantBuffers1(
				0,
				1,
				m_constantBufferVP.GetAddressOf(),
				nullptr,
				nullptr
			);
			// Attach our pixel shader.
			context->GSSetShader(
				m_geometryShaderVP.Get(),
				nullptr,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderN.Get(),
				nullptr,
				0
			);

			context->PSSetConstantBuffers1(
				0,
				1,
				m_lightBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			//TODO: camera position changes for each viewport
			context->PSSetConstantBuffers1(
				1,
				1,
				m_camBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);


			context->PSSetShaderResources(0, 1, fountain.GetSRV().GetAddressOf());
			context->PSSetShaderResources(1, 1, fountain.GetNormalMap().GetAddressOf());
			context->PSSetShaderResources(2, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			context->PSSetSamplers(0, 1, sampler.GetAddressOf());

			// Draw the objects.
			context->DrawIndexed(
				fountain.GetIndices().size(),
				0,
				0
			);

		}


		//Draw GrappleGirl
		{
			m_constantBufferDataM.model = GrappleGirl.GetWorldMatrix();

			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderNS.Get(),
				nullptr,
				0
			);

			context->PSSetShaderResources(0, 1, GrappleGirl.GetSRV().GetAddressOf());
			context->PSSetShaderResources(1, 1, GrappleGirl.GetNormalMap().GetAddressOf());
			context->PSSetShaderResources(2, 1, GrappleGirl.GetSpecularMap().GetAddressOf());
			context->PSSetShaderResources(3, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			context->PSSetSamplers(0, 1, sampler.GetAddressOf());

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				GrappleGirl.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				GrappleGirl.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			// Draw the objects.
			context->DrawIndexed(
				GrappleGirl.GetIndices().size(),
				0,
				0
			);
		}


		//Draw Wolves
		{
			m_constantInstanceData.view = m_constantBufferData.view;
			m_constantInstanceData.projection = m_constantBufferData.projection;

			context->UpdateSubresource1(
				m_constantInstanceBuffer.Get(),
				0,
				NULL,
				&m_constantInstanceData,
				0,
				0,
				0
			);

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderInstanced.Get(),
				nullptr,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantInstanceBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->PSSetShaderResources(0, 1, Wolf.GetSRV().GetAddressOf());
			context->PSSetShaderResources(1, 1, Wolf.GetNormalMap().GetAddressOf());
			context->PSSetShaderResources(2, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderN.Get(),
				nullptr,
				0
			);

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				Wolf.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Wolf.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


			// Draw the objects.
			context->DrawIndexedInstanced(
				Wolf.GetIndices().size(),
				12,
				0,
				0,
				0
			);

		}

		//Draw Gunturret
		{
		//m_constantBufferDataM.model = GunTurret.GetWorldMatrix();

		//// Attach our pixel shader.
		//context->PSSetShader(
		//	m_pixelShaderNS.Get(),
		//	nullptr,
		//	0
		//);

		//// Attach our vertex shader.
		//context->VSSetShader(
		//	m_vertexShaderM.Get(),
		//	nullptr,
		//	0
		//);

		//context->PSSetConstantBuffers1(
		//	0,
		//	1,
		//	m_lightBuffer.GetAddressOf(),
		//	nullptr,
		//	nullptr
		//);

		//// Prepare the constant buffer to send it to the graphics device.
		//context->UpdateSubresource1(
		//	m_constantBufferM.Get(),
		//	0,
		//	NULL,
		//	&m_constantBufferDataM,
		//	0,
		//	0,
		//	0
		//);

		//// Send the constant buffer to the graphics device.
		//context->VSSetConstantBuffers1(
		//	0,
		//	1,
		//	m_constantBufferM.GetAddressOf(),
		//	nullptr,
		//	nullptr
		//);

		//context->UpdateSubresource1(
		//	m_camBuffer.Get(),
		//	0,
		//	NULL,
		//	&m_specBufferCamData,
		//	0,
		//	0,
		//	0
		//);

		//context->PSSetShaderResources(0, 1, GunTurret.GetSRV().GetAddressOf());
		//context->PSSetShaderResources(1, 1, GunTurret.GetNormalMap().GetAddressOf());
		//context->PSSetShaderResources(2, 1, GunTurret.GetSpecularMap().GetAddressOf());
		//context->PSSetShaderResources(3, 1, shadowMap->m_ShadowSRV.GetAddressOf());

		//stride = sizeof(RobustVertex);
		//offset = 0;
		//context->IASetVertexBuffers(
		//	0,
		//	1,
		//	GunTurret.GetVertexBuffer().GetAddressOf(),
		//	&stride,
		//	&offset
		//);

		//context->IASetIndexBuffer(
		//	GunTurret.GetIndexBuffer().Get(),
		//	DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
		//	0
		//);

		//context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


		//// Draw the objects.
		//context->DrawIndexed(
		//	GunTurret.GetIndices().size(),
		//	0,
		//	0
		//);

		}

		//Draw Sphere
		{
			m_constantBufferDataM.model = Sphere.GetWorldMatrix();

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderN.Get(),
				nullptr,
				0
			);

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			context->PSSetConstantBuffers1(
				0,
				1,
				m_lightBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->UpdateSubresource1(
				m_camBuffer.Get(),
				0,
				NULL,
				&m_specBufferCamData,
				0,
				0,
				0
			);

			context->PSSetShaderResources(0, 1, Sphere.GetSRV().GetAddressOf());
			context->PSSetShaderResources(1, 1, Sphere.GetNormalMap().GetAddressOf());
			context->PSSetShaderResources(2, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				Sphere.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Sphere.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


			// Draw the objects.
			context->DrawIndexed(
				Sphere.GetIndices().size(),
				0,
				0
			);

		}


		//Draw Knight
		{
			m_constantBufferDataM.model = Knight.GetWorldMatrix();

			// Attach our pixel shader.
			context->PSSetShader(
				m_pixelShaderNS.Get(),
				nullptr,
				0
			);

			// Attach our vertex shader.
			context->VSSetShader(
				m_vertexShaderM.Get(),
				nullptr,
				0
			);

			context->PSSetConstantBuffers1(
				0,
				1,
				m_lightBuffer.GetAddressOf(),
				nullptr,
				nullptr
			);

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
				0,
				0
			);

			// Send the constant buffer to the graphics device.
			context->VSSetConstantBuffers1(
				0,
				1,
				m_constantBufferM.GetAddressOf(),
				nullptr,
				nullptr
			);

			context->UpdateSubresource1(
				m_camBuffer.Get(),
				0,
				NULL,
				&m_specBufferCamData,
				0,
				0,
				0
			);

			context->PSSetShaderResources(0, 1, Knight.GetSRV().GetAddressOf());
			context->PSSetShaderResources(1, 1, Knight.GetNormalMap().GetAddressOf());
			context->PSSetShaderResources(2, 1, Knight.GetSpecularMap().GetAddressOf());
			context->PSSetShaderResources(3, 1, shadowMap->m_ShadowSRV.GetAddressOf());

			stride = sizeof(RobustVertex);
			offset = 0;
			context->IASetVertexBuffers(
				0,
				1,
				Knight.GetVertexBuffer().GetAddressOf(),
				&stride,
				&offset
			);

			context->IASetIndexBuffer(
				Knight.GetIndexBuffer().Get(),
				DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
				0
			);

			context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());


			// Draw the objects.
			context->DrawIndexed(
				Knight.GetIndices().size(),
				0,
				0
			);

		}

		//DrawStatue
		{
			m_constantBufferDataM.model = Statue.GetWorldMatrix();

			// Prepare the constant buffer to send it to the graphics device.
			context->UpdateSubresource1(
				m_constantBufferM.Get(),
				0,
				NULL,
				&m_constantBufferDataM,
				0,
0,
0
);
context->UpdateSubresource1(
	m_constantBufferVP.Get(),
	0,
	NULL,
	&m_constantBufferDataVP,
	0,
	0,
	0
);

context->UpdateSubresource1(
	m_lightBufferVP.Get(),
	0,
	NULL,
	&m_lightBufferDataVP,
	0,
	0,
	0
);

// Each vertex is one instance of the VertexPositionColor struct.
context->IASetVertexBuffers(
	0,
	1,
	Statue.GetVertexBuffer().GetAddressOf(),
	&stride,
	&offset
);

context->IASetIndexBuffer(
	Statue.GetIndexBuffer().Get(),
	DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
	0
);

context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

context->IASetInputLayout(m_inputLayout.Get());

// Attach our vertex shader.
context->VSSetShader(
	m_vertexShaderM.Get(),
	nullptr,
	0
);

// Send the constant buffer to the graphics device.
context->VSSetConstantBuffers1(
	0,
	1,
	m_constantBufferM.GetAddressOf(),
	nullptr,
	nullptr
);
// Send the constant buffer to the graphics device.
context->GSSetConstantBuffers1(
	0,
	1,
	m_constantBufferVP.GetAddressOf(),
	nullptr,
	nullptr
);

context->GSSetConstantBuffers1(
	1,
	1,
	m_lightBufferVP.GetAddressOf(),
	nullptr,
	nullptr
);
// Attach our pixel shader.
context->GSSetShader(
	m_geometryShaderVP.Get(),
	nullptr,
	0
);

// Attach our pixel shader.
context->PSSetShader(
	m_pixelShaderN.Get(),
	nullptr,
	0
);

// Draw the objects.
context->DrawIndexed(
	Statue.GetIndices().size(),
	0,
	0
);

		}

	}


	////Snow Particles
	//context->CSSetShader(
	//	m_computeShaderSnow.Get(),
	//	nullptr,
	//	0
	//);

	//context->CSSetUnorderedAccessViews(
	//	0,
	//	1,
	//	snowUAV.GetAddressOf(),
	//	0
	//);

	//context->Dispatch(800, 1, 1);




	//context->GSSetConstantBuffers(
	//	0,
	//	1,
	//	m_constantBufferVP.GetAddressOf()
	//);

	//context->GSSetConstantBuffers(
	//	1,
	//	1,
	//	m_camBuffer.GetAddressOf()
	//);

	//context->PSSetConstantBuffers(
	//	0,
	//	1,
	//	m_lightBuffer.GetAddressOf()
	//);

	//context->PSSetConstantBuffers(
	//	1,
	//	1,
	//	m_camBuffer.GetAddressOf()
	//);

	//context->PSSetShaderResources(
	//	0,
	//	1,
	//	snowTexture.GetAddressOf()
	//);

	//context->PSSetSamplers(
	//	0,
	//	1,
	//	sampler.GetAddressOf()
	//);

	//context->IASetInputLayout(m_inputLayoutParticle.Get());
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	//context->GSSetShaderResources(0, 1, CSISRV.GetAddressOf());
	//context->PSSetShaderResources(0, 1, snowTexture.GetAddressOf());

	//context->VSSetShader(
	//	m_vertexShaderSnow.Get(),
	//	0,
	//	0
	//);
	//context->GSSetShader(
	//	m_geometryShaderSnow.Get(),
	//	0,
	//	0
	//);
	//context->PSSetShader(
	//	m_pixelShaderSnow.Get(),
	//	0,
	//	0
	//);


	//context->Draw(800, 0);





	//After Effects
	m_screenViewport[0] = m_RTTSceneViewport;
	context->RSSetViewports(1, m_screenViewport);

	ID3D11RenderTargetView *const targetss[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targetss, m_deviceResources->GetDepthStencilView());
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::White);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	{

		model = XMMatrixIdentity();
		XMStoreFloat4x4(&m_constantBufferDataM.model, XMMatrixTranspose(model));

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource1(
			m_constantBufferM.Get(),
			0,
			NULL,
			&m_constantBufferDataM,
			0,
			0,
			0
		);

		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource1(
			m_screenEffectBuffer.Get(),
			0,
			NULL,
			&m_screenEffectData,
			0,
			0,
			0
		);

		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers1(
			0,
			1,
			m_constantBufferM.GetAddressOf(),
			nullptr,
			nullptr
		);

		context->PSSetConstantBuffers1(
			0,
			1,
			m_screenEffectBuffer.GetAddressOf(),
			nullptr,
			nullptr
		);

		// Each vertex is one instance of the VertexPositionColor struct.
		context->IASetVertexBuffers(
			0,
			1,
			m_vertexFloor.GetAddressOf(),
			&stride,
			&offset
		);

		context->IASetIndexBuffer(
			m_indexFloor.Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
		);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		context->IASetInputLayout(m_inputLayout.Get());

		context->PSSetShaderResources(0, 1, RTTSRVScene.GetAddressOf());

		// Attach our vertex shader.
		context->VSSetShader(
			m_vertexShaderM.Get(),
			nullptr,
			0
		);

		context->GSSetShader(
			m_GSScreenQuad.Get(),
			nullptr,
			0
		);

		// Attach our pixel shader.
		context->PSSetShader(
			m_PSScreenEffect.Get(),
			nullptr,
			0
		);

		// Draw the objects.
		context->DrawIndexed(
			1,
			0,
			0
		);

	}


}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadVSMTask = DX::ReadDataAsync(L"VertexShaderM.cso");
	auto loadVSSkyTask = DX::ReadDataAsync(L"VSSkybox.cso");
	auto loadVSSnowTask = DX::ReadDataAsync(L"VSSnow.cso");
	auto loadVSInstanceTask = DX::ReadDataAsync(L"VSInstanced.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");
	auto loadGSVPTask = DX::ReadDataAsync(L"GeometryShaderVP.cso");
	auto loadGSSkyTask = DX::ReadDataAsync(L"GSSkybox.cso");
	auto loadGSSnowTask = DX::ReadDataAsync(L"GSSnow.cso");
	auto loadGSScreenQuadTask = DX::ReadDataAsync(L"GSScreenQuad.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadPSNTask = DX::ReadDataAsync(L"PSNormalMap.cso");
	auto loadPSNSTask = DX::ReadDataAsync(L"PSNormalSpecular.cso");
	auto loadPSSkyTask = DX::ReadDataAsync(L"PSSkybox.cso");
	auto loadPSShadowTask = DX::ReadDataAsync(L"PSShadow.cso");
	auto loadPSSnowTask = DX::ReadDataAsync(L"PSSnow.cso");
	auto loadPSScreenEffectTask = DX::ReadDataAsync(L"PSScreenEffect.cso");
	auto loadCSSnowTask = DX::ReadDataAsync(L"CSSnow.cso");


	shadowMap = new ShadowMap(m_deviceResources->GetD3DDevice(), 2048, 2048);

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

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION",0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

	auto createVSMTask = loadVSMTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShaderM
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBufferM
			)
		);
	});

	auto createVSSkyTask = loadVSSkyTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShaderSky
			)
		);

		//static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		//{
		//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//	{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//};

		//DX::ThrowIfFailed(
		//	m_deviceResources->GetD3DDevice()->CreateInputLayout(
		//		vertexDesc,
		//		ARRAYSIZE(vertexDesc),
		//		&fileData[0],
		//		fileData.size(),
		//		&m_inputLayout
		//	)
		//);
	});

	auto createVSSnowTask = loadVSSnowTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShaderSnow
			)
		);


		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TYPE", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayoutParticle
			)
		);
	});

	auto createVSInstanceTask = loadVSInstanceTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShaderInstanced
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(MInstancedConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantInstanceBuffer
			)
		);


	});

	auto createGSTask = loadGSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_geometryShader
			)
		);
	});

	auto createGSVPTask = loadGSVPTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_geometryShaderVP
			)
		);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBufferVP
			)
		);

		CD3D11_BUFFER_DESC lightBufferDesc(sizeof(ViewProjectionLightBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&lightBufferDesc,
				nullptr,
				&m_lightBufferVP
			)
		);
	});

	auto createGSSkyTask = loadGSSkyTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_geometryShaderSky
			)
		);
	});

	auto createGSSnowTask = loadGSSnowTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_geometryShaderSnow
			)
		);
	});

	auto createGSScreenQuadTask = loadGSScreenQuadTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateGeometryShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_GSScreenQuad.GetAddressOf()
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

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
			)
		);

		CD3D11_BUFFER_DESC lightBufferDesc(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&lightBufferDesc,
				nullptr,
				&m_lightBuffer
			)
		);

		CD3D11_BUFFER_DESC screenEffectBufferDesc(sizeof(ScreenEffect), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&screenEffectBufferDesc,
				nullptr,
				m_screenEffectBuffer.GetAddressOf()
			)
		);
	});

	auto createPSNTask = loadPSNTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShaderN
			)
		);
	});

	auto createPSNSTask = loadPSNSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShaderNS
			)
		);

		CD3D11_BUFFER_DESC camBufferDesc(sizeof(SpecularBufferCam), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&camBufferDesc,
				nullptr,
				&m_camBuffer
			)
		);
	});

	auto createPSSkyTask = loadPSSkyTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShaderSky
			)
		);

		CD3D11_RASTERIZER_DESC rasterDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
		rasterDesc.FrontCounterClockwise = true;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, m_rasterizerStateCCW.GetAddressOf())
		);

		rasterDesc.FrontCounterClockwise = false;

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterDesc, m_rasterizerStateCW.GetAddressOf())
		);
	});

	auto createPSSnowTask = loadPSSnowTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShaderSnow
			)
		);
	});

	auto createPSScreenEffectTask = loadPSScreenEffectTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_PSScreenEffect.GetAddressOf()
			)
		);
	});

	auto createPSShadowTask = loadPSShadowTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_pixelShaderShadow.GetAddressOf()
			)
		);
	});

	auto createCSSnowTask = loadCSSnowTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				m_computeShaderSnow.GetAddressOf()
			)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto CreateShaders = (createPSTask && createGSScreenQuadTask && createCSSnowTask && createPSSnowTask && createGSSnowTask && createVSSnowTask && createPSShadowTask && createVSTask && createVSInstanceTask && createPSNTask && createPSNSTask && createPSSkyTask && createVSSkyTask && createGSTask && createGSSkyTask);

	auto createCubeTask = CreateShaders.then([this]() {

		// Load mesh vertices. Each vertex has a position and a color.
		static const RobustVertex cubeVertices[] =
		{
			//LEFT
			{XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f), XMFLOAT3(-1.0f,0.0f,0.0f)},
			{XMFLOAT3(-1.0f, -1.0f,  1.0f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(-1.0f,0.0f,0.0f)},
			{XMFLOAT3(-1.0f,  1.0f, -1.0f),XMFLOAT2(1.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(-1.0f,0.0f,0.0f)},
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(-1.0f,0.0f,0.0f) },

			//BACK
			{ XMFLOAT3(-1.0f, -1.0f,  1.0f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(1.0f, -1.0f,  1.0f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(1.0f,  1.0f,  1.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,1.0f) },

			//RIGHT
			{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT2(1.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(1.0f,0.0f,0.0f) },

			//FRONT
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(-1.0f,  1.0f, -1.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },

			//TOP
			{ XMFLOAT3(-1.0f,  1.0f, -1.0f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(-1.0f,  1.0f,  1.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(1.0f,  1.0f, -1.0f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(1.0f,  1.0f, 1.0f), XMFLOAT2(1.0f,0.0f) ,XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f)},


			//BOTTOM
			{ XMFLOAT3(1.0f, -1.0f,  1.0f),XMFLOAT2(1.0f,1.0f) ,XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(-1.0f, -1.0f,  1.0f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(1.0f, -1.0f, -1.0f),XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f) ,XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f) },
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
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
		static const unsigned short cubeIndices[] =
		{
			0,1,2,
			1,3,2,

			4,5,6,
			5,7,6,

			8,9,10,
			9,11,10,

			12,13,14,
			13,15,14,

			16,17,18,
			17,19,18,

			20,21,22,
			21,23,22,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
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

		HRESULT result;

		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"paint.dds",
			NULL, &cubeSRV);
		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Box_Wood02Dark_N.dds",
			NULL, cubeNSRV.GetAddressOf());
	});

	auto createFloorTask = CreateShaders.then([this]()
	{

		RobustVertex floorVertices[] =
		{
			{ XMFLOAT3(0.0f,  0.0f, 0.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,0.0f) },

		};

		unsigned short floorIndices[] =
		{
			0
		};

		m_indexFloorCount = ARRAYSIZE(floorIndices);

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = floorVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc = CD3D11_BUFFER_DESC(sizeof(floorVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexFloor
			)
		);


		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = floorIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc = CD3D11_BUFFER_DESC(sizeof(floorIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexFloor
			)
		);
		HRESULT result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"cobblestoneD.dds",
			NULL, &floorSRV);
		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"cobblestoneN.dds",
			NULL, &floorNSRV);
		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"cobblestoneS.dds",
			NULL, &floorSSRV);
	});

	//TODO: make a loop through all models in a models array, make an array of modefilenames and texturefilenames that is read from a file(After Project Task)
	auto createFountainTask = CreateShaders.then([this]()
	{
		fountain.CreateModel(m_deviceResources, "fountain.obj", L"Fountain_D.dds",L"Fountain_N.dds");
	});

	auto createGrappleGirlTask = CreateShaders.then([this]()
	{
		GrappleGirl.CreateModel(m_deviceResources, "GrappleGirl.obj", L"DIFFUSE_Character.dds", L"NORMAL_Character.dds", L"SPECULAR_Character.dds");
	});

	auto createGunTurretTask = CreateShaders.then([this]()
	{
		GunTurret.CreateModel(m_deviceResources, "GunTurret01.obj", L"T_HeavyTurret_D.dds", L"T_HeavyTurret_N.dds", L"T_HeavyTurret_S.dds");
	});

	auto createStatueTask = CreateShaders.then([this]()
	{
		Statue.CreateModel(m_deviceResources, "statue.obj", L"granite.dds", L"T_HeavyTurret_N.dds");
	});

	auto createKnightTask = CreateShaders.then([this]()
	{
		Knight.CreateModel(m_deviceResources, "knight_sword.obj", L"armor_default_color.dds", L"armor_default_nmap.dds", L"armor_default_rough.dds");
	});

	auto createSkyCubeTask = CreateShaders.then([this]()
	{
		HRESULT result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"DarkSnowy.dds",
			NULL, &SkyCubeSRV);

		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"flatmap.dds",
			NULL, flatNormalMapSRV.GetAddressOf());
	});

	auto createSphereTask = CreateShaders.then([this]()
	{
		Sphere.CreateModel(m_deviceResources, "Sphere.obj", L"paint.dds", L"FieldstoneBump.dds");
	});

	auto createSnowflakesTask = CreateShaders.then([this]()
	{
		//Init snowFlakes data
		for (unsigned int i = 0; i < 800; ++i)
		{
			Particle snowflake;
			snowflake.age = 1;
			snowflake.pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
			snowflake.size = XMFLOAT2(0.0f,0.0f);
			snowflake.type = 0;
			snowflake.velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
			snowflakes.push_back(snowflake);
		}

		CD3D11_BUFFER_DESC constantBufferDesc;
		constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		constantBufferDesc.ByteWidth = sizeof(Particle)* snowflakes.size();
		constantBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		constantBufferDesc.CPUAccessFlags = 0;
		constantBufferDesc.StructureByteStride = sizeof(Particle);
		constantBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &snowflakes[0];

		//Create input structured buffer
		//Maybe not necessary
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				&initData,
				m_structuredBufferI.GetAddressOf()
			)
		);

		//Create RW structured buffer
		constantBufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				m_structuredBufferRW.GetAddressOf()
			)
		);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.Flags = 0;
		srvDesc.BufferEx.NumElements = snowflakes.size();

		HRESULT result = m_deviceResources->GetD3DDevice()->CreateShaderResourceView(m_structuredBufferI.Get(), &srvDesc, CSISRV.GetAddressOf());

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = snowflakes.size();
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;

		result = m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(m_structuredBufferRW.Get(),&uavDesc, snowUAV.GetAddressOf());


		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"snowflake.dds",
			NULL, snowTexture.GetAddressOf());
	});

	auto createWolfTask = CreateShaders.then([this]()
	{
		Wolf.CreateModel(m_deviceResources, "wolf_howl.obj", L"alphaWhiteR.dds", L"alphaN.dds");
	});

	//TODO: SHADOWS, make look better, wonderful grammar Dallas. also, shadow map for cone light, AND, don't shadow map if the light is turned off. cone or directional

	//Texture Filter
	CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;

	m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &sampler);

	samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &samplerFloor);


	samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &samplerPCF);

	// Once the cube is loaded, the object is ready to be rendered.
	(createCubeTask && createSnowflakesTask && createStatueTask && createFountainTask && createGrappleGirlTask && createFloorTask &&createGunTurretTask && createKnightTask && createSkyCubeTask
		&& createSphereTask && createWolfTask).then([this]() {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_vertexShaderM.Reset();
	m_vertexShaderSky.Reset();
	m_vertexShaderSnow.Reset();
	m_vertexShaderInstanced.Reset();
	m_inputLayout.Reset();
	m_inputLayoutParticle.Reset();
	m_geometryShader.Reset();
	m_geometryShaderVP.Reset();
	m_geometryShaderSky.Reset();
	m_geometryShaderSnow.Reset();
	m_pixelShader.Reset();
	m_pixelShaderN.Reset();
	m_pixelShaderNS.Reset();
	m_pixelShaderSky.Reset();
	m_pixelShaderSnow.Reset();
	m_constantBuffer.Reset();
	m_constantBufferM.Reset();
	m_constantBufferVP.Reset();
	m_constantInstanceBuffer.Reset();
	m_lightBuffer.Reset();
	m_lightBufferVP.Reset();
	m_camBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_indexFloor.Reset();
	m_vertexFloor.Reset();
	sampler.Reset();
	samplerFloor.Reset();
	samplerPCF.Reset();
	cubeSRV.Reset();
	floorSRV.Reset();
	floorNSRV.Reset();
	flatNormalMapSRV.Reset();
	SkyCubeSRV.Reset();
	RTTSRV.Reset();
	m_rasterizerStateCW.Reset();
	m_rasterizerStateCCW.Reset();
	m_RTTRenderTargetView.Reset();
	m_RTTDepthStencilView.Reset();

	Goomba.Release();
	fountain.Release();
	GunTurret.Release();
	Sphere.Release();
	cube.Release();

	delete shadowMap;
}

Light CreateDirectionalLight(XMFLOAT4 direction, XMFLOAT4 color)
{
	Light tempLight;
	tempLight.color = color;
	tempLight.normal = direction;
	tempLight.ratio = XMFLOAT4(0.0f, 0.0f, 300.0f, 1);
	tempLight.position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

	tempLight.position.x = ((tempLight.ratio.z - NEAR_PLANE)*0.5f) - (tempLight.normal.x * 30 *0.5f);
	tempLight.position.y = ((tempLight.ratio.z - NEAR_PLANE)*0.5f) - (tempLight.normal.y * 30 *0.5f);
	tempLight.position.z = ((tempLight.ratio.z - NEAR_PLANE)*0.5f) - (tempLight.normal.z * 30 *0.5f);
	//tempLight.position.w = 1;

	return tempLight;
}

Light CreatePointLight(XMFLOAT4 position, XMFLOAT4 color, float radius)
{
	Light tempLight = {};
	tempLight.position = position;
	tempLight.position.w = 1;
	tempLight.color = color;
	tempLight.ratio = XMFLOAT4(0.0f, 0.0f, radius, 1);
	tempLight.normal = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	return tempLight;
}

Light CreateSpotLight(XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 color, float coneRatio,float radius)
{
	Light tempLight = {};
	tempLight.position = position;
	tempLight.position.w = 2;
	tempLight.color = color;
	tempLight.ratio = XMFLOAT4(coneRatio*0.95f, coneRatio, radius, 1);
	tempLight.normal = direction;
	return tempLight;
}

void Sample3DSceneRenderer::InitializeLights()
{
	//m_lightBufferData.lights[0] = CreateDirectionalLight(XMFLOAT4(-0.1, -0.1, -0.9, 0.0f), (XMFLOAT4)Colors::White);
	//Directionals
	m_lightBufferData.lights[0] = CreateDirectionalLight(XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f), (XMFLOAT4)Colors::Silver);


	//Points
	m_lightBufferData.lights[1] = CreatePointLight(XMFLOAT4(-10.0f, 0.5f, -10.0f, 0.0f), (XMFLOAT4)Colors::Wheat, 5.0f);

	//Spots
	m_lightBufferData.lights[2] = CreateSpotLight(XMFLOAT4(-0.75f, 0.0f, 0.0f, 0.0f), XMFLOAT4(1, 0, 0, 0.0f), (XMFLOAT4)Colors::DeepSkyBlue, .90,10);
	m_lightBufferData.lights[3] = CreateSpotLight(XMFLOAT4(-2.0f, 5.0f, 2.0f, 0.0f), XMFLOAT4(0.5f, -1, -0.5f, 0.0f), (XMFLOAT4)Colors::Yellow, .99,50);
	m_lightBufferData.lights[4] = CreateSpotLight(XMFLOAT4(-2.0f, 5.0f, -2.0f, 0.0f), XMFLOAT4(0.5f, -1, 0.5f, 0.0f), (XMFLOAT4)Colors::Cyan, .99,50);
	m_lightBufferData.lights[5] = CreateSpotLight(XMFLOAT4(2.0f, 5.0f, 2.0f, 0.0f), XMFLOAT4(-0.5f, -1, -0.5f, 0.0f), (XMFLOAT4)Colors::Lime, .99,50);
	m_lightBufferData.lights[6] = CreateSpotLight(XMFLOAT4(2.0f, 5.0f, -2.0f, 0.0f), XMFLOAT4(-0.5f, -1, 0.5f, 0.0f), (XMFLOAT4)Colors::Fuchsia, .99,50);
}

void Sample3DSceneRenderer::InitializeScreenEffect()
{
	m_screenEffectData.lossColor.x = 0.0f;
	m_screenEffectData.lossColor.y = 0.0f;
	m_screenEffectData.lossColor.z = 0.0f;
	m_screenEffectData.lossColor.w = 0.0f;
}

std::vector<RobustVertex> LoadVerticestoVector(RobustVertex* _vertices,unsigned int _size)
{
	std::vector<RobustVertex> vertices;

	for (unsigned int i = 0; i < _size; ++i)
	{
		RobustVertex tempVert = _vertices[i];
		vertices.push_back(tempVert);
	}
	return vertices;
}

std::vector<unsigned short> LoadIndicestoVector(unsigned short* _indices, unsigned int _size)
{
	std::vector<unsigned short> indices;

	for (unsigned int i = 0; i < _size; ++i)
	{
		unsigned short tempIndex = _indices[i];
		indices.push_back(tempIndex);
	}
	return indices;
}

void CalculateTangents(std::vector<RobustVertex>& _vertices,std::vector<unsigned short>& _indices)
{
	for (unsigned int i = 0; i < _indices.size(); i += 3)
	{
		//TODO:: Average Tangents
		XMFLOAT3 tempVert1 = _vertices[_indices[i]].pos;
		XMFLOAT3 tempVert2 = _vertices[_indices[i + 1]].pos;
		XMFLOAT3 tempVert3 = _vertices[_indices[i + 2]].pos;
		XMVECTOR vertEdge1 = XMVectorSubtract(XMLoadFloat3(&tempVert2), XMLoadFloat3(&tempVert1));
		XMVECTOR vertEdge2 = XMVectorSubtract(XMLoadFloat3(&tempVert3), XMLoadFloat3(&tempVert1));
		XMFLOAT3 vertEdges1;
		XMFLOAT3 vertEdges2;
		XMStoreFloat3(&vertEdges1, vertEdge1);
		XMStoreFloat3(&vertEdges2, vertEdge2);


		XMFLOAT2 texCoord1 = _vertices[_indices[i]].texCoords;
		XMFLOAT2 texCoord2 = _vertices[_indices[i + 1]].texCoords;
		XMFLOAT2 texCoord3 = _vertices[_indices[i + 2]].texCoords;

		XMVECTOR texEdge1 = XMVectorSubtract(XMLoadFloat2(&texCoord2), XMLoadFloat2(&texCoord1));
		XMVECTOR texEdge2 = XMVectorSubtract(XMLoadFloat2(&texCoord3), XMLoadFloat2(&texCoord1));
		XMFLOAT2 texEdges1;
		XMFLOAT2 texEdges2;
		XMStoreFloat2(&texEdges1, texEdge1);
		XMStoreFloat2(&texEdges2, texEdge2);

		float ratio = 1.0f / (texEdges1.x * texEdges2.y - texEdges2.x * texEdges1.y);

		XMFLOAT3 uDirection = XMFLOAT3((texEdges2.y * vertEdges1.x - texEdges1.y * vertEdges2.x) * ratio,
			(texEdges2.y * vertEdges1.y - texEdges1.y * vertEdges2.y) * ratio,
			(texEdges2.y * vertEdges1.z - texEdges1.y * vertEdges2.z) * ratio);
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
			XMStoreFloat3(&dotResult, XMVector3Dot(XMLoadFloat3(&_vertices[_indices[i + j]].normal), uDirec));
			XMVECTOR tangent;
			XMFLOAT4 prevTangent = _vertices[_indices[i + j]].tangent;
			tangent = uDirec - XMLoadFloat3(&_vertices[_indices[i + j]].normal) * dotResult.x;
			tangent = XMVector3Normalize(tangent);
			XMStoreFloat4(&_vertices[_indices[i + j]].tangent, tangent);


			XMVECTOR cross = XMVector3Cross(XMLoadFloat3(&_vertices[_indices[i + j]].normal), uDirec);
			XMVECTOR handedness = vDirec;
			XMStoreFloat3(&dotResult, XMVector3Dot(cross, handedness));
			_vertices[_indices[i + j]].tangent.w = (dotResult.x < 0.0f) ? -1.0f : 1.0f;


			//if (_indices[i + j] != i + j)
			//{
			//	break;
			//}

			//if (prevTangent.x != 0 && prevTangent.y != 0 && prevTangent.z != 0)
			//{
			//	prevTangent.x = (prevTangent.x + _vertices[_indices[i + j]].tangent.x)*0.5f;
			//	prevTangent.y = (prevTangent.y + _vertices[_indices[i + j]].tangent.y)*0.5f;
			//	prevTangent.z = (prevTangent.z + _vertices[_indices[i + j]].tangent.z)*0.5f;
			//}
		}
	}
}
