\# C++ API Reference



All public functionality is exposed through

`USimpleSlateStylerSubsystem`, an editor subsystem you can obtain from

`GEditor`.



```cpp

\#include "SimpleSlateStylerSubsystem.h"

\#include "Editor.h"



USimpleSlateStylerSubsystem\* Sub =

&#x20;   GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>();

```



Every method listed below is also callable from Blueprint unless explicitly

marked otherwise.



\## Brush Registration



\### `RegisterTextureBrush`



```cpp

void RegisterTextureBrush(

&#x20;   FName StyleName,

&#x20;   UTexture2D\* Texture,

&#x20;   FVector2D Size = FVector2D(64.0, 64.0));

```



Registers a texture as a Slate brush under `StyleName`. The brush is

written into `FAppStyle` immediately and available via

`FAppStyle::Get().GetBrush(StyleName)`.



\### `RegisterMaterialBrush`



```cpp

UMaterialInstanceDynamic\* RegisterMaterialBrush(

&#x20;   FName StyleName,

&#x20;   UMaterialInterface\* Material,

&#x20;   FVector2D Size = FVector2D(64.0, 64.0));

```



Registers a material as a Slate brush and returns a dynamically created

`UMaterialInstanceDynamic`. Use the returned instance to drive parameters

at runtime.



The returned instance is owned by the subsystem and kept alive until the

subsystem is deinitialized or the style is removed with `RemoveScope`.



\## Material Parameters



\### `SetMaterialScalar`



```cpp

void SetMaterialScalar(FName StyleName, FName ParamName, float Value);

```



Sets a scalar parameter on a previously registered material brush and

invalidates Slate so the change is visible on the next frame.



\### `SetMaterialVector`



```cpp

void SetMaterialVector(FName StyleName, FName ParamName, FLinearColor Value);

```



Sets a vector parameter. Same invalidation behavior.



\### `GetMaterialInstance`



```cpp

UMaterialInstanceDynamic\* GetMaterialInstance(FName StyleName) const;

```



Returns the dynamic instance for a style, or `nullptr` if the style was

not registered as a material.



\## Invalidation



\### `InvalidateAll`



```cpp

void InvalidateAll();

```



Marks every Slate widget as needing a repaint. Call this after mutating

material parameters outside of `SetMaterialScalar` / `SetMaterialVector`,

or after loading new styles from a file.



\## DSL Loading



\### `LoadFile`



```cpp

bool LoadFile(const FString\& FilePath);

```



Parses a single `.SimpleSlateStyler` file and applies every entry. Returns

`false` when the file cannot be read or contains syntax errors; errors are

printed to the Output Log under the `LogSimpleSlateStyler` category.



Loading a file does \*\*not\*\* clear existing registrations; entries are

added or overwritten by name.



\### `ReloadAll`



```cpp

void ReloadAll();

```



Discovers and loads every `.SimpleSlateStyler` file under



\- `<Project>/Config/SimpleSlateStyler/`

\- `<AnyPlugin>/Config/SimpleSlateStyler/`



This is what the plugin runs on startup and after file changes.



\### `DumpRegisteredStyles`



```cpp

void DumpRegisteredStyles() const;

```



Prints every registered style to the Output Log, grouped by scope.



\## Scope Isolation



A \*\*scope\*\* is the first segment of a style key before the first dot.

`MyPlugin.PanelBg` belongs to scope `MyPlugin`. Keys without a dot belong

to no scope and are not reported by the scope query functions.



\### `GetRegisteredNames`



```cpp

const TArray<FName>\& GetRegisteredNames() const;

```



All style keys this subsystem has registered, in load order. Not exposed

to Blueprint.



\### `IsMaterialStyle`



```cpp

bool IsMaterialStyle(FName StyleName) const;

```



Returns `true` if the style was registered from a material. Not exposed to

Blueprint.



\### `GetScopeNames`



```cpp

TArray<FName> GetScopeNames() const;

```



