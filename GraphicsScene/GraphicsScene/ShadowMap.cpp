#include "pch.h"
#include "ShadowMap.h"

ShadowMap::ShadowMap()
{

}


ShadowMap::ShadowMap(ID3D11Device3* _device, unsigned int _width, unsigned int _height) :
	m_width(_width),
	m_height(_height),
	m_ShadowSRV(0),
	m_ShadowDSV(0)
{
	//Init ShadowViewPort
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = _width;
	m_viewport.Height = _height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	D3D11_TEXTURE2D_DESC tDesc;
	tDesc.Width = _width;
	tDesc.Height = _height;
	tDesc.MipLevels = 1;
	tDesc.ArraySize = 1;
	tDesc.Format = DXGI_FORMAT_R32_FLOAT;
	tDesc.SampleDesc.Count = 1;
	tDesc.SampleDesc.Quality = 0;
	tDesc.Usage = D3D11_USAGE_DEFAULT;
	tDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	tDesc.MiscFlags = 0;
	tDesc.BindFlags = D3D11_BIND_RENDER_TARGET |
					  D3D11_BIND_SHADER_RESOURCE;
	
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowMap;
	HRESULT result = _device->CreateTexture2D(&tDesc, nullptr, shadowMap.GetAddressOf());

	//D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	//dsvDesc.Flags = 0;
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//dsvDesc.Texture2D.MipSlice = 0;
	
	//result = _device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, m_ShadowDSV.GetAddressOf());

	//D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	//srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	//srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = tDesc.MipLevels;
	//srvDesc.Texture2D.MostDetailedMip = 0;

	result = _device->CreateShaderResourceView(shadowMap.Get(), nullptr, m_ShadowSRV.GetAddressOf());
	result = _device->CreateRenderTargetView1(shadowMap.Get(), nullptr, m_ShadowRTV.GetAddressOf());

	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		m_width,
		m_height,
		1, // This depth stencil view has only one texture.
		1, // Use a single mipmap level.
		D3D11_BIND_DEPTH_STENCIL
	);

	ID3D11Texture2D* depthStencil;
	result = _device->CreateTexture2D(
		&depthStencilDesc,
		nullptr,
		&depthStencil
	);


	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	result = _device->CreateDepthStencilView(
		depthStencil,
		&depthStencilViewDesc,
		m_ShadowDSV.GetAddressOf()
	);

	depthStencil->Release();
	depthStencil = nullptr;

}


//TODO: play with dynamic memory/Evil Three functions just for funsies and review for foundations Exam


void ShadowMap::BindDSVandSetNullRenderTarget(ID3D11DeviceContext3* _context)
{
	_context->RSSetViewports(1, &m_viewport);

	ID3D11RenderTargetView* targets[1] = { m_ShadowRTV.Get() };
	_context->OMSetRenderTargets(1, targets, m_ShadowDSV.Get());

	_context->ClearDepthStencilView(m_ShadowDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_context->ClearRenderTargetView(m_ShadowRTV.Get(), DirectX::Colors::White);

	//TODO: Note: better design for RTVs is to stack them where you can just push/pop RTVs,DSVs, and viewport
}