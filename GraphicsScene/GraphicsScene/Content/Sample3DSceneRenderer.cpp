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
	InitializeLights();

	//XMStoreFloat4x4(&view, XMMatrixIdentity());

	static const XMVECTORF32 eye = { 0.0f, 1.0f, -5.0f, 1.0f };
	static const XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 1.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMStoreFloat4x4(&camera, XMMatrixInverse(0, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
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
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix/* * orientationMatrix*/)
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
		Translate(0.0f, 0.5f, 0.0f);
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

	//RightButton
	if (buttons[2])
	{
		if (m_lightBufferData.lights[2].color.x != 1)
		{
			m_lightBufferData.lights[2].color = XMFLOAT4(1, 1, 1, 1);
		}
		else
		{
			m_lightBufferData.lights[2].color = XMFLOAT4(0, 0, 0, 0);
		}

		buttons[2] = false;
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
			newcamera = XMMatrixRotationX(diffy*0.01f) * newcamera * XMMatrixRotationY(diffx*0.01f);
			newcamera.r[3] = pos;
		}
	}

	XMStoreFloat4x4(&camera, newcamera);

	//HEADLamp Calculations
	XMMATRIX headLamp = XMMatrixIdentity();
	headLamp = XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.5f, 0.0f),headLamp);
	headLamp = XMMatrixMultiply(XMMatrixRotationX(XMConvertToRadians(10)), headLamp);
	headLamp = XMMatrixMultiply(headLamp, newcamera);

	XMStoreFloat4(&m_lightBufferData.lights[2].position, headLamp.r[3]);
	XMStoreFloat4(&m_lightBufferData.lights[2].normal, headLamp.r[2]);
	m_lightBufferData.lights[2].position.w = 2;


	/*Be sure to inverse the camera & Transpose because they don't use pragma pack row major in shaders*/
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(0, newcamera)));

	mouse_move = false;/*Reset*/
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	//TODO: mirror translate function
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::Translate(float xOffset,float yOffset, float zOffset)
{
	// Prepare to pass the updated model matrix to the shader
	XMMATRIX model = XMLoadFloat4x4(&m_constantBufferData.model);
	model = XMMatrixMultiply(XMMatrixTranslation(xOffset, yOffset, zOffset),model);
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(model));
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
	UINT stride = sizeof(RobustVertex);
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

	context->PSSetConstantBuffers1(
		0,
		1,
		m_lightBuffer.GetAddressOf(),
		nullptr,
		nullptr
	);


	//ID3D11ShaderResourceView *SRV[]{ cubeSRV };
	ID3D11ShaderResourceView *SRV[]{ cubeSRV,floorSRV,goombaSRV };
	context->PSSetShaderResources(0, 1, &SRV[0]);

	context->PSSetSamplers(0, 1, &sampler);

	// Draw the objects.
	context->DrawIndexed(
		m_indexCount,
		0,
		0
		);








	XMMATRIX model = XMMatrixIdentity();
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(model));

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

	//ID3D11ShaderResourceView *SRV2[]{ floorSRV };
	context->PSSetShaderResources(0, 1, &SRV[1]);

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

	context->PSSetSamplers(0, 1, &samplerFloor);


	// Draw the objects.
	context->DrawIndexed(
		m_indexFloorCount,
		0,
		0
	);




	model = XMMatrixIdentity();
	model = XMMatrixMultiply(XMMatrixTranslation(1.0f, 1.0f, 1.0f), model);
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(model));

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

	//ID3D11ShaderResourceView *SRV2[]{ floorSRV };
	context->PSSetShaderResources(0, 1, &SRV[0]);

	stride = sizeof(RobustVertex);
	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		pyramid.m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		pyramid.m_indexBuffer.Get(),
		DXGI_FORMAT_R32_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->PSSetSamplers(0, 1, &samplerFloor);


	// Draw the objects.
	context->DrawIndexed(
		pyramid.m_indexCount,
		0,
		0
	);





	context->PSSetShaderResources(0, 1, &SRV[2]);

	stride = sizeof(RobustVertex);
	offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		Goomba.m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
	);

	context->IASetIndexBuffer(
		Goomba.m_indexBuffer.Get(),
		DXGI_FORMAT_R32_UINT, // Each index is one 16-bit unsigned integer (short).
		0
	);

	context->PSSetSamplers(0, 1, &samplerFloor);


	// Draw the objects.
	context->DrawIndexed(
		Goomba.m_indexCount,
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

		CD3D11_BUFFER_DESC lightBufferDesc(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&lightBufferDesc,
				nullptr,
				&m_lightBuffer
			)
		);
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this] () {

		// Load mesh vertices. Each vertex has a position and a color.
		static const RobustVertex cubeVertices[] = 
		{
			//LEFT
			{XMFLOAT3(-0.5f, -0.5f, -0.5f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f), XMFLOAT3(-1.0f,0.0f,0.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(-1.0f,0.0f,0.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f),XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(-1.0f,0.0f,0.0f)},
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(-1.0f,0.0f,0.0f) },

			//BACK
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,1.0f) },

			//RIGHT
			{ XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT2(1.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(1.0f,0.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(1.0f,0.0f,0.0f) },

			//FRONT
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT2(1.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,0.0f,-1.0f) },

			//TOP
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f),XMFLOAT2(1.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			{ XMFLOAT3(0.5f,  0.5f, 0.5f), XMFLOAT2(1.0f,0.0f) ,XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f)},


			//BOTTOM
			{ XMFLOAT3(0.5f, -0.5f,  0.5f),XMFLOAT2(1.0f,1.0f) ,XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f),XMFLOAT2(0.0f,1.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(0.5f, -0.5f, -0.5f),XMFLOAT2(1.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f)},
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f) ,XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,-1.0f,0.0f) },
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




		//FloorCreation
		{
			RobustVertex floorVertices[] =
			{
				{ XMFLOAT3(-5.0f,  0.0f, -5.0f),XMFLOAT2(0.0f,10.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
				{ XMFLOAT3(-5.0f,  0.0f,  5.0f),XMFLOAT2(0.0f,0.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
				{ XMFLOAT3(5.0f,  0.0f, -5.0f),XMFLOAT2(10.0f,10.0f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
				{ XMFLOAT3(5.0f,  0.0f, 5.0f), XMFLOAT2(10.0f,0.0f) ,XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT3(0.0f,1.0f,0.0f) },
			};




			vertexBufferData = { 0 };
			vertexBufferData.pSysMem = floorVertices;
			vertexBufferData.SysMemPitch = 0;
			vertexBufferData.SysMemSlicePitch = 0;
			vertexBufferDesc = CD3D11_BUFFER_DESC(sizeof(floorVertices), D3D11_BIND_VERTEX_BUFFER);
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&vertexBufferDesc,
					&vertexBufferData,
					&m_vertexFloor
				)
			);

			unsigned short floorIndices[] =
			{
				0,1,2,
				1,3,2
			};

			m_indexFloorCount = ARRAYSIZE(floorIndices);

			indexBufferData = { 0 };
			indexBufferData.pSysMem = floorIndices;
			indexBufferData.SysMemPitch = 0;
			indexBufferData.SysMemSlicePitch = 0;
			indexBufferDesc = CD3D11_BUFFER_DESC(sizeof(floorIndices), D3D11_BIND_INDEX_BUFFER);
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&indexBufferDesc,
					&indexBufferData,
					&m_indexFloor
				)
			);

		}

		HRESULT result;

		//PyramidModel
		{
			pyramid.loadOBJ("test pyramid.obj");
			//pyramid.CreateVertexBuffer();
			//pyramid.CreateIndexBuffer();

			vertexBufferData = { 0 };
			vertexBufferData.pSysMem = pyramid.m_vertices.data();
			vertexBufferData.SysMemPitch = 0;
			vertexBufferData.SysMemSlicePitch = 0;
			vertexBufferDesc = CD3D11_BUFFER_DESC(pyramid.m_vertices.size() * sizeof(RobustVertex), D3D11_BIND_VERTEX_BUFFER);
			result = m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				pyramid.m_vertexBuffer.GetAddressOf()
			);

			pyramid.m_indexCount = pyramid.m_indices.size();

			indexBufferData = { 0 };
			indexBufferData.pSysMem = pyramid.m_indices.data();
			indexBufferData.SysMemPitch = 0;
			indexBufferData.SysMemSlicePitch = 0;
			indexBufferDesc = CD3D11_BUFFER_DESC(pyramid.m_indices.size() * sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
			result =
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&indexBufferDesc,
					&indexBufferData,
					pyramid.m_indexBuffer.GetAddressOf()
				);

		}

		//GoombaModel
		{
			Goomba.loadOBJ("Goomba.obj");
			//Goomba.CreateVertexBuffer();
			//Goomba.CreateIndexBuffer();

			vertexBufferData = { 0 };
			vertexBufferData.pSysMem = Goomba.m_vertices.data();
			vertexBufferData.SysMemPitch = 0;
			vertexBufferData.SysMemSlicePitch = 0;
			vertexBufferDesc = CD3D11_BUFFER_DESC(Goomba.m_vertices.size() * sizeof(RobustVertex), D3D11_BIND_VERTEX_BUFFER);
			result = m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				Goomba.m_vertexBuffer.GetAddressOf()
			);

			Goomba.m_indexCount = Goomba.m_indices.size();

			indexBufferData = { 0 };
			indexBufferData.pSysMem = Goomba.m_indices.data();
			indexBufferData.SysMemPitch = 0;
			indexBufferData.SysMemSlicePitch = 0;
			indexBufferDesc = CD3D11_BUFFER_DESC(Goomba.m_indices.size() * sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
			result =
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&indexBufferDesc,
					&indexBufferData,
					Goomba.m_indexBuffer.GetAddressOf()
				);

		}


		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Diffuse_Fuzzy.dds",
			NULL, &goombaSRV);

		result =  CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Box_Wood02Dark.dds",
								NULL, &cubeSRV);

		result = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"tilefloor2_seamless.dds",
			NULL, &floorSRV);
	});

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
	m_lightBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_indexFloor.Reset();
	m_vertexFloor.Reset();
}

