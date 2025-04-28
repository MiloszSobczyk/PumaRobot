#include "particleSystem.h"

#include <iterator>

#include "dxDevice.h"
#include "exceptions.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const D3D11_INPUT_ELEMENT_DESC ParticleVertex::Layout[4] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "POSITION",1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

const XMFLOAT3 ParticleSystem::EMITTER_DIR = XMFLOAT3(0.8f, 0.5f, 0.0f);
const float ParticleSystem::TIME_TO_LIVE = 1.5f;
const float ParticleSystem::EMISSION_RATE = 100.0f;
const float ParticleSystem::MAX_ANGLE = XM_PIDIV2 / 2.0f;
const float ParticleSystem::MIN_VELOCITY = 0.5f;
const float ParticleSystem::MAX_VELOCITY = 3.f;
const float ParticleSystem::PARTICLE_SIZE = 0.08f;
const float ParticleSystem::PARTICLE_SCALE = 1.0f;
const float ParticleSystem::MIN_ANGLE_VEL = -XM_PI;
const float ParticleSystem::MAX_ANGLE_VEL = XM_PI;
const int ParticleSystem::MAX_PARTICLES = 500;
const float ParticleSystem::GRAVITY = 3.f;

vector<ParticleVertex> ParticleSystem::Update(float dt, DirectX::XMFLOAT4 cameraPosition, DirectX::XMFLOAT3 emitterPos)
{
	m_emitterPos = emitterPos;
	size_t removeCount = 0;
	for (auto& p : m_particles)
	{
		UpdateParticle(p, dt);
		if (p.Vertex.Age >= TIME_TO_LIVE)
			++removeCount;
	}
	m_particles.erase(m_particles.begin(), m_particles.begin() + removeCount);

	m_particlesToCreate += dt * EMISSION_RATE;
	while (m_particlesToCreate >= 1.0f)
	{
		--m_particlesToCreate;
		if (m_particles.size() < MAX_PARTICLES)
			m_particles.push_back(RandomParticle());
	}
	return GetParticleVerts(cameraPosition);
}

XMFLOAT3 ParticleSystem::RandomVelocity()
{
	static uniform_real_distribution angleDist{ 0.f, XM_2PI };
	static uniform_real_distribution magnitudeDist{ 0.f, tan(MAX_ANGLE) };
	static uniform_real_distribution velDist{ MIN_VELOCITY, MAX_VELOCITY };
	float angle = angleDist(m_random);
	float magnitude = magnitudeDist(m_random);
	XMFLOAT3 v{ cos(angle)*magnitude, 1.0f, sin(angle)*magnitude };
	XMVECTOR dir = XMLoadFloat3(&EMITTER_DIR);
	auto velocity = XMLoadFloat3(&v) + dir;
	auto len = velDist(m_random);
	velocity = len * XMVector3Normalize(velocity);
	XMStoreFloat3(&v, velocity);
	return v;
}

Particle ParticleSystem::RandomParticle()
{
	static uniform_real_distribution angularVelDist{ MIN_ANGLE_VEL, MAX_ANGLE_VEL };
	Particle p;
	// DONE : 1.27 Setup initial particle properties
	p.Vertex.Pos = m_emitterPos;
	p.Vertex.PreviousPos = m_emitterPos;
	p.Vertex.Age = 0.0f;
	p.Velocities.Velocity = RandomVelocity();
	p.Vertex.Size = PARTICLE_SIZE;

	return p;
}

void ParticleSystem::UpdateParticle(Particle& p, float dt)
{
	// DONE : 1.28 Update particle properties
	p.Vertex.Age += dt;
	p.Velocities.Velocity.y -= GRAVITY * dt;
	p.Vertex.PreviousPos = p.Vertex.Pos;
	p.Vertex.Pos.x += p.Velocities.Velocity.x * dt;
	p.Vertex.Pos.y += p.Velocities.Velocity.y * dt;
	p.Vertex.Pos.z += p.Velocities.Velocity.z * dt;
}

vector<ParticleVertex> ParticleSystem::GetParticleVerts(DirectX::XMFLOAT4 cameraPosition)
{

	XMFLOAT4 cameraTarget(0.0f, 0.0f, 0.0f, 1.0f);

	std::vector<ParticleVertex> vertices;

	// DONE : 1.29 Copy particles' vertex data to a vector and sort them
	vertices.reserve(m_particles.size());

	std::transform(m_particles.begin(), m_particles.end(),
		std::back_inserter(vertices),
		[](const Particle& p) { return p.Vertex; });

	XMVECTOR cameraPos = XMLoadFloat4(&cameraPosition);
	XMVECTOR cameraDir = XMVectorSubtract(XMLoadFloat4(&cameraTarget), cameraPos);

	std::sort(vertices.begin(), vertices.end(),
		[cameraPos](const ParticleVertex& p1, const ParticleVertex& p2) {
			const XMVECTOR p1Pos = XMLoadFloat3(&p1.Pos);
			const XMVECTOR p2Pos = XMLoadFloat3(&p2.Pos);

			const float d1 = XMVectorGetX(XMVector3LengthSq(p1Pos - cameraPos));
			const float d2 = XMVectorGetX(XMVector3LengthSq(p2Pos - cameraPos));

			return d1 > d2;
		});

	return vertices;
}