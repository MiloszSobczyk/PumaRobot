#pragma once
#include "dxApplication.h"
#include "mesh.h"
#include "environmentMapper.h"
#include "particleSystem.h"

namespace mini::gk2
{
	class Puma : public DxApplication
	{
	public:
		using Base = DxApplication;

		explicit Puma(HINSTANCE appInstance);

	protected:
		void Update(const Clock& dt) override;
		void Render() override;

	private:
#pragma region CONSTANTS
		static const DirectX::XMFLOAT4 LIGHT_POS;
		static const unsigned int MODEL_NUM = 6;
		static const DirectX::XMVECTOR MIRROR_NORMAL;
#pragma endregion
		dx_ptr<ID3D11Buffer> m_cbWorldMtx, //vertex shader constant buffer slot 0
			m_cbProjMtx;	//vertex shader constant buffer slot 2 & geometry shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbViewMtx; //vertex shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbSurfaceColor;	//pixel shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbLightPos; //pixel shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbTexTransform;
		DirectX::XMFLOAT4X4 m_texTransformMtx;

		dx_ptr<ID3D11Buffer> m_vbParticleSystem;
		dx_ptr<ID3D11ShaderResourceView> m_particleTexture;
		dx_ptr<ID3D11ShaderResourceView> m_mirrorTexture;

		KeyboardState actualKeyboardState;
		KeyboardState previouseKebyoardState;

		bool inAnimation = false;

		float radius = 0.5f;
		DirectX::XMFLOAT4X4 mirrorTransform;
		Mesh m_mirror;

		float angles[MODEL_NUM];
		DirectX::XMFLOAT4X4 m_modelWorldMatrices[MODEL_NUM];
		Model model[MODEL_NUM];
		Mesh m_model[MODEL_NUM];
		Mesh m_cylinder;
		Mesh m_box;

		DirectX::XMFLOAT4X4 m_projMtx;

		ParticleSystem m_particleSystem;

		dx_ptr<ID3D11RasterizerState> m_rsCullFront;
		dx_ptr<ID3D11RasterizerState> m_rsCullBack;
		dx_ptr<ID3D11RasterizerState> m_rsCCW;
		dx_ptr<ID3D11BlendState> m_bsAlpha;
		dx_ptr<ID3D11BlendState> m_bsAdd;
		dx_ptr<ID3D11DepthStencilState> m_dssNoWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTest;
		dx_ptr<ID3D11SamplerState> m_samplerWrap;

		dx_ptr<ID3D11InputLayout> m_inputlayout, m_particleLayout;

		dx_ptr<ID3D11VertexShader> m_phongVS, m_particleVS, m_textureVS;
		dx_ptr<ID3D11GeometryShader> m_particleGS;
		dx_ptr<ID3D11PixelShader> m_phongPS, m_particlePS, m_texturePS;

		void inverse_kinematics(DirectX::XMVECTOR pos, DirectX::XMVECTOR normal);
		DirectX::XMVECTOR CalculateAnimation(const double& dt);

		void UpdateAnimation(const double& dt);
		void UpdateCameraCB(DirectX::XMMATRIX viewMtx);
		void UpdateCameraCB() { UpdateCameraCB(m_camera.getViewMatrix()); }
		void UpdateParticles(const double& dt, DirectX::XMVECTOR emitterPos);

		void DrawMirror();
		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void DrawCylinder();
		void DrawBox();
		void DrawModel();
		void DrawParticles();

		void SetupWorldMatrices();

		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void SetSurfaceColor(DirectX::XMFLOAT4 color);
		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList) { SetTextures(std::move(resList), m_samplerWrap); }

		bool HandleCameraInput(double dt) override;
		void HandlePumaMovement(double dt);

		void DrawScene();
		void DrawMirroredScene();
	};
}