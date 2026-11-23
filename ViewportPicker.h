#pragma once

#include <d3d12.h>

#include <cstddef>

class Camera;
class Scene;
struct Ray;

// 화면에서 클릭한 자리에 있는 가장 가까운 물체를 찾습니다.
class ViewportPicker final
{
public:
    ViewportPicker() = delete;

    // 마우스가 3D 화면 안에 있으면 true입니다.
    static bool IsInside(
        const D3D12_VIEWPORT& viewport,
        const POINT& mousePosition);

    // 물체를 찾으면 true를 반환하고 그 번호를 돌려줍니다.
    static bool Pick(
        const Scene& scene,
        const Camera& camera,
        const D3D12_VIEWPORT& viewport,
        const POINT& mousePosition,
        std::size_t& objectIndex);

private:
    // 마우스 자리를 카메라에서 뻗어 나가는 3D 공간의 선으로 바꿉니다.
    static bool CreateRay(
        const Camera& camera,
        const D3D12_VIEWPORT& viewport,
        const POINT& mousePosition,
        Ray& ray);
};
