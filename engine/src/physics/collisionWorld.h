#pragma once

namespace Engine::Math
{
	template<typename T>
	class cVector3;

	using cVec3f = cVector3<float>;
}

namespace Engine::Physics
{
	struct sCapsuleCollider;
	struct sAABBCollider; 

	namespace CollisionWorld
	{
		void AddCollider(const sAABBCollider& _rCollider);
		void Clear(); 
		Math::cVec3f MoveCapsule(const sCapsuleCollider& _rCapsule, const Math::cVec3f& _rMovement);
		bool FindGroundHeight(const Math::cVec3f& _rPosition, float _maximumHeight, float& _rGroundHeight);
	}
}
