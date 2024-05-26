#include "Precompiled.h"
#include "ParticleSystem.h"

using namespace SpringEngine;
using namespace SpringEngine::Graphics;
using namespace SpringEngine::Math;

void ParticleSystem::Initialize(const ParticleSystemInfo& info)
{
	mInfo = info;
	mNextAvailableParticleIndex = 0;
	mNextSpawnTime = info.spawnDelay;
	mLifeTime = info.systemLifeTime;
	mParticleIndexes.resize(info.maxParticles);
	mParticles.resize(info.maxParticles);
	for (uint32_t i = 0; i < info.maxParticles; ++i)
	{
		mParticleIndexes[i] = i;
		mParticles[i].Initialize();
	}

	MeshPX particleMesh = MeshBuilder::CreateScreenQuad();
	mRenderObject.meshBuffer.Initialize(particleMesh);
	mRenderObject.diffuseMapId = info.particleTextureId;

}

void ParticleSystem::Terminate()
{
	mRenderObject.Terminate();
	for (Particle& p : mParticles)
	{
		p.Terminate();
	}
}

void ParticleSystem::Update(float deltaTime)
{
	if (mLifeTime > 0.0f)
	{
		mLifeTime -= deltaTime;
		mNextSpawnTime -= deltaTime;
		if (mNextSpawnTime <= 0.0f)
		{
			SpawnParticles();
		}
		for (Particle& p : mParticles)
		{
			p.Update(deltaTime);
		}
		std::sort(mParticleIndexes.begin(), mParticleIndexes.end(), [&](const int& a, const int& b)
			{
				float distSqA = Math::MagnitudeSqr(mParticles[a].GetTransform().position - mCamera->GetPosition());
				float distSqB = Math::MagnitudeSqr(mParticles[b].GetTransform().position - mCamera->GetPosition());
				return distSqA < distSqB;
			});

	}
}

void ParticleSystem::DebugUI()
{
	if (ImGui::CollapsingHeader("ParticleSystem", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("SpawnPosition", &mInfo.spawnPosition.x);
		if (ImGui::DragFloat3("SpawnDirection", &mInfo.spawnDirection.x))
		{
			mInfo.spawnDirection = Math::Normalize(mInfo.spawnDirection);
		}
		ImGui::DragInt("MinPerEmit", &mInfo.minParticlePerEmit);
		ImGui::DragInt("MaxperEmit", &mInfo.maxParticlePerEmit, 1, mInfo.minParticlePerEmit + 1, 10);
		ImGui::DragFloat("MinTime", &mInfo.minTimeBetweenEmit, 0.1f);
		ImGui::DragFloat("MaxTime", &mInfo.maxTimeBetweenEmit, 0.1f, mInfo.maxTimeBetweenEmit, 10.0f);
		ImGui::DragFloat("MinAngle", &mInfo.minSpawnAngle, 0.1f);
		ImGui::DragFloat("MaxAngle", &mInfo.maxSpawnAngle, 0.1f, mInfo.minSpawnAngle, 3.0f);
		ImGui::DragFloat("MinSpeed", &mInfo.minSpeed, 1.0f);
		ImGui::DragFloat("MaxSpeed", &mInfo.maxSpeed, 1.0f, mInfo.minSpeed, 100.0f);
		ImGui::DragFloat("MinLifeTime", &mInfo.minLifeTime, 0.1f);
		ImGui::DragFloat("MaxLifeTime", &mInfo.maxLifeTime, 0.1f, mInfo.minLifeTime, 10.0f);
		ImGui::ColorEdit4("MinStartColor", &mInfo.minStartColor.r);
		ImGui::ColorEdit4("MaxStartColor", &mInfo.maxStartColor.r);
		ImGui::ColorEdit4("MinEndColor", &mInfo.minEndColor.r);
		ImGui::ColorEdit4("MaxEndColor", &mInfo.maxEndColor.r);
		ImGui::DragFloat3("MinStartScale", &mInfo.minStartScale.x);
		ImGui::DragFloat3("MaxStartScale", &mInfo.maxStartScale.x);
		ImGui::DragFloat3("MinEndScale", &mInfo.minEndScale.x);
		ImGui::DragFloat3("MaxEndScale", &mInfo.maxEndScale.x);
	}

}

void ParticleSystem::SetCamera(Graphics::Camera& camera)
{
	mCamera = &camera;
}

void ParticleSystem::SpawnParticles()
{
	int numParticles = mInfo.minParticlePerEmit + 
		(rand() % (mInfo.maxParticlePerEmit - mInfo.minParticlePerEmit));
	for (int i = 0; i < numParticles; ++i)
	{
		SpawnParticle();
	}

	float randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	mNextSpawnTime = mInfo.minTimeBetweenEmit + 
		(randF * (mInfo.maxTimeBetweenEmit - mInfo.minTimeBetweenEmit));
}

void ParticleSystem::SpawnParticle()
{
	Particle& p = mParticles[mNextAvailableParticleIndex];
	mNextAvailableParticleIndex = (mNextAvailableParticleIndex + 1) % mParticles.size();

	Vector3 spawnDirection = mInfo.spawnDirection;
	if (mInfo.maxSpawnAngle > 0.0f)
	{
		float randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		float randAngle = mInfo.minSpawnAngle +
			(randF * (mInfo.maxSpawnAngle - mInfo.minSpawnAngle));

		Vector3 rotAxisA, rotAxisB;
		if (abs(Dot(mInfo.spawnDirection, Vector3::YAxis)) > 0.99f)
		{
			rotAxisA = Normalize(Cross(mInfo.spawnDirection, Vector3::XAxis));
			rotAxisB = Normalize(Cross(mInfo.spawnDirection, rotAxisA));
		}
		else
		{
			rotAxisA = Normalize(Cross(mInfo.spawnDirection, Vector3::YAxis));
			rotAxisB = Normalize(Cross(mInfo.spawnDirection, rotAxisA));
		}

		Matrix4 matRotA = Matrix4::RotationAxis(rotAxisA, randAngle);
		Matrix4 matRotB = Matrix4::RotationAxis(rotAxisB, randAngle);
		spawnDirection = TransformNormal(mInfo.spawnDirection, matRotA * matRotB);
	}

	float randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	float speed = mInfo.minSpeed + (randF * (mInfo.maxSpeed - mInfo.minSpeed));

	ParticleActivateData data;
	data.velocity = spawnDirection * speed;
	data.position = mInfo.spawnPosition;

	randF = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	data.lifeTime = mInfo.minLifeTime + (randF * (mInfo.maxLifeTime - mInfo.minLifeTime));

	float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	data.startColor = Lerp(mInfo.minStartColor, mInfo.maxStartColor, t);

	t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	data.endColor = Lerp(mInfo.minEndColor, mInfo.maxEndColor, t);

	t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	data.startScale = Lerp(mInfo.minStartScale, mInfo.maxStartScale, t);

	t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	data.endScale = Lerp(mInfo.minEndScale, mInfo.maxEndScale, t);

	p.Activate(data);

}
