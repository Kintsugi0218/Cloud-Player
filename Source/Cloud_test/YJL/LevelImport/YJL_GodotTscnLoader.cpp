// YJL_GodotTscnLoader.cpp
// .tscn 文件解析器实现。
//
// 实现策略：
//   1. 单次扫读全文件，按 [section_header] 分块
//   2. 每个 [node ...] 后跟若干 "key = value" 直到下一个 [
//   3. 用类型名分发到 SpawnXxx 函数生成对应 UE component
//   4. 父子关系：维护 path → SceneComponent 表，按节点 parent 路径查
//
// 坐标系转换：Godot (X右, Y上, Z前-左手) → UE (X前, Y右, Z上)
//   位置：(gx, gy, gz) → (-gz, gx, gy) * UnitScale
//   旋转/缩放矩阵：基向量按相同规则换分量
//   实际做法：构造 Godot 的 4x4 矩阵，左乘 / 右乘转换矩阵 C 得到 UE 矩阵

#include "YJL_GodotTscnLoader.h"

#include "../YJL_Common.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Components/LightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

// =========================================================
// 内部数据结构 & 工具
// =========================================================
namespace
{
    // 每个 [section] + 后续 key=value 块
    struct FTscnBlock
    {
        FString Header;                      // 整行 [...] 内容，包含中括号
        FString HeaderType;                  // ext_resource / sub_resource / node / 其他
        TMap<FString, FString> HeaderAttrs;  // header 里 key=value（如 type="CSGBox3D" parent="."）
        TMap<FString, FString> Props;        // 块体内 key = value
    };

    // 单个解析后的节点（仅 [node ...] 块）
    struct FTscnNode
    {
        FString Name;
        FString Type;                  // 显式 type=...，若是 instance= 则填 "(Instance)"
        FString ParentPath;            // Godot 形式 "." / "Terrain" / "Terrain/SpawnMound"
        bool bIsInstance = false;      // 是否是 instance=ExtResource(...)
        bool bHasScript = false;       // 是否有 script = ExtResource(...)
        FString TransformRaw;          // 原始 "Transform3D(...)" 字符串（可能为空）
        TMap<FString, FString> Props;  // 节点的其他属性（size / radius / height / material / mesh / shape ...）
    };

    // ===== 字符串工具 =====
    static FString Trim(const FString& In)
    {
        FString S = In;
        S.TrimStartAndEndInline();
        return S;
    }

    // 解析 header 内 key="value" 或 key=value 的 attrs（用于 [node name="X" type="Y" parent="Z"]）
    static void ParseHeaderAttrs(const FString& HeaderInside, TMap<FString, FString>& Out)
    {
        // HeaderInside 形如：node name="MainGround" type="CSGBox3D" parent="Terrain" unique_id=58281693
        // 简易状态机解析
        int32 i = 0;
        const int32 N = HeaderInside.Len();
        // 跳过开头的 token（"node" / "ext_resource" 等），不当 attr
        while (i < N && HeaderInside[i] != ' ') ++i;

        while (i < N)
        {
            while (i < N && FChar::IsWhitespace(HeaderInside[i])) ++i;
            if (i >= N) break;

            // 读 key
            int32 KeyStart = i;
            while (i < N && HeaderInside[i] != '=' && !FChar::IsWhitespace(HeaderInside[i])) ++i;
            FString Key = HeaderInside.Mid(KeyStart, i - KeyStart);
            if (i >= N || HeaderInside[i] != '=') break;
            ++i; // 跳过 '='

            // 读 value（带引号 / 不带）
            FString Val;
            if (i < N && HeaderInside[i] == '"')
            {
                ++i;
                int32 VS = i;
                while (i < N && HeaderInside[i] != '"') ++i;
                Val = HeaderInside.Mid(VS, i - VS);
                if (i < N) ++i; // 跳过 '"'
            }
            else
            {
                // 不带引号，可能是 ExtResource("1_player") / 数字 / 字面量
                // 直接读到下一个空格（但要跳过括号内空格 —— 这里 ExtResource(...) 内无空格，OK）
                int32 VS = i;
                int32 Depth = 0;
                while (i < N)
                {
                    TCHAR C = HeaderInside[i];
                    if (C == '(') ++Depth;
                    else if (C == ')') --Depth;
                    else if (Depth == 0 && FChar::IsWhitespace(C)) break;
                    ++i;
                }
                Val = HeaderInside.Mid(VS, i - VS);
            }
            Out.Add(Key, Val);
        }
    }

