#include "Puma.h"
#include <array>
#include "mesh.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;
const XMFLOAT4 Puma::LIGHT_POS = { 2.0f, 3.0f, 3.0f, 1.0f };

Puma::Puma(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"League of Legends"),
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4, 2>()),
	m_vbParticleSystem(m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES))
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	DirectX::XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	for (int i = 0; i < MODEL_NUM; i++)
	{
		model[i].LoadModelFromFile("resources\\meshes\\mesh" + std::to_string(i + 1) + ".model");
		m_model[i] = Mesh::ModelMesh(m_device, model[i]);
	}
	m_cylinder = Mesh::Cylinder(m_device, 100, 100, 3.f, 0.5f);
	m_box = Mesh::InvertedShadedBox(m_device, 5.f);

	//Constant buffers content
	UpdateBuffer(m_cbLightPos, LIGHT_POS);

	//Render states
	RasterizerDescription rsDesc;
	rsDesc.FrontCounterClockwise = true;
	m_rsCCW = m_device.CreateRasterizerState(rsDesc);
	rsDesc.FrontCounterClockwise = false;
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);
	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCullBack = m_device.CreateRasterizerState(rsDesc);

	m_bsAdd = m_device.CreateBlendState(BlendDescription::AdditiveBlendDescription());
	m_bsAlpha = m_device.CreateBlendState(BlendDescription::AlphaBlendDescription());
	DepthStencilDescription dssDesc;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.StencilWriteMask = 0xff;
	dssDesc.StencilReadMask = 0xff;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.DepthEnable = true;
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);

	auto vsCode = m_device.LoadByteCode(L"phongVS.cso");
	auto psCode = m_device.LoadByteCode(L"phongPS.cso");
	m_phongVS = m_device.CreateVertexShader(vsCode);
	m_phongPS = m_device.CreatePixelShader(psCode);
	m_inputlayout = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//We have to make sure all shaders use constant buffers in the same slots!
	//Not all slots will be use by each shader
	ID3D11Buffer* vsb[] = { m_cbWorldMtx.get(),  m_cbViewMtx.get(), m_cbProjMtx.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx, 3: tex1Mtx, 4: tex2Mtx
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2); //Geometry Shaders - 0: projMtx
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos
}

void Puma::UpdateCameraCB(XMMATRIX viewMtx)
{
	XMVECTOR det;
	XMMATRIX invViewMtx = XMMatrixInverse(&det, viewMtx);
	XMFLOAT4X4 view[2];
	DirectX::XMStoreFloat4x4(view, viewMtx);
	DirectX::XMStoreFloat4x4(view + 1, invViewMtx);
	UpdateBuffer(m_cbViewMtx, view);
}

void Puma::Update(const Clock& c)
{
	double dt = c.getFrameTime();
	HandleCameraInput(dt);
}

void Puma::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorldMtx, mtx);
}

void Puma::SetSurfaceColor(DirectX::XMFLOAT4 color)
{
	UpdateBuffer(m_cbSurfaceColor, color);
}

void mini::gk2::Puma::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void mini::gk2::Puma::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void Puma::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void mini::gk2::Puma::DrawCylinder()
{
	XMFLOAT4X4 mtx;
	SetSurfaceColor({ 0.f, 0.75f, 0.f, 1.f });
	XMStoreFloat4x4(&mtx, XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(0.f, -1.f, -1.5f));
	DrawMesh(m_cylinder, mtx);
}

void mini::gk2::Puma::DrawBox()
{
	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, XMMatrixTranslation(0.f, 1.5f, 0.f));
	SetSurfaceColor({ 214.f / 255.f, 212.f / 255.f, 67.f / 255.f, 1.f });
	DrawMesh(m_box, mtx);
}

void mini::gk2::Puma::DrawModel()
{
	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, XMMatrixTranslation(0.f, 1.5f, 0.f));
	SetSurfaceColor({ 214.f / 255.f, 212.f / 255.f, 67.f / 255.f, 1.f });
	for(int i = 0; i < MODEL_NUM; i++)
		DrawMesh(m_model[i], mtx);
}

bool mini::gk2::Puma::HandleCameraInput(double dt)
{
	MouseState mstate;
	KeyboardState kbState;
	bool moved = false;
	if (!m_mouse.GetState(mstate))
		return false;

	if (!m_keyboard.GetState(kbState))
		return false;

	auto d = mstate.getMousePositionChange();
	if (mstate.isButtonDown(0))
	{
		m_camera.Rotate(d.y * ROTATION_SPEED, d.x * ROTATION_SPEED);
		moved = true;
	}
	else if (mstate.isButtonDown(1))
	{
		m_camera.Zoom(d.y * ZOOM_SPEED);
		moved = true;
	}

	XMVECTOR moveVec = XMVectorZero();
	float moveSpeed = MOVE_SPEED * dt; // Units per second

	if (kbState.isKeyDown(DIK_W))
		moveVec += m_camera.getForwardDir(); // world forward (+Z)

	if (kbState.isKeyDown(DIK_S))
		moveVec -= m_camera.getForwardDir(); // world backward (-Z)

	if (kbState.isKeyDown(DIK_A))
		moveVec -= m_camera.getRightDir(); // world left (-X)

	if (kbState.isKeyDown(DIK_D))
		moveVec += m_camera.getRightDir();  // world right (+X)

	if (kbState.isKeyDown(DIK_Q))
		moveVec += XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);  // world up (+Y)

	if (kbState.isKeyDown(DIK_E))
		moveVec += XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // world down (-Y)

	if (!XMVector3Equal(moveVec, XMVectorZero()))
	{
		moved = true;
		moveVec = XMVector3Normalize(moveVec);
		moveVec = XMVectorScale(moveVec, moveSpeed);
		m_camera.MoveTarget(moveVec);
	}

	return moved;
}

void Puma::DrawScene()
{
	SetShaders(m_phongVS, m_phongPS);

	UpdateCameraCB();

	DrawCylinder();
	DrawModel();
	DrawBox();
}

void Puma::Render()
{
	Base::Render();

	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);

	DrawScene();
}