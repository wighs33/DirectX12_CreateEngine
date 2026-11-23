#include "GuideNavigation.h"

void GuideNavigation::Request(const std::string& id)
{
    requestedId_ = id;
    hasRequest_ = true;
}

void GuideNavigation::RequestBack()
{
    isBackRequested_ = true;
}

bool GuideNavigation::Apply()
{
    bool isChanged = false;
    if (isBackRequested_)
    {
        isBackRequested_ = false;
        if (!history_.empty())
        {
            currentId_ = history_.back();
            history_.pop_back();
            isChanged = true;
        }
    }

    if (hasRequest_)
    {
        hasRequest_ = false;
        if (requestedId_ != currentId_)
        {
            history_.push_back(currentId_);
            currentId_ = requestedId_;
            isChanged = true;
        }
    }
    return isChanged;
}

const std::string& GuideNavigation::GetCurrentId() const
{
    return currentId_;
}

bool GuideNavigation::CanGoBack() const
{
    return !history_.empty();
}
