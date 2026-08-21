#pragma once

namespace Engine::GFX
{
	class cScene;
}

namespace World 
{

	namespace WorldGenerator
	{
		void Generate(Engine::GFX::cScene& _rScene, int _seed);
	}

}