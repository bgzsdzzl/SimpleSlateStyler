

#include "SimpleSlateStylerSubsystem.h"

#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimpleSlateStyler, Log, All);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void USimpleSlateStylerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Defer the very first load until the editor is fully up. Editor subsystems
    // are initialized during editor startup, but FAppStyle and the asset registry
    // are safest to touch a moment later.
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
            {
                ReloadAll();
                return false; // one-shot
            }),
        0.2f);
}

void USimpleSlateStylerSubsystem::Deinitialize()
{
    MaterialInstances.Reset();
    RegisteredNames.Reset();
    Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void USimpleSlateStylerSubsystem::RegisterTextureBrush(
    FName StyleName,
    UTexture2D* Texture,
    FVector2D Size)
{
    if (!Texture)
    {
        UE_LOG(LogSimpleSlateStyler, Warning,
            TEXT("RegisterTextureBrush: null texture for '%s'."), *StyleName.ToString());
        return;
    }

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = Size;
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.TintColor = FSlateColor(FLinearColor::White);

    SetBrushInternal(StyleName, Brush);
}

UMaterialInstanceDynamic* USimpleSlateStylerSubsystem::RegisterMaterialBrush(
    FName StyleName,
    UMaterialInterface* Material,
    FVector2D Size)
{
    if (!Material)
    {
        UE_LOG(LogSimpleSlateStyler, Warning,
            TEXT("RegisterMaterialBrush: null material for '%s'."), *StyleName.ToString());
        return nullptr;
    }

    UMaterialInstanceDynamic* MID =
        UMaterialInstanceDynamic::Create(Material, this);

    MaterialInstances.Add(StyleName, MID);

    FSlateBrush Brush;
    Brush.SetResourceObject(MID);
    Brush.ImageSize = Size;
    Brush.DrawAs = ESlateBrushDrawType::Image;
    Brush.TintColor = FSlateColor(FLinearColor::White);

    SetBrushInternal(StyleName, Brush);
    return MID;
}

void USimpleSlateStylerSubsystem::SetMaterialScalar(
    FName StyleName, FName ParamName, float Value)
{
    if (TObjectPtr<UMaterialInstanceDynamic>* Found = MaterialInstances.Find(StyleName))
    {
        if (*Found)
        {
            (*Found)->SetScalarParameterValue(ParamName, Value);
            InvalidateAll();
        }
    }
}

void USimpleSlateStylerSubsystem::SetMaterialVector(
    FName StyleName, FName ParamName, FLinearColor Value)
{
    if (TObjectPtr<UMaterialInstanceDynamic>* Found = MaterialInstances.Find(StyleName))
    {
        if (*Found)
        {
            (*Found)->SetVectorParameterValue(ParamName, Value);
            InvalidateAll();
        }
    }
}

UMaterialInstanceDynamic* USimpleSlateStylerSubsystem::GetMaterialInstance(FName StyleName) const
{
    if (const TObjectPtr<UMaterialInstanceDynamic>* Found = MaterialInstances.Find(StyleName))
    {
        return Found->Get();
    }
    return nullptr;
}

void USimpleSlateStylerSubsystem::InvalidateAll()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().InvalidateAllWidgets(false);
    }
}

// ---------------------------------------------------------------------------
// DSL loading
// ---------------------------------------------------------------------------

bool USimpleSlateStylerSubsystem::LoadFile(const FString& FilePath)
{
    TArray<FSimpleSlateStylerEntry> Entries;
    TArray<FString> Errors;

    if (!FSimpleSlateStylerParser::ParseFile(FilePath, Entries, Errors))
    {
        for (const FString& Error : Errors)
        {
            UE_LOG(LogSimpleSlateStyler, Error, TEXT("%s: %s"), *FilePath, *Error);
        }
        return false;
    }

    UE_LOG(LogSimpleSlateStyler, Log,
        TEXT("Loaded %d entries from %s"), Entries.Num(), *FilePath);

    for (const FSimpleSlateStylerEntry& Entry : Entries)
    {
        ApplyEntry(Entry);
    }
    return true;
}

