#pragma once

struct ModelData;

// 같은 자리에 있는 정점들의 법선을 평균해 외곽선을 밀어낼 방향을 만듭니다.
// 각진 곳의 정점은 면마다 법선이 달라서 그대로 밀면 외곽선이 갈라집니다.
class OutlineNormalBaker final
{
public:
    OutlineNormalBaker() = delete;

    // 모델의 모든 조각을 함께 보므로 재질이 바뀌는 이음매도 갈라지지 않습니다.
    static void Bake(ModelData& modelData);
};