Light CreateDirectionalLight(XMFLOAT4 direction, XMFLOAT4 color)
{
	Light tempLight;
	tempLight.color = color;
	tempLight.normal = direction;
	tempLight.ratio = XMFLOAT4(0.0f, 0.0f, 0.0f, NUM_LIGHTS);
	tempLight.position = XMFLOAT4(0.0f, 0.0f, 0.0f,0.0f);
	return tempLight;
}

Light CreatePointLight(XMFLOAT4 position, XMFLOAT4 color,float radius)
{
	Light tempLight = {};
	tempLight.position = position;
	tempLight.position.w = 1;
	tempLight.color = color;
	tempLight.ratio = XMFLOAT4(0.0f,0.0f, radius, NUM_LIGHTS);
	tempLight.normal = XMFLOAT4(0.0f, 0.0f, 0.0f,0.0f);
	return tempLight;
}

Light CreateSpotLight(XMFLOAT4 position, XMFLOAT4 direction, XMFLOAT4 color,float coneRatio)
{
	Light tempLight = {};
	tempLight.position = position;
	tempLight.position.w = 2;
	tempLight.color = color;
	tempLight.ratio = XMFLOAT4(coneRatio*0.95f,coneRatio, 10, NUM_LIGHTS);
	tempLight.normal = direction;
	return tempLight;
}

void Sample3DSceneRenderer::InitializeLights()
{
	m_lightBufferData.lights[0] = CreateDirectionalLight(XMFLOAT4(-0.577,-0.577,-0.577,0.0f), (XMFLOAT4)Colors::White);
	m_lightBufferData.lights[1] = CreatePointLight(XMFLOAT4(0.0f,0.0f,-1.0f,0.0f), (XMFLOAT4)Colors::OrangeRed,2.5f);
	m_lightBufferData.lights[2] = CreateSpotLight(XMFLOAT4(-0.75f,0.0f,0.0f,0.0f),XMFLOAT4(1,0,0,0.0f), (XMFLOAT4)Colors::White, 1);
}