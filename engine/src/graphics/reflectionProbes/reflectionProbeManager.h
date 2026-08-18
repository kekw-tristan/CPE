#pragma once 

#include "math/vector3.h"

#include <cstdint>
#include <vector>

namespace Engine::GFX
{
	using ReflectionProbeHandle = uint32_t;

	struct sReflectionProbe;

	namespace ReflectionProbeManager
	{
		ReflectionProbeHandle AddProbe(const sReflectionProbe& _rProbe);
		void SetProbe(ReflectionProbeHandle _probeHandle, const sReflectionProbe& _rProbe);

		void Clear();

		void  SetCellSize(float _cellSize);
		float GetCellSize();

		uint32_t GetProbeCount();

		const sReflectionProbe& GetProbe(ReflectionProbeHandle _probeIndex);
		const std::vector<sReflectionProbe>& GetProbes();

		std::vector<ReflectionProbeHandle> FindActiveProbeIndices(const Math::cVec3f& _rPosition, uint32_t _maxProbeCount, int32_t _searchRadiusCells);
		
		void SetProbeDirty(ReflectionProbeHandle _probeHandle, bool _dirty);
	}
}