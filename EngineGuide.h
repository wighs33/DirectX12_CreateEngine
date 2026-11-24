#pragma once

#include "EngineGuideData.h"

#include <filesystem>
#include <string>
#include <vector>

// 도감의 내용을 보관하고 카드, 기능, 단계를 이름으로 찾아 줍니다.
class EngineGuide final
{
public:
    // 모든 내용을 읽고 검사에 성공한 경우에만 기존 내용을 교체합니다.
    bool Load(const std::filesystem::path& filePath);

    const std::vector<GuideGroup>& GetGroups() const;
    const std::vector<GuideCard>& GetCards() const;
    const std::vector<GuideFeature>& GetFeatures() const;
    const std::vector<GuideStage>& GetStages() const;

    // 찾는 이름이 없으면 nullptr을 반환합니다.
    const GuideCard* FindCard(const std::string& id) const;
    const GuideFeature* FindFeature(const std::string& id) const;
    const GuideStage* FindStage(const std::string& id) const;

private:
    // 카드끼리의 연결, 기능의 순서, 단계의 커밋이 실제 있는 카드를 가리키는지 확인합니다.
    bool ValidateReferences() const;

    EngineGuideData data_;
};
