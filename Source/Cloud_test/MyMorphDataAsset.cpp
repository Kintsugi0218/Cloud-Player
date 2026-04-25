#include "MyMorphDataAsset.h"

FPrimaryAssetId UMyMorphDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId("MyMorphDataAsset", GetFName());
}