void USimpleSlateStylerSubsystem::ReloadAll()
{
    TArray<FString> Files;

    // 1. Project-level config.
    {
        const FString ProjectDir =
            FPaths::ProjectConfigDir() / TEXT("SimpleSlateStyler");
        IFileManager::Get().FindFilesRecursive(
            Files, *ProjectDir, TEXT("*.SimpleSlateStyler"),
            /*Files=*/true, /*Directories=*/false);
    }

    // 2. Every discovered plugin's own config folder.
    {
        const TArray<TSharedRef<IPlugin>> Plugins =
            IPluginManager::Get().GetDiscoveredPlugins();
        for (const TSharedRef<IPlugin>& Plugin : Plugins)
        {
            const FString PluginDir =
                FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config/SimpleSlateStyler"));
            IFileManager::Get().FindFilesRecursive(
                Files, *PluginDir, TEXT("*.SimpleSlateStyler"),
                /*Files=*/true, /*Directories=*/false);
        }
    }

    UE_LOG(LogSimpleSlateStyler, Log,
        TEXT("Discovered %d .SimpleSlateStyler file(s)."), Files.Num());

    for (const FString& File : Files)
    {
        LoadFile(File);
    }

    InvalidateAll();
}

void USimpleSlateStylerSubsystem::DumpRegisteredStyles() const
{
    UE_LOG(LogSimpleSlateStyler, Log,
        TEXT("=== Simple Slate Styler: %d registered style(s) ==="),
        RegisteredNames.Num());

    // Group by scope for readability.
    TMap<FName, TArray<FName>> Grouped;
    TArray<FName> NoScope;

    for (const FName& StyleName : RegisteredNames)
    {
        const FString NameStr = StyleName.ToString();
        int32 DotIndex = INDEX_NONE;
        if (NameStr.FindChar(TEXT('.'), DotIndex) && DotIndex > 0)
        {
            Grouped.FindOrAdd(FName(*NameStr.Left(DotIndex))).Add(StyleName);
        }
        else
        {
            NoScope.Add(StyleName);
        }
    }

    for (const TPair<FName, TArray<FName>>& Pair : Grouped)
    {
        UE_LOG(LogSimpleSlateStyler, Log, TEXT("  [Scope: %s]"), *Pair.Key.ToString());
        for (const FName& StyleName : Pair.Value)
        {
            const bool bIsMaterial = MaterialInstances.Contains(StyleName);
            UE_LOG(LogSimpleSlateStyler, Log,
                TEXT("    %s  (%s)"),
                *StyleName.ToString(),
                bIsMaterial ? TEXT("Material") : TEXT("Texture"));
        }
    }

    if (NoScope.Num() > 0)
    {
        UE_LOG(LogSimpleSlateStyler, Log, TEXT("  [Scope: <none>]"));
        for (const FName& StyleName : NoScope)
        {
            const bool bIsMaterial = MaterialInstances.Contains(StyleName);
            UE_LOG(LogSimpleSlateStyler, Log,
                TEXT("    %s  (%s)"),
                *StyleName.ToString(),
                bIsMaterial ? TEXT("Material") : TEXT("Texture"));
        }
    }
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------

void USimpleSlateStylerSubsystem::ApplyEntry(const FSimpleSlateStylerEntry& Entry)
{
    FString Type = Entry.Type.ToLower();

    // Auto-detect: paths containing "/M_" or "_M." are treated as materials.
    if (Type.IsEmpty())
    {
        const bool bLooksLikeMaterial =
            Entry.Resource.Contains(TEXT("/M_")) ||
            Entry.Resource.Contains(TEXT("_M."));
        Type = bLooksLikeMaterial ? TEXT("material") : TEXT("texture");
    }

    if (Type == TEXT("material"))
    {
        UMaterialInterface* Material =
            LoadObject<UMaterialInterface>(nullptr, *Entry.Resource);
        if (!Material)
        {
            UE_LOG(LogSimpleSlateStyler, Warning,
                TEXT("Style '%s': cannot load material '%s'."),
                *Entry.Name.ToString(), *Entry.Resource);
            return;
        }

        UMaterialInstanceDynamic* MID =
            RegisterMaterialBrush(Entry.Name, Material, Entry.Size);
        if (!MID)
        {
            return;
        }

        for (const TPair<FName, float>& Pair : Entry.ScalarParams)
        {
            MID->SetScalarParameterValue(Pair.Key, Pair.Value);
        }
        for (const TPair<FName, FLinearColor>& Pair : Entry.VectorParams)
        {
            MID->SetVectorParameterValue(Pair.Key, Pair.Value);
        }
    }
    else if (Type == TEXT("texture"))
    {
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Entry.Resource);
        if (!Texture)
        {
            UE_LOG(LogSimpleSlateStyler, Warning,
                TEXT("Style '%s': cannot load texture '%s'."),
                *Entry.Name.ToString(), *Entry.Resource);
            return;
        }
        RegisterTextureBrush(Entry.Name, Texture, Entry.Size);
    }
    else
    {
        UE_LOG(LogSimpleSlateStyler, Warning,
            TEXT("Style '%s': unknown type '%s'."),
            *Entry.Name.ToString(), *Entry.Type);
    }
}