Every distinct scope name currently registered.



\### `GetStylesInScope`



```cpp

TArray<FName> GetStylesInScope(FName ScopeName) const;

```



All style keys registered under `<ScopeName>.`.



\### `RemoveScope`



```cpp

void RemoveScope(FName ScopeName);

```



Unregisters every style whose key starts with `<ScopeName>.`.



\*\*Note:\*\* Slate has no per-key removal API. The physical brush stays inside

`FAppStyle` until the editor restarts, but the registry stops reporting it

and `FindBrush` no longer resolves it. Within a single editor session this

is sufficient for unloading a plugin's styles.



\### `FindBrush`



```cpp

const FSlateBrush\* FindBrush(

&#x20;   FName StyleName,

&#x20;   FName PreferredScope = NAME\_None) const;

```



Scope-aware lookup. Search order:



1\. `<PreferredScope>.<StyleName>` if the subsystem registered it

2\. `<StyleName>` if the subsystem registered it

3\. `FAppStyle::Get().GetBrush("<StyleName>")` when no scope was supplied



Returns `nullptr` when a scope was supplied and nothing matched. Not

exposed to Blueprint because `FSlateBrush` is not a `USTRUCT`.



\## Console Commands



Registered by the plugin module at editor startup.



| Command | Behavior |

| --- | --- |

| `SimpleSlateStyler.Reload` | Calls `ReloadAll()` |

| `SimpleSlateStyler.Dump` | Calls `DumpRegisteredStyles()` |



\## Blueprint Usage



In an editor utility widget or blueprint, get the subsystem with the

\*\*Get Editor Subsystem\*\* node and pick `SimpleSlateStylerSubsystem`.



Available Blueprint nodes:



\- `Register Texture Brush`

\- `Register Material Brush`

\- `Set Material Scalar`

\- `Set Material Vector`

\- `Get Material Instance`

\- `Invalidate All`

\- `Load File`

\- `Reload All`

\- `Dump Registered Styles`

\- `Get Scope Names`

\- `Get Styles In Scope`

\- `Remove Scope`



`FindBrush`, `GetRegisteredNames` and `IsMaterialStyle` are C++ only.



\## Full Example



```cpp

\#include "SimpleSlateStylerSubsystem.h"

\#include "Editor.h"

\#include "Materials/MaterialInterface.h"

\#include "Styling/AppStyle.h"



void ApplyStyles()

{

&#x20;   USimpleSlateStylerSubsystem\* Sub =

&#x20;       GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>();

&#x20;   if (!Sub)

&#x20;   {

&#x20;       return;

&#x20;   }



&#x20;   UMaterialInterface\* Material =

&#x20;       LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/M\_PanelBg.M\_PanelBg"));

&#x20;   if (!Material)

&#x20;   {

&#x20;       return;

&#x20;   }



&#x20;   Sub->RegisterMaterialBrush("MyPlugin.PanelBg", Material, FVector2D(256, 256));

&#x20;   Sub->SetMaterialScalar("MyPlugin.PanelBg", "Glow", 2.0f);

&#x20;   Sub->SetMaterialVector("MyPlugin.PanelBg", "Tint",

&#x20;       FLinearColor(1.0f, 0.5f, 0.25f, 1.0f));



&#x20;   const FSlateBrush\* Brush = Sub->FindBrush("PanelBg", "MyPlugin");

&#x20;   // Use Brush in a widget...

}

```



\## Lifetime and Threading



\- The subsystem is created by the editor at startup and lives for the

&#x20; duration of the editor session.

\- `LoadFile` and `ReloadAll` parse and mutate state on the game thread.

&#x20; They must not be called from background threads directly. The internal

&#x20; directory watcher hops back to the game thread before reloading.

\- Dynamic material instances are owned by the subsystem. Do not destroy

&#x20; or garbage-collect them manually; they are released together with the

&#x20; subsystem or when their scope is removed.