    // 分块读取 .tscn 文件 → TArray<FTscnBlock>
    static bool LoadTscnBlocks(const FString& FilePath, TArray<FTscnBlock>& OutBlocks, FString& OutError)
    {
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *FilePath))
        {
            OutError = FString::Printf(TEXT("无法读取文件：%s"), *FilePath);
            return false;
        }

        TArray<FString> Lines;
        Content.ParseIntoArrayLines(Lines, /*bCullEmpty=*/false);

        FTscnBlock* Current = nullptr;
        for (int32 li = 0; li < Lines.Num(); ++li)
        {
            const FString Raw = Lines[li];
            FString L = Trim(Raw);
            if (L.IsEmpty()) continue;

            if (L.StartsWith(TEXT("[")))
            {
                // 新的 section
                FTscnBlock Block;
                Block.Header = L;

                // 取 [ 和 ] 之间的内容
                int32 RBracket = L.Find(TEXT("]"));
                if (RBracket > 0)
                {
                    FString Inside = L.Mid(1, RBracket - 1);
                    Inside = Trim(Inside);

                    // 首 token = header type
                    int32 SpacePos;
                    if (Inside.FindChar(' ', SpacePos))
                    {
                        Block.HeaderType = Inside.Left(SpacePos);
                    }
                    else
                    {
                        Block.HeaderType = Inside;
                    }
                    ParseHeaderAttrs(Inside, Block.HeaderAttrs);
                }

                OutBlocks.Add(Block);
                Current = &OutBlocks.Last();
            }
            else if (Current)
            {
                // 块体内的 key = value
                int32 EqPos;
                if (L.FindChar('=', EqPos))
                {
                    FString K = Trim(L.Left(EqPos));
                    FString V = Trim(L.Mid(EqPos + 1));
                    Current->Props.Add(K, V);
                }
            }
        }
        return true;
    }

    // 解析 "Transform3D(b00, b01, b02,  b10, b11, b12,  b20, b21, b22,  tx, ty, tz)"
    // 并转换到 UE 的 FTransform（含坐标系换算）。
    //
    // Godot Transform3D 序列化：12 个浮点，前 9 个是 basis（按列存还是按行存？
    //   Godot 文档：Transform3D(basis_xx, basis_xy, basis_xz,  basis_yx, basis_yy, basis_yz,  basis_zx, basis_zy, basis_zz,  origin_x, origin_y, origin_z)
    //   即 basis 矩阵的"行优先" 9 个数。basis * v 让局部向量映射到世界向量。
    //   注意：Godot 3.x 的 .tscn 也是这个格式。
    //
    // 坐标系：
    //   Godot：X 右，Y 上，Z 前（朝向相机），右手系
    //   UE：   X 前，Y 右，Z 上，左手系
    //   映射：UE_x = -Godot_z;  UE_y = Godot_x;  UE_z = Godot_y;
    //
    // 这等价于在 Godot 空间和 UE 空间间用变换矩阵 C 共轭：M_ue = C * M_godot * C^-1
    // 其中 C 是从 godot basis vector 到 UE basis vector 的映射。
    //
    // 简化做法：因为 C 是 permutation+flip，对位置和 basis 各列分别套用 (gx,gy,gz)→(-gz,gx,gy) 即可。
    // 对 basis 还需要把它当作"列向量集"做同样变换 → 见代码。
    static bool ParseGodotTransform3D(const FString& Raw, float UnitScale, FTransform& OutUE)
    {
        // 期望以 "Transform3D(" 开头
        int32 LP = Raw.Find(TEXT("("));
        int32 RP = Raw.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (LP < 0 || RP <= LP) return false;

        FString Inner = Raw.Mid(LP + 1, RP - LP - 1);
        TArray<FString> Toks;
        Inner.ParseIntoArray(Toks, TEXT(","), true);
        if (Toks.Num() < 12) return false;

        TArray<float> F;
        F.Reserve(12);
        for (int32 i = 0; i < 12; ++i)
        {
            F.Add(FCString::Atof(*Trim(Toks[i])));
        }

        // basis 按 Godot 约定：Transform3D(xx, xy, xz, yx, yy, yz, zx, zy, zz, tx, ty, tz)
        // 实际 Godot 序列化的是 basis 的"3 个列向量"（每个列向量 3 个分量），即：
        //   X_axis = (F[0], F[1], F[2])  → basis * (1,0,0)
        //   Y_axis = (F[3], F[4], F[5])  → basis * (0,1,0)
        //   Z_axis = (F[6], F[7], F[8])  → basis * (0,0,1)
        // origin = (F[9], F[10], F[11])
        //
        // 也就是说 basis_matrix（列向量排列）：
        //   | F[0] F[3] F[6] |
        //   | F[1] F[4] F[7] |
        //   | F[2] F[5] F[8] |
        FVector GodotXAxis(F[0], F[1], F[2]);
        FVector GodotYAxis(F[3], F[4], F[5]);
        FVector GodotZAxis(F[6], F[7], F[8]);
        FVector GodotOrigin(F[9], F[10], F[11]);

        // 坐标转换函数 (Godot → UE)
        auto G2U = [](const FVector& G) -> FVector
        {
            // UE_x = -Godot_z;  UE_y = Godot_x;  UE_z = Godot_y
            return FVector(-G.Z, G.X, G.Y);
        };

        // 位置：直接换 + 乘 UnitScale
        FVector UEPos = G2U(GodotOrigin) * UnitScale;

        // basis 的 3 个轴也分别换。但还要注意：Godot 的 X/Y/Z 轴语义跟 UE 不一样，
        // 我们的 basis "Godot 局部 → Godot 世界" 要变成 "UE 局部 → UE 世界"。
        // 局部空间也要做相同的轴变换：
        //   UE_local_x = -Godot_local_z;  UE_local_y = Godot_local_x;  UE_local_z = Godot_local_y
        // 即 UE basis 的列向量是 G2U 应用到对应的 Godot 列向量上：
        //   UE_axis_X = G2U(GodotZAxis) * -1  ←等等，这里要小心
        //
        // 推导：UE basis * UE_local = UE_world
        //   UE_world = G2U(Godot_world) = G2U(Godot basis * Godot_local)
        //   UE_local = G2U(Godot_local)（按定义）
        //
        // 想要 UE_basis 的第 i 个列向量 = UE basis * e_i
        //   令 e_i_UE → 对应 Godot 的哪个 e_j_godot？由 UE_local = G2U(Godot_local) 反推：
        //     e_x_UE = (1,0,0) UE → 反推 Godot_local：要找 Godot_local 使 G2U(Godot_local) = (1,0,0)
        //       G2U(g) = (-g.z, g.x, g.y) = (1, 0, 0) ⇒ g.z = -1, g.x = 0, g.y = 0 ⇒ Godot_local = (0,0,-1) = -e_z_godot
        //     e_y_UE = (0,1,0) → Godot_local = e_x_godot
        //     e_z_UE = (0,0,1) → Godot_local = e_y_godot
        //
        // 所以：
        //   UE_basis * e_x_UE = G2U(Godot_basis * (-e_z_godot)) = G2U(-GodotZAxis)
        //   UE_basis * e_y_UE = G2U(GodotXAxis)
        //   UE_basis * e_z_UE = G2U(GodotYAxis)
        FVector UE_ColX = G2U(-GodotZAxis);
        FVector UE_ColY = G2U(GodotXAxis);
        FVector UE_ColZ = G2U(GodotYAxis);

        // FMatrix UE 是行优先（4x4 行排列），M.M[row][col]
        // 构造 4x4：列向量 X/Y/Z + 平移
        FMatrix Mtx = FMatrix::Identity;
        Mtx.M[0][0] = UE_ColX.X; Mtx.M[0][1] = UE_ColX.Y; Mtx.M[0][2] = UE_ColX.Z;
        Mtx.M[1][0] = UE_ColY.X; Mtx.M[1][1] = UE_ColY.Y; Mtx.M[1][2] = UE_ColY.Z;
        Mtx.M[2][0] = UE_ColZ.X; Mtx.M[2][1] = UE_ColZ.Y; Mtx.M[2][2] = UE_ColZ.Z;
        Mtx.M[3][0] = UEPos.X;   Mtx.M[3][1] = UEPos.Y;   Mtx.M[3][2] = UEPos.Z;

        OutUE.SetFromMatrix(Mtx);
        return true;
    }

    // 解析 "Vector3(a, b, c)" → FVector
    static bool ParseVec3(const FString& Raw, FVector& Out)
    {
        int32 LP = Raw.Find(TEXT("("));
        int32 RP = Raw.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (LP < 0 || RP <= LP) return false;
        TArray<FString> Toks;
        Raw.Mid(LP + 1, RP - LP - 1).ParseIntoArray(Toks, TEXT(","), true);
        if (Toks.Num() < 3) return false;
        Out = FVector(
            FCString::Atof(*Trim(Toks[0])),
            FCString::Atof(*Trim(Toks[1])),
            FCString::Atof(*Trim(Toks[2]))
        );
        return true;
    }

    // 解析 ExtResource("4_grass") → "4_grass"（去掉双引号）
    static FString ParseExtResourceId(const FString& Raw)
    {
        int32 LP = Raw.Find(TEXT("("));
        int32 RP = Raw.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        if (LP < 0 || RP <= LP) return TEXT("");
        FString Id = Raw.Mid(LP + 1, RP - LP - 1);
        Id = Trim(Id);
        Id.RemoveFromStart(TEXT("\""));
        Id.RemoveFromEnd(TEXT("\""));
        return Id;
    }
}

