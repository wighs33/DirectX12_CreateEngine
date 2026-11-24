#pragma once

#include <DirectXMath.h>

#include <string>
#include <vector>

// Scene File의 version입니다. 읽는 쪽과 쓰는 쪽이 같은 값을 씁니다.
namespace SceneFileFormat
{
    // version 1은 기본 도형 이름을 "primitive"에, version 2는 모델 이름이나 경로를 "model"에 적습니다.
    constexpr int PrimitiveOnlyVersion = 1;
    constexpr int CurrentVersion = 2;
}

// Scene File에 적히는 물체 하나입니다.
struct SceneObjectData
{
    std::string name;
    // 기본 도형의 이름이나 모델 파일의 경로입니다.
    std::string modelId;
    DirectX::XMFLOAT3 position{};
    DirectX::XMFLOAT3 rotationDegrees{};
    DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
};

// Scene File의 내용입니다. 모델을 찾기 전이라 GPU Resource와 상관이 없습니다.
struct SceneData
{
    std::vector<SceneObjectData> objects;
};
