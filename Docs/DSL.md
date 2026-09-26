\# DSL Reference



The `.SimpleSlateStyler` format is a small, hand-written DSL for declaring

editor UI brushes. It parses in a single pass, has no dependencies on

Unreal's reflection system, and can be edited by designers and TAs.



\## File Discovery



The plugin scans the following locations recursively:



1\. `<Project>/Config/SimpleSlateStyler/\*\*/\*.SimpleSlateStyler`

2\. `<AnyPlugin>/Config/SimpleSlateStyler/\*\*/\*.SimpleSlateStyler`



Files are discovered at editor startup and re-scanned whenever a matching

file changes on disk.



\## File Syntax



A file is a sequence of entries:



```simple

Name {

&#x20;   Key = Value

&#x20;   Key = Value

}

```



\- Whitespace is insignificant.

\- `=` and `:` are interchangeable.

\- `;` and `,` after a value are optional separators.

\- An entry may be prefixed with the cosmetic keyword `style` or `entry`.



\## Entry Names



Entry names are the style keys used to look brushes up later.



\- Bare words: `MyPlugin.PanelBg`

\- Quoted strings: `"My Plugin.Panel Bg"` (use quotes if the name contains spaces)



If the file declares a `scope` (see below), bare names are automatically

prefixed with that scope.



\## Reserved Properties



Three keys have special meaning and are not treated as material parameters.



\### `Type`



Force the brush type. Accepted values:



\- `Texture` (default when the resource looks like a texture)

\- `Material`



Omit this in most cases; the type is auto-detected.



\### `Resource`



Long package path to the asset.



\- Texture: `/Game/UI/T\_Close` or `/Engine/EditorResources/S\_Level`

\- Material: `/Game/UI/M\_PanelBg` or `/Engine/EngineMaterials/DefaultMaterial`



The path \*\*must not\*\* include an object suffix like `.T\_Close`; the

plugin resolves the first asset at that package path.



\### `Size`



Brush image size in Slate units. Accepts two numbers.



```simple

Size = 64, 64

Size = (256, 256)

Size = \[16, 16]

```



Defaults to `64, 64` if omitted.



\## Custom Material Parameters



Every property that is not one of the three reserved keys becomes a material

parameter. The value's shape decides the parameter type:



| Value form | Parameter type |

| --- | --- |

| Single number | Scalar (`SetScalarParameterValue`) |

| Three or four numbers | Vector (`SetVectorParameterValue`) |

| `#RRGGBB` / `#RRGGBBAA` | Vector |



```simple

PanelBg {

&#x20;   Resource = /Game/UI/M\_PanelBg

&#x20;   Glow     = 1.5             // scalar

&#x20;   Tint     = #FF6600         // vector, alpha defaults to 1.0

&#x20;   Offset   = 0.1, 0.2, 0.0   // vector (RGB)

&#x20;   Fade     = 0.0, 0.0, 0.0, 0.5  // vector (RGBA)

}

```



Parameter names are case-sensitive and must match the parameter names

defined inside the material asset.



\## Value Types



\### Numbers



Integers, floats, negative numbers and scientific notation are all accepted.



```simple

Speed    = 1

Falloff  = 0.85

OffsetX  = -32

Exponent = 1.5e-3

```



\### Number Lists



Numbers can be separated by commas or just whitespace, and can optionally be

wrapped in parentheses or square brackets.



```simple

Size   = 64 64

Offset = (0.1, 0.2, 0.0)

Weights = \[1, 0.5, 0.25, 1]

```



\### Colors



Hex colors are written with a leading `#`.



```simple

Tint   = #FF6600       // RRGGBB, alpha = 1.0

Overlay = #FF660080    // RRGGBBAA

```



\### Strings



Bare words cover most cases. Use single or double quotes when the value

contains spaces, punctuation, or a `#`.



```simple

Resource = /Game/UI/M\_PanelBg

Tooltip  = "Panel background brush"

```



\## Comments



```simple

// Line comment



/\*

&#x20;   Block comment

\*/

```



\## Scope



An optional `scope` keyword at the top of a file prefixes every entry name

below it.



```simple

scope MyPlugin



PanelBg {

&#x20;   Resource = /Game/UI/M\_PanelBg

}

```



`PanelBg` is registered as `MyPlugin.PanelBg`.



Rules:



\- Only the first non-comment line may be a `scope` declaration.

\- Entry names that already start with `<Scope>.` are not prefixed twice.

\- Omitting `scope` is equivalent to writing the full key on each entry.



\## Type Auto-detection



When `Type` is omitted, the plugin inspects the `Resource` string:



\- Contains `/M\_` or `\_M.` → treated as `Material`

\- Otherwise → treated as `Texture`



For unambiguous assets, always write the full path and let the plugin

decide. Use `Type = Material` when a material path does not match the

heuristic.



\## Full Example



```simple

// MyPlugin editor styles.

scope MyPlugin



// A material-backed panel background with runtime tweakable parameters.

PanelBg {

&#x20;   Type     = Material

&#x20;   Resource = /Game/MyPlugin/UI/M\_PanelBg

&#x20;   Size     = 256, 256

&#x20;   Tint     = #FFFFFF

&#x20;   Glow     = 1.2

&#x20;   Speed    = 0.4

}



// A simple texture icon.

CloseIcon {

&#x20;   Resource = /Game/MyPlugin/UI/T\_Close

&#x20;   Size     = 16, 16

}



// Texture with a non-standard size.

HeaderIcon {

&#x20;   Resource = /Game/MyPlugin/UI/T\_Header

&#x20;   Size     = 32, 32

}

```



After saving, run the console command `SimpleSlateStyler.Dump` to verify

that the entries are registered.



\## Grammar Reference



```

file        := \[ scope-decl ] { entry }

scope-decl  := "scope" \[ "=" | ":" ] name

entry       := \[ "style" | "entry" ] name "{" { property } "}"

property    := name \[ "=" | ":" ] value \[ ";" | "," ]

value       := number | number-list | color | string

number-list := \[ "(" | "\[" ] number { "," number } \[ ")" | "]" ]

```



Where `name` is either a bare word or a quoted string.