// =========================================================
// AYJL_GodotLevelHost 实现
// =========================================================
AYJL_GodotLevelHost::AYJL_GodotLevelHost()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* SR = CreateDefaultSubobject<USceneComponent>(TEXT("YJL_HostRoot"));
    SetRootComponent(SR);

    GeneratedRoot = CreateDefaultSubobject<USceneComponent>(TEXT("YJL_GeneratedRoot"));
    GeneratedRoot->SetupAttachment(SR);
}

void AYJL_GodotLevelHost::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 关键修复：OnConstruction 在 UE Editor 里每次拖动/旋转/缩放 Actor 都会被触发，
    // 之前无脑 BuildFromTscn 导致每次拖动都重新解析整个 .tscn 并累加几何。
    //
    // 新策略：只在 GeneratedRoot 当前没有子组件时构建（即首次拖到场景 / 关卡加载时）。
    // 后续重建只通过 PostEditChangeProperty 的白名单属性 或 Play 时的 BeginPlay 走显式调用。
    if (GeneratedRoot && GeneratedRoot->GetAttachChildren().Num() == 0)
    {
        BuildFromTscn();
    }
}

void AYJL_GodotLevelHost::BeginPlay()
{
    Super::BeginPlay();
    // 运行时启动时如果几何为空（Editor 里没勾过 Build / 切到 Play 模式后的 PIE 实例）
    // 再补一次构建，保证 Play 时一定有几何
    if (GeneratedRoot && GeneratedRoot->GetAttachChildren().Num() == 0)
    {
        BuildFromTscn();
    }
}

#if WITH_EDITOR
void AYJL_GodotLevelHost::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 只有"导入相关"属性变化才重建；拖动 Actor / 改 Tag 等不重建（避免累加 + 性能差）
    const FName Changed = PropertyChangedEvent.MemberProperty
        ? PropertyChangedEvent.MemberProperty->GetFName()
        : NAME_None;

    static const TSet<FName> RebuildTriggers = {
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, TscnFilePath),
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, UnitScale),
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, FallbackMaterial),
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, MaterialMap),
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, bRebuildNow),
        GET_MEMBER_NAME_CHECKED(AYJL_GodotLevelHost, bEnableCollision),
    };

    if (RebuildTriggers.Contains(Changed))
    {
        BuildFromTscn();
    }
}
#endif

void AYJL_GodotLevelHost::ClearGeneratedChildren()
{
    // 暴力清理：遍历 Actor 持有的所有组件，凡是不是 RootComponent 也不是 GeneratedRoot 的
    // SceneComponent 全部销毁。不依赖 attach 关系（之前的 BUG 就是依赖 attach 链，
    // 孤儿组件 / 累加残留 / 被 reparent 的组件统统会漏掉）。
    USceneComponent* RC = GetRootComponent();

    TArray<UActorComponent*> AllComps;
    GetComponents(AllComps);

    int32 NumKilled = 0;
    for (UActorComponent* Comp : AllComps)
    {
        if (!Comp || !IsValid(Comp)) continue;
        if (Comp == RC || Comp == GeneratedRoot) continue;

        if (USceneComponent* SC = Cast<USceneComponent>(Comp))
        {
            SC->DestroyComponent(/*bPromoteChildren*/ false);
            ++NumKilled;
        }
    }

    UE_LOG(LogYJL, Log, TEXT("[GodotLoader] ClearGeneratedChildren: 销毁 %d 个 SceneComponent"), NumKilled);
}

