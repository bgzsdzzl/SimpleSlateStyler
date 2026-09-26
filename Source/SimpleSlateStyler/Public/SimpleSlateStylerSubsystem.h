

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "SimpleSlateStylerParser.h"
#include "SimpleSlateStylerSubsystem.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * Central editor subsystem that turns parsed .SimpleSlateStyler entries into
 * real Slate brushes registered into FAppStyle.
 *
 * Other plugins can:
 *   - drop their own *.SimpleSlateStyler files under Config/SimpleSlateStyler/
 *   - or call the BlueprintCallable API directly at runtime.
 */
UCLASS()
class SIMPLESLATESTYLER_API USimpleSlateStylerSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // USimpleSlateStylerSubsystem.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------
    // Public registration API (C++ and Blueprint).
    // ------------------------------------------------------------------

    /** Register a texture as a Slate brush under StyleName. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void RegisterTextureBrush(
        FName StyleName,
        UTexture2D* Texture,
        FVector2D Size = FVector2D(64.0, 64.0));

    /**
     * Register a material as a Slate brush under StyleName.
     * Returns the dynamic material instance so callers can drive parameters.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    UMaterialInstanceDynamic* RegisterMaterialBrush(
        FName StyleName,
        UMaterialInterface* Material,
        FVector2D Size = FVector2D(64.0, 64.0));

    /** Set a scalar parameter on an already-registered material brush. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void SetMaterialScalar(FName StyleName, FName ParamName, float Value);

    /** Set a vector parameter on an already-registered material brush. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void SetMaterialVector(FName StyleName, FName ParamName, FLinearColor Value);

    /** Return the dynamic material instance for a style name, or nullptr. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    UMaterialInstanceDynamic* GetMaterialInstance(FName StyleName) const;

    /** Invalidate all Slate widgets so the new brushes take effect immediately. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void InvalidateAll();

    // ------------------------------------------------------------------
    // DSL loading.
    // ------------------------------------------------------------------

    /** Parse a single .SimpleSlateStyler file and apply it. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    bool LoadFile(const FString& FilePath);

    /**
     * Discover and reload every *.SimpleSlateStyler file located under
     *   1. <Project>/Config/SimpleSlateStyler/
     *   2. <AnyPlugin>/Config/SimpleSlateStyler/
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void ReloadAll();

    /** Log every currently registered style name to the output log. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void DumpRegisteredStyles() const;

    // ------------------------------------------------------------------
// Scope isolation API.
//
// A "scope" is just the first segment of a style key before the first dot.
// "MyPlugin.PanelBg" belongs to scope "MyPlugin".
//
// Styles registered without a dot have no scope and are not listed by
// GetScopeNames / GetStylesInScope, but they still live in the registry.
// ------------------------------------------------------------------

/** All distinct scope names currently registered. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    TArray<FName> GetScopeNames() const;

    /** All style keys registered under the given scope. */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    TArray<FName> GetStylesInScope(FName ScopeName) const;

    /**
     * Unregister every style whose key starts with "<ScopeName>.".
     *
     * The style entries remain in FAppStyle until the editor restarts (Slate
     * has no per-key removal API), but the registry stops reporting them, so
     * FindBrush will no longer resolve them.
     */
    UFUNCTION(BlueprintCallable, Category = "SimpleSlateStyler")
    void RemoveScope(FName ScopeName);

    /**
     * Scope-aware lookup.
     *
     * Search order:
     *   1. "<PreferredScope>.<StyleName>" if it is registered here.
     *   2. "<StyleName>" if it is registered here.
     *   3. FAppStyle::Get().GetBrush("<StyleName>") -- only when no
     *      PreferredScope was supplied.
     *
     * Returns nullptr when PreferredScope was supplied but nothing matched.
     * Otherwise always returns a valid brush pointer (possibly the Slate
     * default brush).
     *
     * Not exposed to Blueprint because FSlateBrush is not a USTRUCT.
     */
    const FSlateBrush* FindBrush(FName StyleName, FName PreferredScope = NAME_None) const;
private:
    void ApplyEntry(const FSimpleSlateStylerEntry& Entry);
    void SetBrushInternal(FName StyleName, const FSlateBrush& Brush);

    /** Style names registered by this subsystem, in load order. */
    UPROPERTY()
    TArray<FName> RegisteredNames;

    /** Dynamic material instances created for material brushes. */
    UPROPERTY()
    TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> MaterialInstances;
};