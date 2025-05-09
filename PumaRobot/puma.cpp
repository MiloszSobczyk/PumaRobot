#include "Puma.h"
#include <array>
#include "mesh.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;
const XMFLOAT4 Puma::LIGHT_POS = { 2.0f, 3.0f, 3.0f, 1.0f };
const DirectX::XMVECTOR Puma::MIRROR_NORMAL = { 0.f, 0.f, -1.f, 1.f };

Puma::Puma(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"League of Legends"),
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4, 2>()),
	m_cbTexTransform(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_vbParticleSystem(m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES)),
	m_mirrorTexture(m_device.CreateShaderResourceView(L"resources/textures/mirror.png")),
	m_particleTexture(m_device.CreateShaderResourceView(L"resources/textures/rain.png"))
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	DirectX::XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	XMMATRIX transform =  XMMatrixRotationX(XMConvertToRadians(135.f)) * XMMatrixRotationY(XMConvertToRadians(90.f)) * XMMatrixTranslation(-1.65f, 0.f, 0.f);
	XMStoreFloat4x4(&mirrorTransform, transform);
	m_mirror = Mesh::DoubleRect(m_device, radius * 3.f);

	for (int i = 0; i < MODEL_NUM; i++)
	{
		angles[i] = -30.f * i;
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
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	DepthStencilDescription writeDesc;
	writeDesc.StencilEnable = true;
	writeDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	writeDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	writeDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	writeDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssStencilWrite = m_device.CreateDepthStencilState(writeDesc);

	DepthStencilDescription testDesc;
	testDesc.StencilEnable = true;
	testDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	testDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	m_dssStencilTest = m_device.CreateDepthStencilState(testDesc);

	auto vsCode = m_device.LoadByteCode(L"phongVS.cso");
	auto psCode = m_device.LoadByteCode(L"phongPS.cso");
	m_phongVS = m_device.CreateVertexShader(vsCode);
	m_phongPS = m_device.CreatePixelShader(psCode);
	m_inputlayout = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	SamplerDescription sd;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.MaxAnisotropy = 16;

	m_samplerWrap = m_device.CreateSamplerState(sd);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	vsCode = m_device.LoadByteCode(L"texturedVS.cso");
	psCode = m_device.LoadByteCode(L"texturedPS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SamplerDescription sd1;
	// TODO : 0.01 Set to proper addressing (wrap) and filtering (16x anisotropic) modes of the sampler
	sd1.Filter = D3D11_FILTER_ANISOTROPIC;
	sd1.MaxAnisotropy = 16;
	sd1.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sd1.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd1.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd1.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_samplerWrap = m_device.CreateSamplerState(sd1);

	//We have to make sure all shaders use constant buffers in the same slots!
	//Not all slots will be use by each shader
	ID3D11Buffer* vsb[] = { m_cbWorldMtx.get(),  m_cbViewMtx.get(), m_cbProjMtx.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx, 3: tex1Mtx, 4: tex2Mtx
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2); //Geometry Shaders - 0: projMtx
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos
}

void mini::gk2::Puma::inverse_kinematics(DirectX::XMVECTOR pos, DirectX::XMVECTOR normal)
{
	const float l1 = 0.91f, l2 = 0.81f, l3 = 0.33f;
	const float dy = 0.27f, dz = 0.26f;

	// Normalize the normal vector
	normal = XMVector3Normalize(normal);

	// Compute the wrist position (back from the tool tip)
	XMVECTOR pos1 = pos + normal * l3;

	// Extract position components
	float x1 = XMVectorGetX(pos1);
	float y1 = XMVectorGetY(pos1);
	float z1 = XMVectorGetZ(pos1);

	// Compute projection in XZ-plane
	float e = sqrtf(z1 * z1 + x1 * x1 - dz * dz);
	angles[1] = atan2f(z1, -x1) + atan2f(dz, e);

	// Planar position for arm in Y-e plane
	XMVECTOR pos2 = XMVectorSet(e, y1 - dy, 0.0f, 0.0f);
	float px = XMVectorGetX(pos2);
	float py = XMVectorGetY(pos2);
	
	// Compute angle a3 (elbow)
	float len_sq = px * px + py * py;
	float cos_a3 = (len_sq - l1 * l1 - l2 * l2) / (2.0f * l1 * l2);
	cos_a3 = min(1.0f, max(-1.0f, cos_a3)); // clamp for safety
	angles[3] = -acosf(cos_a3);

	// Compute angle a2 (shoulder)
	float k = l1 + l2 * cosf(angles[3]);
	float l = l2 * sinf(angles[3]);
	angles[2] = -atan2f(py, sqrtf(px * px)) - atan2f(l, k);

	// Compute rotation of normal back through inverse joints
	XMMATRIX invRotY = XMMatrixRotationY(-angles[1]);
	XMMATRIX invRotZ = XMMatrixRotationZ(-(angles[2] + angles[3]));
	XMVECTOR normal1 = XMVector3TransformNormal(normal, invRotY);
	normal1 = XMVector3TransformNormal(normal1, invRotZ);

	float nx = XMVectorGetX(normal1);
	float ny = XMVectorGetY(normal1);
	float nz = XMVectorGetZ(normal1);

	angles[5] = acosf(nx); // assuming this is the twist of the end-effector
	angles[4] = atan2f(nz, ny);
}

DirectX::XMVECTOR mini::gk2::Puma::CalculateAnimation(const double& dt)
{
	const static float angleSpeed = 30.f;
	static float angle = 0.f;
	angle += static_cast<float>(dt) * angleSpeed;
	if (angle > 360.f)
		angle -= 360.f;
	float angleRad = XMConvertToRadians(angle);

	float x = radius * cosf(angleRad);
	float z = 0.f;
	float y = radius * sinf(angleRad);

	XMVECTOR position = XMVectorSet(x, y, z, 1.0f);

	XMVECTOR center = XMVectorSet(0.0f, 0.f, 0.f, 1.0f);
	XMVECTOR normal = MIRROR_NORMAL;

	position = XMVector3Transform(position, XMLoadFloat4x4(&mirrorTransform));
	normal = XMVector3TransformNormal(normal, XMLoadFloat4x4(&mirrorTransform));

	inverse_kinematics(position, normal);

	return position;
}

void mini::gk2::Puma::UpdateAnimation(const double& dt)
{
	if (inAnimation)
	{
		auto pos = CalculateAnimation(dt);
		UpdateParticles(dt, pos);
	}
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

void mini::gk2::Puma::UpdateParticles(const double& dt, DirectX::XMVECTOR emitterPos)
{
	XMFLOAT3 em;
	XMStoreFloat3(&em, emitterPos);
	UpdateBuffer(m_vbParticleSystem, m_particleSystem.Update(dt, m_camera.getCameraPosition(), em));
}

void Puma::Update(const Clock& c)
{	
	double dt = c.getFrameTime();
	
	if (m_keyboard.GetState(actualKeyboardState))
	{
		HandleCameraInput(dt);
		HandlePumaMovement(dt);
	}

	UpdateAnimation(dt);
	SetupWorldMatrices();

	previouseKebyoardState = actualKeyboardState;
}

void mini::gk2::Puma::SetupWorldMatrices()
{
	XMMATRIX worldMtx[MODEL_NUM];

	worldMtx[0] = XMMatrixIdentity();
	worldMtx[1] = XMMatrixRotationY(angles[1]);
	worldMtx[2] = XMMatrixTranslation(0.f, -0.27f, 0.f) * XMMatrixRotationZ(angles[2]) * XMMatrixTranslation(0.f, 0.27f, 0.f) * worldMtx[1];
	worldMtx[3] = XMMatrixTranslation(0.91f, -0.27f, 0.f) * XMMatrixRotationZ(angles[3]) * XMMatrixTranslation(-0.91f, 0.27f, 0.f) * worldMtx[2];
	worldMtx[4] = XMMatrixTranslation(0.f, -0.27f, 0.26f) * XMMatrixRotationX(angles[4]) * XMMatrixTranslation(0.f, 0.27f, -0.26f) * worldMtx[3];
	worldMtx[5] = XMMatrixTranslation(1.72f, -0.27f, 0.f) * XMMatrixRotationZ(angles[5]) * XMMatrixTranslation(-1.72f, 0.27f, 0.f) * worldMtx[4];


	for (int i = 0; i < MODEL_NUM; i++)
	{
		XMStoreFloat4x4(&m_modelWorldMatrices[i], worldMtx[i]);
	}
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

void mini::gk2::Puma::DrawMirror()
{
	float inv = 1.0f / (3.0f * radius);
	XMMATRIX texM = XMMatrixScaling(inv, inv, 1.f)
		* XMMatrixTranslation(0.5f, 0.5f, 0.f);
	XMStoreFloat4x4(&m_texTransformMtx, texM);

	UpdateBuffer(m_cbTexTransform, m_texTransformMtx);
	ID3D11Buffer* cb3 = m_cbTexTransform.get();
	m_device.context()->VSSetConstantBuffers(3, 1, &cb3);

	SetShaders(m_textureVS, m_texturePS);
	m_device.context()->IASetInputLayout(m_inputlayout.get());
	SetTextures({ m_mirrorTexture.get() }, m_samplerWrap);

	DrawMesh(m_mirror, mirrorTransform);
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
	SetSurfaceColor({ 0.8f, 0.8f, 0.2f, 1.f });
	DrawMesh(m_box, mtx);
}

void mini::gk2::Puma::DrawModel()
{
	SetSurfaceColor({ 0.8f, 0.8f, 0.8f, 1.f });
	for (int i = 0; i < MODEL_NUM; i++)
	{
		DrawMesh(m_model[i], m_modelWorldMatrices[i]);
	}
}

void Puma::DrawScene()
{
	DrawMirroredScene();

	UpdateCameraCB();

	DrawCylinder();
	DrawModel();
	DrawBox();
	DrawParticles();

	DrawMirror();
}

void mini::gk2::Puma::DrawMirroredScene()
{
	m_device.context()->OMSetDepthStencilState(m_dssStencilWrite.get(), 1);
	UpdateCameraCB();
	DrawMirror();

	m_device.context()->OMSetDepthStencilState(m_dssStencilTest.get(), 1);

	SetShaders(m_phongVS, m_phongPS);

	XMMATRIX mirror = XMMatrixScaling(1.f, 1.f, -1.f);
	XMMATRIX model = XMLoadFloat4x4(&mirrorTransform);
	XMMATRIX inv = XMMatrixInverse(nullptr, model);
	XMMATRIX mirrorRef = inv * mirror * model;

	m_device.context()->RSSetState(m_rsCCW.get());

	XMMATRIX viewMtx = m_camera.getViewMatrix();
	UpdateCameraCB(mirrorRef * viewMtx);

	XMVECTOR lightPosVec = XMLoadFloat4(&LIGHT_POS);
	lightPosVec = XMVector3Transform(lightPosVec, mirrorRef);
	XMFLOAT4 mirroredLight;
	XMStoreFloat4(&mirroredLight, lightPosVec);
	UpdateBuffer(m_cbLightPos, mirroredLight);

	DrawCylinder();
	DrawModel();
	DrawBox();
	DrawParticles();

	UpdateBuffer(m_cbLightPos, LIGHT_POS);

	UpdateCameraCB();
	m_device.context()->RSSetState(nullptr);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);

	m_device.context()->OMSetBlendState(m_bsAdd.get(), nullptr, 0xFFFFFFFF);
	DrawMirror();
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	SetShaders(m_phongVS, m_phongPS);
}

void mini::gk2::Puma::DrawParticles()
{
	if (m_particleSystem.particlesCount() == 0)
		return;

	m_device.context()->IASetInputLayout(m_particleLayout.get());
	SetShaders(m_particleVS, m_particlePS);
	SetTextures({ m_particleTexture.get() }, m_samplerWrap);
	m_device.context()->GSSetShader(m_particleGS.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	auto vb = m_vbParticleSystem.get();
	m_device.context()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_device.context()->OMSetBlendState(m_bsAdd.get(), nullptr, 0xFFFFFFFF);
	m_device.context()->Draw(m_particleSystem.particlesCount(), 0);

	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Puma::Render()
{
	Base::Render();

	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);

	DrawScene();
}

bool mini::gk2::Puma::HandleCameraInput(double dt)
{
	MouseState mstate;
	bool moved = false;
	if (!m_mouse.GetState(mstate))
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

	if (actualKeyboardState.isKeyDown(DIK_W))
		moveVec += m_camera.getForwardDir(); // world forward (+Z)

	if (actualKeyboardState.isKeyDown(DIK_S))
		moveVec -= m_camera.getForwardDir(); // world backward (-Z)

	if (actualKeyboardState.isKeyDown(DIK_A))
		moveVec -= m_camera.getRightDir(); // world left (-X)

	if (actualKeyboardState.isKeyDown(DIK_D))
		moveVec += m_camera.getRightDir();  // world right (+X)

	if (actualKeyboardState.isKeyDown(DIK_Q))
		moveVec += XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);  // world up (+Y)

	if (actualKeyboardState.isKeyDown(DIK_E))
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

void mini::gk2::Puma::HandlePumaMovement(double dt)
{
	if (actualKeyboardState.keyPressed(previouseKebyoardState, DIK_C))
	{
		inAnimation = !inAnimation;
	}

	if (inAnimation) return;

	static const float velocity = 5.f;

	for (int i = 0; i < 5; i++)
	{
		if (actualKeyboardState.isKeyDown(DIK_1 + 2 * i))
		{
			angles[i + 1] += velocity * dt;
		}
		if (actualKeyboardState.isKeyDown(DIK_2 + 2 * i))
		{
			angles[i + 1] -= velocity * dt;
		}
	}
}