// =========================================================
// Editor 工具按钮
// =========================================================
void AYJL_GodotLevelHost::ForceClearAll()
{
    ClearGeneratedChildren();
    if (bScreenLog && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan,
            FString::Printf(TEXT("[GodotLoader] %s: ForceClearAll 完成"), *GetName()));
    }
}

void AYJL_GodotLevelHost::ForceRebuild()
{
    BuildFromTscn();
}

void AYJL_GodotLevelHost::Diagnose()
{
    TArray<UActorComponent*> AllComps;
    GetComponents(AllComps);

    int32 NumScene = 0, NumStatic = 0, NumLight = 0, NumOther = 0;
    for (UActorComponent* C : AllComps)
    {
        if (!C) continue;
        if (C->IsA(UStaticMeshComponent::StaticClass())) ++NumStatic;
        else if (C->IsA(UDirectionalLightComponent::StaticClass()) ||
                 C->IsA(UPointLightComponent::StaticClass())) ++NumLight;
        else if (C->IsA(USceneComponent::StaticClass())) ++NumScene;
        else ++NumOther;
    }

    const int32 RootChildren = GeneratedRoot ? GeneratedRoot->GetAttachChildren().Num() : -1;
    const FString Msg = FString::Printf(
        TEXT("[GodotLoader] %s 诊断: 总组件=%d, StaticMesh=%d, Light=%d, SceneComp=%d, 其他=%d, GeneratedRoot直接子=%d"),
        *GetName(), AllComps.Num(), NumStatic, NumLight, NumScene, NumOther, RootChildren);

    UE_LOG(LogYJL, Warning, TEXT("%s"), *Msg);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, Msg);
    }
}

