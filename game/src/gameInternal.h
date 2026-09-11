#pragma once

#include "game.h"

#include "world/forestAtmosphere.h"

#include "graphics/light/light.h"
#include "graphics/light/lightManager.h"

#include "graphics/material/material.h"
#include "graphics/material/materialManager.h"

#include "graphics/reflectionProbes/reflectionProbeManager.h"
#include "graphics/vulkan/reflectionProbe.h"

#include "graphics/shapeModel/shapeModelDesc.h"
#include "graphics/shapeModel/shapeModelLights.h"
#include "graphics/shapeModel/shapeModelLoader.h"
#include "graphics/shapeModel/shapeModelManager.h"
#include "graphics/shapeModel/shapeMeshLibrary.h"

#include "physics/collisionWorld.h"

#include "spells/spellManager.h"

#include "world/chunk.h"
#include "world/terrainHeight.h"
#include "world/worldConfig.h"
#include "world/worldGenerator.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <unordered_set>

