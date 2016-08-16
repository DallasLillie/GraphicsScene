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

	static XMVECTORF32 eye = { 0.0f, 3.0f, -5.0f, 1.0f };
	static XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 1.0f };
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
		100.0f
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

	//KeyboardInput
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





	XMMATRIX pointLight = XMMatrixIdentity();
	XMVECTOR lightpos = XMLoadFloat4(&m_lightBufferData.lights[1].position);

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
	cubeWorld = XMMatrixMultiply(XMMatrixScaling(0.51f, 0.51f, 0.51f), cubeWorld);


	//GunMatrix
	XMMATRIX model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.0f, 1.0f, 0.0f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(135)), model);
	model = XMMatrixMultiply(XMMatrixScaling(0.2f, 0.2f, 0.2f), model);
	model = XMMatrixMultiply(model, cubeWorld);
	XMStoreFloat4x4(&GunTurret.GetWorldMatrix(), XMMatrixTranspose(model));


	//cubeWorld = XMMatrixTranspose(cubeWorld);

	//PyramidMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(5.5f, 1.5f, 5.5f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(-radians), model);
	model = XMMatrixMultiply(model, XMMatrixRotationY(radians));
	XMStoreFloat4x4(&pyramid.GetWorldMatrix(), XMMatrixTranspose(model));


	//GoombaMatrix
	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.25f, 0.0f), model);
	model = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(-135)), model);
	model = XMMatrixMultiply(XMMatrixRotationY(radians), model);

	model = XMMatrixMultiply(model, XMMatrixTranspose(XMLoadFloat4x4(&pyramid.GetWorldMatrix())));
	XMStoreFloat4x4(&Goomba.GetWorldMatrix(), XMMatrixTranspose(model));


	//SphereMatrix
	for (unsigned int i = 0; i < 10; ++i)
	{
		model = XMMatrixIdentity();
		model = XMMatrixMultiply(XMMatrixRotationY(radians*(10-i)), model);
		model = XMMatrixMultiply(XMMatrixTranslation(0.0f + (i * 2), 5.0f, 0.0f +(i * 2)), model);
		model = XMMatrixMultiply(XMMatrixRotationY(radians*(i + 1)), model);
		model = XMMatrixMultiply(XMMatrixScaling(0.4f+(i*0.1), 0.4f + (i*0.1), 0.4f + (i*0.1)), model);
		XMStoreFloat4x4(&Sphere.GetWorldMatrix(), XMMatrixTranspose(model));


		m_constantInstanceData.model[i] = Sphere.GetWorldMatrix();

	}


	//HEADLamp Calculations
	XMMATRIX headLamp = XMMatrixIdentity();
	headLamp = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.25f, 0.0f), headLamp);
	headLamp = XMMatrixMultiply(XMMatrixRotationX(XMConvertToRadians(10)), headLamp);
	headLamp = XMMatrixMultiply(headLamp, newcamera);

	XMStoreFloat4(&m_lightBufferData.lights[2].position, headLamp.r[3]);
	XMStoreFloat4(&m_lightBufferData.lights[2].normal, headLamp.r[2]);
	m_lightBufferData.lights[2].position.w = 2;



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
		directionalLight = XMMatrixRotationAxis(XMVECTOR(XMLoadFloat3(&XMFLOAT3(-1.0f,0.0f,1.0f))),XMConvertToRadians(0.5));
	}
	if (buttons['K'])
	{
		directionalLight = XMMatrixRotationAxis(XMVECTOR(XMLoadFloat3(&XMFLOAT3(-1.0f, 0.0f, 1.0f))), XMConvertToRadians(-0.5));
	}
	//directionalLight = XMMatrixMultiply(XMMatrixRotationY(XMConvertToRadians(1)), directionalLight);
	directionalLight = XMMatrixTranspose(directionalLight);
	lightNorm = XMVector4Transform(lightNorm, directionalLight);
	XMStoreFloat4(&m_lightBufferData.lights[0].normal, lightNorm);

	//TODO: both viewports
	m_specBufferCamData.cameraPosition.x = camera._41;
	m_specBufferCamData.cameraPosition.y = camera._42;
	m_specBufferCamData.cameraPosition.z = camera._43;
	m_specBufferCamData.cameraPosition.w = camera._44;

	/*Be sure to inverse the camera & Transpose because they don't use pragma pack row major in shaders*/
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newcamera)));

	m_constantBufferDataVP.view[0] = m_constantBufferData.view;
	m_constantBufferDataVP.view[1] = m_constantBufferData.view;

	//Maybenotdothis every frame? just a size right? does it ever get updated outside of this?
	Size	outputSize;
	outputSize = m_deviceResources->GetOutputSize();
	for (unsigned int i = 0; i < NUM_LIGHTS; ++i)
	{
		m_lightBufferDataVP.UpdateView(m_lightBufferData.lights[i], outputSize.Width, outputSize.Height);
		m_lightBufferDataVP.UpdateProjection(m_lightBufferData.lights[i], outputSize.Width, outputSize.Height);
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


		// Draw the objects.
		context->DrawIndexed(
			GunTurret.GetIndices().size(),
			0,
			0
		);

	}

	//First Pass Shadows




























	//Second Pass
	//TODO::better way to to do this
	D3D11_VIEWPORT m_screenViewport[2];
	m_screenViewport[0] = m_deviceResources->GetScreenViewport();
	m_screenViewport[1] = m_deviceResources->GetScreenViewport2();
	context->RSSetViewports(2, m_screenViewport);

	ID3D11RenderTargetView *const target[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, target, m_deviceResources->GetDepthStencilView());
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::White);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	
	//RESET GS
	ID3D11GeometryShader *tempGS = nullptr;
	context->GSSetShader(tempGS, nullptr, 0);

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


	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
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
		context->PSSetShaderResources(1, 1, floorNSRV.GetAddressOf());

		context->PSSetSamplers(0, 1, sampler.GetAddressOf());

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





	//DRAWPYRAMID
	{
		m_constantBufferDataM.model = pyramid.GetWorldMatrix();

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
	pyramid.GetVertexBuffer().GetAddressOf(),
	&stride,
	&offset
);