void AYJL_GodotLevelHost::BuildFromTscn()
{
    ClearGeneratedChildren();

    FString ResolvedPath = ResolveTscnFilePath();
    if (ResolvedPath.IsEmpty())
    {
        return;
    }

    auto ScreenLog = [this](const FColor& Color, const FString& S)
    {
        UE_LOG(LogYJL, Log, TEXT("[GodotLoader] %s"), *S);
        if (bScreenLog && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 6.0f, Color, FString::Printf(TEXT("[GodotLoader] %s"), *S));
        }
    };

    const double StartT = FPlatformTime::Seconds();

    TArray<FTscnBlock> Blocks;
    FString Err;
    if (!LoadTscnBlocks(ResolvedPath, Blocks, Err))
    {
        ScreenLog(FColor::Red, Err);
        return;
    }

    // 加载基础 Mesh（缓存到本地静态，避免每次拖动重新查）
    static UStaticMesh* CubeMesh = nullptr;
    static UStaticMesh* SphereMesh = nullptr;
    static UStaticMesh* CylinderMesh = nullptr;
    if (!CubeMesh)     CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!SphereMesh)   SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!CylinderMesh) CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    UMaterialInterface* FallbackMat = FallbackMaterial;
    if (!FallbackMat)
    {
        FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }

    // 先扫一遍 ext_resource，建一个 id → 类型信息表，用于解析 instance / material 引用
    struct FExtResInfo { FString Type; FString Path; };
    TMap<FString, FExtResInfo> ExtRes;

    // 也扫一遍 sub_resource，缓存它的 id → type 类型
    TMap<FString, FString> SubResType;

    for (const FTscnBlock& B : Blocks)
    {
        if (B.HeaderType == TEXT("ext_resource"))
        {
            FExtResInfo Info;
            if (const FString* T = B.HeaderAttrs.Find(TEXT("type"))) Info.Type = *T;
            if (const FString* P = B.HeaderAttrs.Find(TEXT("path"))) Info.Path = *P;
            FString Id;
            if (const FString* I = B.HeaderAttrs.Find(TEXT("id"))) Id = *I;
            if (!Id.IsEmpty()) ExtRes.Add(Id, Info);
        }
        else if (B.HeaderType == TEXT("sub_resource"))
        {
            FString Id, Type;
            if (const FString* I = B.HeaderAttrs.Find(TEXT("id"))) Id = *I;
            if (const FString* T = B.HeaderAttrs.Find(TEXT("type"))) Type = *T;
            if (!Id.IsEmpty()) SubResType.Add(Id, Type);
        }
    }

    // 处理 [node ...] 块
    TArray<FTscnNode> Nodes;
    for (const FTscnBlock& B : Blocks)
    {
        if (B.HeaderType != TEXT("node")) continue;

        FTscnNode N;
        if (const FString* Name = B.HeaderAttrs.Find(TEXT("name"))) N.Name = *Name;
        if (const FString* Type = B.HeaderAttrs.Find(TEXT("type"))) N.Type = *Type;
        if (const FString* Parent = B.HeaderAttrs.Find(TEXT("parent"))) N.ParentPath = *Parent;
        if (const FString* Inst = B.HeaderAttrs.Find(TEXT("instance")))
        {
            N.bIsInstance = true;
            if (N.Type.IsEmpty()) N.Type = TEXT("(Instance)");
        }
        if (const FString* TR = B.Props.Find(TEXT("transform"))) N.TransformRaw = *TR;
        if (B.Props.Contains(TEXT("script"))) N.bHasScript = true;
        N.Props = B.Props;
        Nodes.Add(N);
    }

    // 维护 PathToComp：节点完整路径 → 对应 UE component（用作子节点的 attach 父）
    // 根节点 "."  → GeneratedRoot
    TMap<FString, USceneComponent*> PathToComp;
    PathToComp.Add(TEXT("."), GeneratedRoot);

    int32 Spawned = 0;
    int32 Skipped = 0;

    // 按文件出现顺序处理（Godot 写 .tscn 时父总是先于子写入，所以一遍过就行）
    for (const FTscnNode& N : Nodes)
    {
        // 找父
        USceneComponent* ParentComp = nullptr;
        if (USceneComponent** P = PathToComp.Find(N.ParentPath))
        {
            ParentComp = *P;
        }
        else
        {
            // 父没找到 → fallback 到 GeneratedRoot，记日志
            ParentComp = GeneratedRoot;
            UE_LOG(LogYJL, Verbose, TEXT("[GodotLoader] %s 的父 '%s' 未找到，回退到 GeneratedRoot"), *N.Name, *N.ParentPath);
        }

        // 解析 transform（如果有）
        FTransform Xform;
        bool bHasXform = false;
        if (!N.TransformRaw.IsEmpty())
        {
            bHasXform = ParseGodotTransform3D(N.TransformRaw, UnitScale, Xform);
        }

        // 完整路径：父路径 + "/" + name（或者就是根）
        const FString FullPath = (N.ParentPath == TEXT(".")) ? N.Name : (N.ParentPath + TEXT("/") + N.Name);

        // 决定这个节点要做什么 —— 显式跳过的类型黑名单
        // 这些节点 UE 端用别的方式处理（BP / 引擎天空雾 / Kill Z）：
        //   - instance / script：子场景实例（你已在 UE 端 BP 实现）
        //   - Label3D：教程文字（UE 端可用 UMG/3D Widget 替代）
        //   - WorldEnvironment：天空/雾（UE 端用 Sky Atmosphere）
        //   - RigidBody3D / StaticBody3D / Area3D / CollisionShape3D：纯功能体（无视觉）
        //
        // 关键修复：跳过节点【不创建任何 UE 组件】，把它们的 path 直接映射到父组件。
        // 这样它们的子节点会自动挂到祖父，不会留下空 SceneComponent 残留。
        const bool bExplicitSkip =
            N.bIsInstance || N.bHasScript ||
            N.Type == TEXT("Label3D") ||
            N.Type == TEXT("WorldEnvironment") ||
            N.Type == TEXT("RigidBody3D") ||
            N.Type == TEXT("StaticBody3D") ||
            N.Type == TEXT("Area3D") ||
            N.Type == TEXT("CollisionShape3D") ||
            N.Type == TEXT("AudioStreamPlayer3D") ||
            N.Type == TEXT("Camera3D");

        if (bExplicitSkip)
        {
            if (bHasXform)
            {
                // 有 transform 的跳过节点（如 RigidBody3D 容器）：创建空 SceneComponent
                // 占位，但绝对没有任何视觉，仅保留 transform 给子节点继承
                USceneComponent* Placeholder = NewObject<USceneComponent>(this, NAME_None);
                Placeholder->SetupAttachment(ParentComp);
                Placeholder->RegisterComponent();
                Placeholder->SetRelativeTransform(Xform);
                PathToComp.Add(FullPath, Placeholder);
            }
            else
            {
                // 没 transform 的跳过节点（Label3D / WorldEnvironment / 纯逻辑节点）：
                // path 直接指向父组件，子节点会"穿透"挂到祖父，不留任何残留
                PathToComp.Add(FullPath, ParentComp);
            }
            ++Skipped;
            continue;
        }

        if (N.Type == TEXT("Node3D") || N.Type == TEXT("Node"))
        {
            // 仅作为容器
            USceneComponent* Group = NewObject<USceneComponent>(this, NAME_None);
            Group->SetupAttachment(ParentComp);
            Group->RegisterComponent();
            if (bHasXform) Group->SetRelativeTransform(Xform);
            PathToComp.Add(FullPath, Group);
            continue;
        }

        if (N.Type == TEXT("DirectionalLight3D"))
        {
            UDirectionalLightComponent* L = NewObject<UDirectionalLightComponent>(this, NAME_None);
            L->SetupAttachment(ParentComp);
            L->RegisterComponent();
            if (bHasXform) L->SetRelativeTransform(Xform);
            if (const FString* E = N.Props.Find(TEXT("light_energy")))
            {
                L->SetIntensity(FCString::Atof(**E) * 3.14f); // Godot energy ≈ UE intensity 数量级换算近似
            }
            PathToComp.Add(FullPath, L);
            ++Spawned;
            continue;
        }

        if (N.Type == TEXT("OmniLight3D"))
        {
            UPointLightComponent* L = NewObject<UPointLightComponent>(this, NAME_None);
            L->SetupAttachment(ParentComp);
            L->RegisterComponent();
            if (bHasXform) L->SetRelativeTransform(Xform);
            PathToComp.Add(FullPath, L);
            ++Spawned;
            continue;
        }

        // ===== CSG 几何 / MeshInstance3D：生成 StaticMeshComponent =====
        UStaticMesh* UseMesh = nullptr;
        FVector ExtraScale(1.0f, 1.0f, 1.0f);  // 在 transform.scale 基础上再叠加（用于尺寸属性）

        if (N.Type == TEXT("CSGBox3D"))
        {
            UseMesh = CubeMesh;
            // size = Vector3(w, h, d) —— Godot 的 size 直接表示米
            FVector Size(1, 1, 1);
            if (const FString* S = N.Props.Find(TEXT("size"))) ParseVec3(*S, Size);
            // Godot Box 默认就是按 size 渲染的，对应 UE Cube（默认 100cm 立方）
            // UE Cube 边长 = 100cm = 1m，所以缩放就是 size（米数）
            // Godot 轴 X右 / Y上 / Z前 → UE 轴 X前 / Y右 / Z上：
            //   UE.X 缩放对应 Godot.Z，UE.Y 对应 Godot.X，UE.Z 对应 Godot.Y
            ExtraScale = FVector(Size.Z, Size.X, Size.Y);
        }
        else if (N.Type == TEXT("CSGSphere3D"))
        {
            UseMesh = SphereMesh;
            // CSGSphere 默认半径 1m，UE Sphere 默认半径 50cm（直径 100cm）
            // 所以缩放 = 2 * radius（米）/ 1 = 2 * radius
            float Radius = 1.0f;
            if (const FString* R = N.Props.Find(TEXT("radius"))) Radius = FCString::Atof(**R);
            const float S = 2.0f * Radius;
            ExtraScale = FVector(S, S, S);
        }
        else if (N.Type == TEXT("CSGCylinder3D"))
        {
            UseMesh = CylinderMesh;
            // CSGCylinder 默认 radius 1m，height 1m，主轴沿 Godot Y
            // UE Cylinder 默认 radius 50cm，height 100cm，主轴沿 UE Z
            // Godot Y → UE Z（主轴对齐，OK）
            // XY 半径缩放 = 2 * radius，Z 高度缩放 = height
            float Radius = 1.0f;
            float Height = 1.0f;
            bool bCone = false;
            if (const FString* R = N.Props.Find(TEXT("radius"))) Radius = FCString::Atof(**R);
            if (const FString* H = N.Props.Find(TEXT("height"))) Height = FCString::Atof(**H);
            if (const FString* C = N.Props.Find(TEXT("cone"))) bCone = (Trim(*C) == TEXT("true"));
            // UE Cylinder X/Y 直径 100cm → 缩放 = 2 * radius
            ExtraScale = FVector(2.0f * Radius, 2.0f * Radius, Height);
            // 锥形目前用普通圆柱代替（视觉上有偏差但不影响玩法）—— 用屏字提示
            (void)bCone;
        }
        else if (N.Type == TEXT("MeshInstance3D"))
        {
            // 根据 mesh = SubResource(...) 的类型决定
            if (const FString* M = N.Props.Find(TEXT("mesh")))
            {
                FString MeshId = ParseExtResourceId(*M); // SubResource 的格式跟 ExtResource 一样写法
                if (FString* MT = SubResType.Find(MeshId))
                {
                    if (*MT == TEXT("BoxMesh")) UseMesh = CubeMesh;
                    else if (*MT == TEXT("SphereMesh")) UseMesh = SphereMesh;
                    else if (*MT == TEXT("CylinderMesh") || *MT == TEXT("CapsuleMesh")) UseMesh = CylinderMesh;
                    else UseMesh = CubeMesh; // 兜底
                }
            }
            // MeshInstance3D 没有显式 size 属性 —— 它的"大小"由父 RigidBody3D 的子 CollisionShape3D 表达
            // 但 RigidBody3D 我们已经在 instance 分支跳过了，所以这里就用 1m 单位 mesh
        }
        else
        {
            // 不认识的节点类型 → 跳过 + 屏字提醒
            UE_LOG(LogYJL, Warning, TEXT("[GodotLoader] 未支持节点类型：%s (name=%s)"), *N.Type, *N.Name);
            // 同样的策略：有 transform 才占位，没 transform 直接映射父
            if (bHasXform)
            {
                USceneComponent* Placeholder = NewObject<USceneComponent>(this, NAME_None);
                Placeholder->SetupAttachment(ParentComp);
                Placeholder->RegisterComponent();
                Placeholder->SetRelativeTransform(Xform);
                PathToComp.Add(FullPath, Placeholder);
            }
            else
            {
                PathToComp.Add(FullPath, ParentComp);
            }
            ++Skipped;
            continue;
        }

        if (!UseMesh)
        {
            ++Skipped;
            continue;
        }

        UStaticMeshComponent* SMC = NewObject<UStaticMeshComponent>(this, NAME_None);
        SMC->SetupAttachment(ParentComp);
        SMC->RegisterComponent();
        SMC->SetStaticMesh(UseMesh);
        if (bHasXform)
        {
            // 把 ExtraScale 乘到 Xform.Scale 上
            FTransform Final = Xform;
            Final.SetScale3D(Final.GetScale3D() * ExtraScale);
            SMC->SetRelativeTransform(Final);
        }
        else
        {
            SMC->SetRelativeScale3D(ExtraScale);
        }

        // 材质
        UMaterialInterface* MatToUse = FallbackMat;
        if (const FString* MatRef = N.Props.Find(TEXT("material")))
        {
            FString Id = ParseExtResourceId(*MatRef);
            if (FExtResInfo* Ext = ExtRes.Find(Id))
            {
                // 从 path 提取文件名（去 .tres 后缀），用作 MaterialMap 的 key
                FString Filename = FPaths::GetBaseFilename(Ext->Path);
                if (TObjectPtr<UMaterialInterface>* Found = MaterialMap.Find(Filename))
                {
                    if (Found->Get()) MatToUse = Found->Get();
                }
            }
        }
        if (MatToUse) SMC->SetMaterial(0, MatToUse);

        // 碰撞
        if (bEnableCollision)
        {
            SMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            SMC->SetCollisionResponseToAllChannels(ECR_Block);
            SMC->SetCollisionObjectType(ECC_WorldStatic);
        }
        else
        {
            SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        PathToComp.Add(FullPath, SMC);
        ++Spawned;
    }

    const double Elapsed = FPlatformTime::Seconds() - StartT;
    ScreenLog(FColor::Green, FString::Printf(
        TEXT("已加载 %s | 生成 %d 节点 / 跳过 %d 节点 / 耗时 %.0f ms"),
        *FPaths::GetCleanFilename(ResolvedPath), Spawned, Skipped, Elapsed * 1000.0));
}

