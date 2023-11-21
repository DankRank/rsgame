// SPDX-License-Identifier: Apache-2.0 OR MIT
#ifndef RSGAME_COMMON
#define RSGAME_COMMON
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#ifndef RSGAME_SERVER
#include <SDL.h>
#include <epoxy/gl.h>
#endif
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>
#include <memory>
#include <utility>
#include <algorithm>
namespace rsgame {
	using glm::vec3;
	using glm::mat4;
};
#endif
