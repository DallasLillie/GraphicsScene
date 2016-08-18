#pragma once
#include "DDSTextureLoader.h"
#include "..\Common\DeviceResources.h"
#include "Content\ShaderStructures.h"
using namespace GraphicsScene;
using namespace DirectX; 

class ShadowMap
{
public:

	//TODO: Private members
	unsigned int m_width;
	unsigned int m_height;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_ShadowSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_ShadowDSV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView1> m_ShadowRTV;

	ShadowMap();

	D3D11_VIEWPORT m_viewport;

	ShadowMap(ID3D11Device3* _device, unsigned int _width, unsigned int _height);

	void BindDSVandSetNullRenderTarget(ID3D11DeviceContext3* _context);
};


