#include "Ray.h"

#include <algorithm>
#include <cmath>
#include <limits>

bool IntersectRayAABB(
    const Ray& ray,
    const AABB& bounds,
    float& intersectionDistance)
{
    constexpr float parallelEpsilon = 1.0e-6f;

    const float origins[] =
    {
        ray.origin.x,
        ray.origin.y,
        ray.origin.z
    };
    const float directions[] =
    {
        ray.direction.x,
        ray.direction.y,
        ray.direction.z
    };
    const float minimums[] =
    {
        bounds.minimum.x,
        bounds.minimum.y,
        bounds.minimum.z
    };
    const float maximums[] =
    {
        bounds.maximum.x,
        bounds.maximum.y,
        bounds.maximum.z
    };

    // t >= 0인 Ray 구간과 각 축 Slab의 진입/이탈 구간을 차례대로 교집합합니다.
    float minimumDistance = 0.0f;
    float maximumDistance = (std::numeric_limits<float>::max)();

    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::abs(directions[axis]) < parallelEpsilon)
        {
            if (origins[axis] < minimums[axis] ||
                origins[axis] > maximums[axis])
            {
                return false;
            }

            continue;
        }

        const float inverseDirection = 1.0f / directions[axis];
        float nearDistance =
            (minimums[axis] - origins[axis]) * inverseDirection;
        float farDistance =
            (maximums[axis] - origins[axis]) * inverseDirection;
        if (nearDistance > farDistance)
        {
            std::swap(nearDistance, farDistance);
        }

        minimumDistance = (std::max)(minimumDistance, nearDistance);
        maximumDistance = (std::min)(maximumDistance, farDistance);
        if (minimumDistance > maximumDistance)
        {
            return false;
        }
    }

    intersectionDistance = minimumDistance;
    return true;
}
