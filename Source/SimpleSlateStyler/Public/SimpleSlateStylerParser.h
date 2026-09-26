

#pragma once

#include "CoreMinimal.h"

/**
 * One parsed entry from a .SimpleSlateStyler file.
 * Field meaning:
 *   Name         - the style key, e.g. "MyPlugin.PanelBg".
 *   Type         - "Material" or "Texture". Empty means auto-detect.
 *   Resource     - long package path, e.g. "/Game/UI/M_Bg".
 *   Size         - brush image size.
 *   ScalarParams - material scalar parameters (only used for Material type).
 *   VectorParams - material vector parameters (only used for Material type).
 */
struct FSimpleSlateStylerEntry
{
    FName Name;
    FString Type;
    FString Resource;
    FVector2D Size = FVector2D(64.0, 64.0);
    TMap<FName, float> ScalarParams;
    TMap<FName, FLinearColor> VectorParams;
};

/**
 * Recursive descent parser for .SimpleSlateStyler files.
 * Pure C++, no UObject reflection, safe to call from any thread (parsing only).
 */
class SIMPLESLATESTYLER_API FSimpleSlateStylerParser
{
public:
    /** Parse a source string. Returns false if any syntax error occurred. */
    static bool ParseString(
        const FString& Source,
        TArray<FSimpleSlateStylerEntry>& OutEntries,
        TArray<FString>& OutErrors);

    /** Load a file and parse it. */
    static bool ParseFile(
        const FString& FilePath,
        TArray<FSimpleSlateStylerEntry>& OutEntries,
        TArray<FString>& OutErrors);

private:
    explicit FSimpleSlateStylerParser(const FString& InSource)
        : Source(InSource)
        , Pos(0)
    {}

    bool ParseAll(TArray<FSimpleSlateStylerEntry>& OutEntries, TArray<FString>& OutErrors);
    bool ParseEntry(FSimpleSlateStylerEntry& OutEntry, FString& OutError);
    bool ParseBlock(FSimpleSlateStylerEntry& OutEntry, FString& OutError);
    bool ParseProperty(FSimpleSlateStylerEntry& OutEntry, FString& OutError);

    // Lexer helpers.
    void SkipWS();
    TCHAR Peek(int32 Offset = 0) const;
    TCHAR Get();
    bool MatchChar(TCHAR C);
    bool ConsumeKeyword(const TCHAR* Kw);
    FString ReadBareWord();
    FString ReadQuotedString();
    float ReadNumber();
    FLinearColor ReadColor();
    bool ReadNumberList(TArray<float>& Out);

    int32 GetLine() const;

    const FString& Source;
    int32 Pos;
};