void AYJL_GodotLevelHost::SpawnAsIndependentActors()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    auto ScreenLog = [this](const FColor& Color, const FString& S)
    {
        UE_LOG(LogYJL, Log, TEXT("[GodotLoader] [Independent] %s"), *S);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 6.0f, Color, FString::Printf(TEXT("[GodotLoader] [Independent] %s"), *S));
        }
    };

    FString ResolvedPath = ResolveTscnFilePath();
    if (ResolvedPath.IsEmpty())
    {
        ScreenLog(FColor::Red, TEXT("TscnFilePath 路径为空，无法离散生成！"));
        return;
    }

    ScreenLog(FColor::Cyan, FString::Printf(TEXT("正在离散生成独立 Actor：从 %s"), *FPaths::GetCleanFilename(ResolvedPath)));

    TArray<FTscnBlock> Blocks;
    FString Err;
    if (!LoadTscnBlocks(ResolvedPath, Blocks, Err))
    {
        ScreenLog(FColor::Red, Err);
        return;
    }

    // 加载基础 Mesh
    static UStaticMesh* CubeMesh = nullptr;
    static UStaticMesh* SphereMesh = nullptr;
    static UStaticMesh* CylinderMesh = nullptr;
    if (!CubeMesh)     CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!SphereMesh)   SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!CylinderMesh) CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    UMaterialInterface* FallbackMat = FallbackMaterial;
    if (!FallbackMat)
    {
        FallbackMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }

    // 扫一遍 ext_resource
    struct FExtResInfo { FString Type; FString Path; };
    TMap<FString, FExtResInfo> ExtRes;
    TMap<FString, FString> SubResType;

    for (const FTscnBlock& B : Blocks)
    {
        if (B.HeaderType == TEXT("ext_resource"))
        {
            FExtResInfo Info;
            if (const FString* T = B.HeaderAttrs.Find(TEXT("type"))) Info.Type = *T;
            if (const FString* P = B.HeaderAttrs.Find(TEXT("path"))) Info.Path = *P;
            FString Id;
            if (const FString* I = B.HeaderAttrs.Find(TEXT("id"))) Id = *I;
            if (!Id.IsEmpty()) ExtRes.Add(Id, Info);
        }
        else if (B.HeaderType == TEXT("sub_resource"))
        {
            FString Id, Type;
            if (const FString* I = B.HeaderAttrs.Find(TEXT("id"))) Id = *I;
            if (const FString* T = B.HeaderAttrs.Find(TEXT("type"))) Type = *T;
            if (!Id.IsEmpty()) SubResType.Add(Id, Type);
        }
    }

    // 处理 [node ...] 块
    TArray<FTscnNode> Nodes;
    for (const FTscnBlock& B : Blocks)
    {
        if (B.HeaderType != TEXT("node")) continue;

        FTscnNode N;
        if (const FString* Name = B.HeaderAttrs.Find(TEXT("name"))) N.Name = *Name;
        if (const FString* Type = B.HeaderAttrs.Find(TEXT("type"))) N.Type = *Type;
        if (const FString* Parent = B.HeaderAttrs.Find(TEXT("parent"))) N.ParentPath = *Parent;
        if (const FString* Inst = B.HeaderAttrs.Find(TEXT("instance")))
        {
            N.bIsInstance = true;
            if (N.Type.IsEmpty()) N.Type = TEXT("(Instance)");
        }
        if (const FString* TR = B.Props.Find(TEXT("transform"))) N.TransformRaw = *TR;
        if (B.Props.Contains(TEXT("script"))) N.bHasScript = true;
        N.Props = B.Props;
        Nodes.Add(N);
    }

    // 维护绝对世界 Transform
    TMap<FString, FTransform> PathToWorldXform;
    PathToWorldXform.Add(TEXT("."), this->GetActorTransform());

    int32 Spawned = 0;
    int32 Skipped = 0;

    const double StartT = FPlatformTime::Seconds();

    for (const FTscnNode& N : Nodes)
    {
        // 计算父世界 Xform
        FTransform ParentWorldXform = this->GetActorTransform();
        if (FTransform* PX = PathToWorldXform.Find(N.ParentPath))
        {
            ParentWorldXform = *PX;
        }

        // 解析本地相对 transform
        FTransform LocalXform = FTransform::Identity;
        bool bHasXform = false;
        if (!N.TransformRaw.IsEmpty())
        {
            bHasXform = ParseGodotTransform3D(N.TransformRaw, UnitScale, LocalXform);
        }

        // 节点的绝对世界 Transform：LocalXform 叠在 ParentWorldXform 上
        FTransform NodeWorldXform = bHasXform ? (LocalXform * ParentWorldXform) : ParentWorldXform;

        // 完整路径：父路径 + "/" + name
        const FString FullPath = (N.ParentPath == TEXT(".")) ? N.Name : (N.ParentPath + TEXT("/") + N.Name);

        // 存储本节点的世界 Xform，方便子节点级联乘
        PathToWorldXform.Add(FullPath, NodeWorldXform);

        // 显式跳过
        const bool bExplicitSkip =
            N.bIsInstance || N.bHasScript ||
            N.Type == TEXT("Label3D") ||
            N.Type == TEXT("WorldEnvironment") ||
            N.Type == TEXT("RigidBody3D") ||
            N.Type == TEXT("StaticBody3D") ||
            N.Type == TEXT("Area3D") ||
            N.Type == TEXT("CollisionShape3D") ||
            N.Type == TEXT("AudioStreamPlayer3D") ||
            N.Type == TEXT("Camera3D");

        if (bExplicitSkip)
        {
            ++Skipped;
            continue;
        }

        // ===== 1. 处理光源 =====
        if (N.Type == TEXT("DirectionalLight3D"))
        {
            ADirectionalLight* DL = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), NodeWorldXform.GetLocation(), NodeWorldXform.GetRotation().Rotator());
            if (DL)
            {
                if (const FString* E = N.Props.Find(TEXT("light_energy")))
                {
                    DL->GetLightComponent()->SetIntensity(FCString::Atof(**E) * 3.14f);
                }
#if WITH_EDITOR
                DL->SetActorLabel(N.Name);
#endif
                DL->Tags.Add(FName("YJL_Imported"));
                ++Spawned;
            }
            continue;
        }

        if (N.Type == TEXT("OmniLight3D"))
        {
            APointLight* PL = World->SpawnActor<APointLight>(APointLight::StaticClass(), NodeWorldXform.GetLocation(), NodeWorldXform.GetRotation().Rotator());
            if (PL)
            {
#if WITH_EDITOR
                PL->SetActorLabel(N.Name);
#endif
                PL->Tags.Add(FName("YJL_Imported"));
                ++Spawned;
            }
            continue;
        }

        // ===== 2. 处理 CSG 几何 / MeshInstance3D =====
        UStaticMesh* UseMesh = nullptr;
        FVector ExtraScale(1.0f, 1.0f, 1.0f);

        if (N.Type == TEXT("CSGBox3D"))
        {
            UseMesh = CubeMesh;
            FVector Size(1, 1, 1);
            if (const FString* S = N.Props.Find(TEXT("size"))) ParseVec3(*S, Size);
            ExtraScale = FVector(Size.Z, Size.X, Size.Y);
        }
        else if (N.Type == TEXT("CSGSphere3D"))
        {
            UseMesh = SphereMesh;
            float Radius = 1.0f;
            if (const FString* R = N.Props.Find(TEXT("radius"))) Radius = FCString::Atof(**R);
            const float S = 2.0f * Radius;
            ExtraScale = FVector(S, S, S);
        }
        else if (N.Type == TEXT("CSGCylinder3D"))
        {
            UseMesh = CylinderMesh;
            float Radius = 1.0f;
            float Height = 1.0f;
            if (const FString* R = N.Props.Find(TEXT("radius"))) Radius = FCString::Atof(**R);
            if (const FString* H = N.Props.Find(TEXT("height"))) Height = FCString::Atof(**H);
            ExtraScale = FVector(2.0f * Radius, 2.0f * Radius, Height);
        }
        else if (N.Type == TEXT("MeshInstance3D"))
        {
            if (const FString* M = N.Props.Find(TEXT("mesh")))
            {
                FString MeshId = ParseExtResourceId(*M);
                if (FString* MT = SubResType.Find(MeshId))
                {
                    if (*MT == TEXT("BoxMesh")) UseMesh = CubeMesh;
                    else if (*MT == TEXT("SphereMesh")) UseMesh = SphereMesh;
                    else if (*MT == TEXT("CylinderMesh") || *MT == TEXT("CapsuleMesh")) UseMesh = CylinderMesh;
                    else UseMesh = CubeMesh;
                }
            }
        }

        if (!UseMesh)
        {
            ++Skipped;
            continue;
        }

        // 最终的网格 absolute 世界 transform 乘上 ExtraScale
        FTransform FinalMeshWorldXform = NodeWorldXform;
        FinalMeshWorldXform.SetScale3D(FinalMeshWorldXform.GetScale3D() * ExtraScale);

        AStaticMeshActor* SMA = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FinalMeshWorldXform.GetLocation(), FinalMeshWorldXform.GetRotation().Rotator());
        if (SMA)
        {
            SMA->SetActorScale3D(FinalMeshWorldXform.GetScale3D());
            
            UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
            if (SMC)
            {
                SMC->SetStaticMesh(UseMesh);

                // 材质
                UMaterialInterface* MatToUse = FallbackMat;
                if (const FString* MatRef = N.Props.Find(TEXT("material")))
                {
                    FString Id = ParseExtResourceId(*MatRef);
                    if (FExtResInfo* Ext = ExtRes.Find(Id))
                    {
                        FString Filename = FPaths::GetBaseFilename(Ext->Path);
                        if (TObjectPtr<UMaterialInterface>* Found = MaterialMap.Find(Filename))
                        {
                            if (Found->Get()) MatToUse = Found->Get();
                        }
                    }
                }
                if (MatToUse) SMC->SetMaterial(0, MatToUse);

                // 物理与碰撞
                if (bEnableCollision)
                {
                    SMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    SMC->SetCollisionResponseToAllChannels(ECR_Block);
                    SMC->SetCollisionObjectType(ECC_WorldStatic);
                }
                else
                {
                    SMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                }
            }

#if WITH_EDITOR
            SMA->SetActorLabel(N.Name);
#endif
            SMA->Tags.Add(FName("YJL_Imported"));
            ++Spawned;
        }
    }

    const double Elapsed = FPlatformTime::Seconds() - StartT;
    ScreenLog(FColor::Green, FString::Printf(
        TEXT("【离散生成成功】已生成 %d 个独立的 Actor 到关卡 / 跳过 %d 节点 / 耗时 %.0f ms！"),
        Spawned, Skipped, Elapsed * 1000.0));
}

FString AYJL_GodotLevelHost::ResolveTscnFilePath() const
{
    if (TscnFilePath.IsEmpty())
    {
        return TscnFilePath;
    }

    // 1. 如果本身就是个存在的绝对路径文件，直接返回
    if (FPaths::FileExists(TscnFilePath))
    {
        return TscnFilePath;
    }

    // 2. 如果是相对路径，尝试同项目根目录拼接
    FString PathFromProject = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TscnFilePath);
    if (FPaths::FileExists(PathFromProject))
    {
        return PathFromProject;
    }

    // 3. 如果绝对路径不存在（例如 Windows 队友在不同盘符下），自动回退到同名文件查找 Content/YJL/GodotScene/<filename>
    FString Filename = FPaths::GetCleanFilename(TscnFilePath);
    FString FallbackPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("YJL/GodotScene") / Filename);
    if (FPaths::FileExists(FallbackPath))
    {
        return FallbackPath;
    }

    // 兜底返回原路径
    return TscnFilePath;
}