context->IASetIndexBuffer(
	pyramid.GetIndexBuffer().Get(),
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


context->PSSetShaderResources(0, 1, pyramid.GetSRV().GetAddressOf());
context->PSSetShaderResources(1, 1, floorNSRV.GetAddressOf());

context->PSSetSamplers(0, 1, sampler.GetAddressOf());

// Draw the objects.
context->DrawIndexed(
	pyramid.GetIndices().size(),
	0,
	0
);

	}


	//Draw Goomba
	{
		m_constantBufferDataM.model = Goomba.GetWorldMatrix();

		context->UpdateSubresource1(
			m_constantBufferM.Get(),
			0,
			NULL,
			&m_constantBufferDataM,
			0,
			0,
			0
		);

		context->PSSetShaderResources(0, 1, Goomba.GetSRV().GetAddressOf());
		context->PSSetShaderResources(1, 1, Goomba.GetNormalMap().GetAddressOf());

		stride = sizeof(RobustVertex);
		offset = 0;
		context->IASetVertexBuffers(
			0,
			1,
			Goomba.GetVertexBuffer().GetAddressOf(),
			&stride,
			&offset
		);

		context->IASetIndexBuffer(
			Goomba.GetIndexBuffer().Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
		);
		context->PSSetSamplers(0, 1, samplerFloor.GetAddressOf());
		// Draw the objects.
		context->DrawIndexed(
			Goomba.GetIndices().size(),
			0,
			0
		);
	}


	//Draw Spheres
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

		context->PSSetShaderResources(0, 1, Sphere.GetSRV().GetAddressOf());
		context->PSSetShaderResources(1, 1, Sphere.GetNormalMap().GetAddressOf());

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
		context->DrawIndexedInstanced(
			Sphere.GetIndices().size(),
			10,
			0,
			0,
			0
		);

	}

	//Draw Gunturret
	{
		m_constantBufferDataM.model = GunTurret.GetWorldMatrix();

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


		// Draw the objects.
		context->DrawIndexed(
			GunTurret.GetIndices().size(),
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
	auto loadVSInstanceTask = DX::ReadDataAsync(L"VSInstanced.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");
	auto loadGSVPTask = DX::ReadDataAsync(L"GeometryShaderVP.cso");
	auto loadGSSkyTask = DX::ReadDataAsync(L"GSSkybox.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadPSNTask = DX::ReadDataAsync(L"PSNormalMap.cso");
	auto loadPSNSTask = DX::ReadDataAsync(L"PSNormalSpecular.cso");
	auto loadPSSkyTask = DX::ReadDataAsync(L"PSSkybox.cso");

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

	// Once both shaders are loaded, create the mesh.
	auto CreateShaders = (createPSTask && createVSTask && createVSInstanceTask && createPSNTask && createPSNSTask && createPSSkyTask && createVSSkyTask && createGSTask && createGSSkyTask);

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

		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Box_Wood02Dark.dds",
			NULL, &cubeSRV);
		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Box_Wood02Dark_N.dds",
			NULL, cubeNSRV.GetAddressOf());
	});

	auto createFloorTask = CreateShaders.then([this]()
	{
		//RobustVertex floorVertices[] =
		//{
		//	{ XMFLOAT3(-5.0f,  0.0f, -5.0f),XMFLOAT2(0.0f,10.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
		//	{ XMFLOAT3(-5.0f,  0.0f,  5.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
		//	{ XMFLOAT3(5.0f,  0.0f, -5.0f),XMFLOAT2(10.0f,10.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
		//	{ XMFLOAT3(5.0f,  0.0f, 5.0f), XMFLOAT2(10.0f,0.0f) ,XMFLOAT4(1.0f,1.0f,1.0f,1.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
		//};

		//unsigned short floorIndices[] =
		//{
		//	0,1,2,
		//	1,3,2
		//};

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
		HRESULT result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Fieldstone.dds",
			NULL, &floorSRV);
		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"FieldstoneBump.dds",
			NULL, &floorNSRV);
	});

	//TODO: make a loop through all models in a models array, make an array of modefilenames and texturefilenames that is read from a file(After Project Task)
	auto createPyramidTask = CreateShaders.then([this]()
	{
		pyramid.CreateModel(m_deviceResources, "test pyramid.obj", L"Box_Wood02Dark.dds");
	});

	auto createGoombaTask = CreateShaders.then([this]()
	{
		Goomba.CreateModel(m_deviceResources, "Goomba.obj", L"Diffuse_Fuzzy.dds", L"Normal_Fuzzy.dds");
	});

	auto createGunTurretTask = CreateShaders.then([this]()
	{
		GunTurret.CreateModel(m_deviceResources, "GunTurret01.obj", L"T_HeavyTurret_D.dds", L"T_HeavyTurret_N.dds", L"T_HeavyTurret_S.dds");
	});

	auto createSkyCubeTask = CreateShaders.then([this]()
	{
		HRESULT result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"DarkSnowy.dds",
			NULL, &SkyCubeSRV);
	});

	auto createSphereTask = CreateShaders.then([this]()
	{
		Sphere.CreateModel(m_deviceResources, "Sphere.obj", L"Fieldstone.dds", L"FieldstoneBump.dds");
	});

	//auto createWolfTask = CreateShaders.then([this]()
	//{
	//	Wolf.CreateModel(m_deviceResources, "Wolf.obj", L"Wolf.dds", L"WolfN.dds");
	//});

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

	// Once the cube is loaded, the object is ready to be rendered.
	(createCubeTask && createPyramidTask && createGoombaTask && createFloorTask &&createGunTurretTask && createSkyCubeTask && createSphereTask).then([this]() {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_vertexShaderM.Reset();
	m_vertexShaderSky.Reset();
	m_vertexShaderInstanced.Reset();
	m_inputLayout.Reset();
	m_geometryShader.Reset();
	m_geometryShaderVP.Reset();
	m_geometryShaderSky.Reset();
	m_pixelShader.Reset();
	m_pixelShaderN.Reset();
	m_pixelShaderNS.Reset();
	m_pixelShaderSky.Reset();
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
	pyramid.Release();
	GunTurret.Release();
	Sphere.Release();
	cube.Release();
}

Light CreateDirectionalLight(XMFLOAT4 direction, XMFLOAT4 color)
{
	Light tempLight;
	tempLight.color = color;
	tempLight.normal = direction;
	tempLight.ratio = XMFLOAT4(0.0f, 0.0f, 0.0f, 1);
	tempLight.position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
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

Light CreateSpotLight(XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 color, float coneRatio)
{
	Light tempLight = {};
	tempLight.position = position;
	tempLight.position.w = 2;
	tempLight.color = color;
	tempLight.ratio = XMFLOAT4(coneRatio*0.95f, coneRatio, 10, 1);
	tempLight.normal = direction;
	return tempLight;
}

void Sample3DSceneRenderer::InitializeLights()
{
	//m_lightBufferData.lights[0] = CreateDirectionalLight(XMFLOAT4(-0.1, -0.1, -0.9, 0.0f), (XMFLOAT4)Colors::White);
	m_lightBufferData.lights[0] = CreateDirectionalLight(XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f), (XMFLOAT4)Colors::Silver);
	m_lightBufferData.lights[1] = CreatePointLight(XMFLOAT4(0.0f, 0.75f, -1.0f, 0.0f), (XMFLOAT4)Colors::OrangeRed, 5.0f);
	m_lightBufferData.lights[2] = CreateSpotLight(XMFLOAT4(-0.75f, 0.0f, 0.0f, 0.0f), XMFLOAT4(1, 0, 0, 0.0f), (XMFLOAT4)Colors::Chartreuse, .90);
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