void USimpleSlateStylerSubsystem::SetBrushInternal(
    FName StyleName, const FSlateBrush& Brush)
{
    // FAppStyle::Get() returns a const reference, but internally it is a
    // FSlateStyleSet. Casting away const is the documented way to hot-patch
    // the application style at runtime.
    FSlateStyleSet& AppStyle =
        const_cast<FSlateStyleSet&>(static_cast<const FSlateStyleSet&>(FAppStyle::Get()));

    // FSlateStyleSet stores brushes as pointers. Allocate a new brush on the heap
    // so that the style set can take ownership and manage its lifetime.
    FSlateBrush* NewBrush = new FSlateBrush(Brush);
    AppStyle.Set(StyleName, NewBrush);

    if (!RegisteredNames.Contains(StyleName))
    {
        RegisteredNames.Add(StyleName);
    }
}

// ---------------------------------------------------------------------------
// Scope isolation
// ---------------------------------------------------------------------------

TArray<FName> USimpleSlateStylerSubsystem::GetScopeNames() const
{
    TSet<FName> UniqueScopes;

    for (const FName& StyleName : RegisteredNames)
    {
        const FString NameStr = StyleName.ToString();
        int32 DotIndex = INDEX_NONE;
        if (NameStr.FindChar(TEXT('.'), DotIndex) && DotIndex > 0)
        {
            UniqueScopes.Add(FName(*NameStr.Left(DotIndex)));
        }
    }

    return UniqueScopes.Array();
}

TArray<FName> USimpleSlateStylerSubsystem::GetStylesInScope(FName ScopeName) const
{
    TArray<FName> Result;
    if (ScopeName.IsNone())
    {
        return Result;
    }

    const FString Prefix = ScopeName.ToString() + TEXT(".");
    for (const FName& StyleName : RegisteredNames)
    {
        if (StyleName.ToString().StartsWith(Prefix))
        {
            Result.Add(StyleName);
        }
    }
    return Result;
}

void USimpleSlateStylerSubsystem::RemoveScope(FName ScopeName)
{
    if (ScopeName.IsNone())
    {
        return;
    }

    const FString Prefix = ScopeName.ToString() + TEXT(".");
    TArray<FName> ToRemove;

    for (const FName& StyleName : RegisteredNames)
    {
        if (StyleName.ToString().StartsWith(Prefix))
        {
            ToRemove.Add(StyleName);
        }
    }

    for (const FName& StyleName : ToRemove)
    {
        RegisteredNames.Remove(StyleName);
        MaterialInstances.Remove(StyleName);

        // Note: we deliberately do NOT touch FAppStyle here. Slate has no
        // per-key removal API. The stale brush stays in FAppStyle until the
        // editor restarts, but the registry stops reporting it, so FindBrush
        // and this subsystem's queries will not see it anymore.
        UE_LOG(LogSimpleSlateStyler, Log,
            TEXT("RemoveScope: unregistered '%s'."), *StyleName.ToString());
    }

    InvalidateAll();
}

const FSlateBrush* USimpleSlateStylerSubsystem::FindBrush(
    FName StyleName, FName PreferredScope) const
{
    // 1. "<PreferredScope>.<StyleName>" when it was actually registered here.
    if (!PreferredScope.IsNone())
    {
        const FName QualifiedName(
            *(PreferredScope.ToString() + TEXT(".") + StyleName.ToString()));

        if (RegisteredNames.Contains(QualifiedName))
        {
            return FAppStyle::Get().GetBrush(QualifiedName);
        }
    }

    // 2. "<StyleName>" as-is, but only if we registered it.
    if (RegisteredNames.Contains(StyleName))
    {
        return FAppStyle::Get().GetBrush(StyleName);
    }

    // 3. If the caller asked for a scope and we did not find it, report
    //    a miss rather than silently falling back to a global style.
    if (!PreferredScope.IsNone())
    {
        return nullptr;
    }

    // 4. Fall back to whatever FAppStyle has.
    return FAppStyle::Get().GetBrush(StyleName);
}