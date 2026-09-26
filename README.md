# Simple Slate Styler

A tiny DSL for declaring editor UI brushes in Unreal Engine 5.
Drop a `.SimpleSlateStyler` file into your plugin's `Config/` folder
and your textures and materials become available as Slate brushes —
no C++ required.

## Features

- Texture and material brushes declared in a plain text DSL
- Runtime material parameter tweaking (`SetMaterialScalar` / `SetMaterialVector`)
- Per-plugin style scopes, so plugins never stomp on each other
- Hot reload on file save
- Console commands: `SimpleSlateStyler.Reload`, `SimpleSlateStyler.Dump`
- Automatic file discovery under `<Project>/Config/SimpleSlateStyler/` and
  `<AnyPlugin>/Config/SimpleSlateStyler/`

## Requirements

- Unreal Engine 5.8 or newer
- Editor only (Windows / macOS / Linux)

## Installation

1. Copy the `SimpleSlateStyler/` folder into `<YourProject>/Plugins/`
2. Right-click the `.uproject` file and choose **Generate Visual Studio project files**
3. Build the project
4. Launch the editor

The plugin loads automatically at `PostEngineInit`.

## Quick Start

Create `<YourProject>/Config/SimpleSlateStyler/MyStyles.SimpleSlateStyler`:

```simple
scope MyPlugin

PanelBg {
    Type = Material
    Resource = /Game/UI/M_PanelBg
    Size = 256, 256
    Glow = 1.5
}

CloseIcon {
    Resource = /Game/UI/T_Close
    Size = 16, 16
}
```

Use the brush in C++:

```cpp
const FSlateBrush* Brush = FAppStyle::Get().GetBrush("MyPlugin.PanelBg");
```

Or use the scope-aware lookup:

```cpp
USimpleSlateStylerSubsystem* Sub =
    GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>();

const FSlateBrush* Brush = Sub->FindBrush("PanelBg", "MyPlugin");
```

## Console Commands

| Command | Description |
| --- | --- |
| `SimpleSlateStyler.Reload` | Re-scan every `.SimpleSlateStyler` file |
| `SimpleSlateStyler.Dump` | Print all registered styles to the Output Log |

## Scope Isolation

Every style key that contains a dot is automatically grouped by the
first segment. `MyPlugin.PanelBg` belongs to scope `MyPlugin`.
Plugins that use the same short name in different scopes never collide.

```cpp
Sub->GetScopeNames();                    // ["MyPlugin", "OtherPlugin"]

Sub->GetStylesInScope("MyPlugin");       // ["MyPlugin.PanelBg", ...]
Sub->FindBrush("PanelBg", "MyPlugin");   // scope-aware lookup
Sub->RemoveScope("MyPlugin");            // unregister the whole scope
```

## Documentation

- [DSL Reference](Docs/DSL.md)
- [C++ API](Docs/API.md)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). All commits must be signed off
(`git commit -s`) per the Developer Certificate of Origin.

## License

Licensed under the Apache License, Version 2.0.
See [LICENSE](LICENSE) and [NOTICE](NOTICE) for details.