

#include "SimpleSlateStylerParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

void FSimpleSlateStylerParser::SkipWS()
{
    while (Pos < Source.Len())
    {
        const TCHAR C = Source[Pos];
        if (FChar::IsWhitespace(C))
        {
            ++Pos;
        }
        else if (C == '/' && Pos + 1 < Source.Len() && Source[Pos + 1] == '/')
        {
            // Line comment.
            while (Pos < Source.Len() && Source[Pos] != '\n')
            {
                ++Pos;
            }
        }
        else if (C == '/' && Pos + 1 < Source.Len() && Source[Pos + 1] == '*')
        {
            // Block comment.
            Pos += 2;
            while (Pos + 1 < Source.Len() && !(Source[Pos] == '*' && Source[Pos + 1] == '/'))
            {
                ++Pos;
            }
            Pos = FMath::Min(Pos + 2, Source.Len());
        }
        else
        {
            break;
        }
    }
}

TCHAR FSimpleSlateStylerParser::Peek(int32 Offset) const
{
    const int32 Index = Pos + Offset;
    return (Index >= 0 && Index < Source.Len()) ? Source[Index] : 0;
}

TCHAR FSimpleSlateStylerParser::Get()
{
    return (Pos < Source.Len()) ? Source[Pos++] : 0;
}

bool FSimpleSlateStylerParser::MatchChar(TCHAR C)
{
    if (Peek() == C)
    {
        ++Pos;
        return true;
    }
    return false;
}

bool FSimpleSlateStylerParser::ConsumeKeyword(const TCHAR* Kw)
{
    const int32 Len = FCString::Strlen(Kw);
    if (Pos + Len > Source.Len())
    {
        return false;
    }
    if (FCString::Strncmp(&Source[Pos], Kw, Len) != 0)
    {
        return false;
    }

    const TCHAR After = (Pos + Len < Source.Len()) ? Source[Pos + Len] : 0;
    if (FChar::IsAlnum(After) || After == '_')
    {
        return false;
    }

    Pos += Len;
    return true;
}

FString FSimpleSlateStylerParser::ReadBareWord()
{
    FString Result;
    while (Pos < Source.Len())
    {
        const TCHAR C = Source[Pos];
        if (FChar::IsWhitespace(C) ||
            C == '=' || C == ':' || C == '{' || C == '}' ||
            C == ',' || C == ';' ||
            C == '(' || C == ')' || C == '[' || C == ']')
        {
            break;
        }
        Result.AppendChar(C);
        ++Pos;
    }
    return Result;
}

FString FSimpleSlateStylerParser::ReadQuotedString()
{
    const TCHAR Quote = Get();
    FString Result;
    while (Pos < Source.Len())
    {
        const TCHAR C = Get();
        if (C == Quote)
        {
            break;
        }
        if (C == '\\' && Pos < Source.Len())
        {
            const TCHAR N = Get();
            if (N == 'n')      Result.AppendChar('\n');
            else if (N == 't') Result.AppendChar('\t');
            else if (N == 'r') Result.AppendChar('\r');
            else               Result.AppendChar(N);
        }
        else
        {
            Result.AppendChar(C);
        }
    }
    return Result;
}

float FSimpleSlateStylerParser::ReadNumber()
{
    FString NumStr;
    if (Peek() == '-' || Peek() == '+')
    {
        NumStr.AppendChar(Get());
    }

    while (Pos < Source.Len())
    {
        const TCHAR C = Source[Pos];
        const bool bExponentSign =
            (C == '-' || C == '+') &&
            NumStr.Len() > 0 &&
            (NumStr.EndsWith(TEXT("e")) || NumStr.EndsWith(TEXT("E")));

        if (FChar::IsDigit(C) || C == '.' || C == 'e' || C == 'E' || bExponentSign)
        {
            NumStr.AppendChar(C);
            ++Pos;
        }
        else
        {
            break;
        }
    }
    return FCString::Atof(*NumStr);
}

FLinearColor FSimpleSlateStylerParser::ReadColor()
{
    FLinearColor Color = FLinearColor::White;
    if (!MatchChar('#'))
    {
        return Color;
    }

    FString Hex;
    while (Pos < Source.Len() && FChar::IsHexDigit(Source[Pos]))
    {
        Hex.AppendChar(Source[Pos]);
        ++Pos;
    }

    if (Hex.Len() == 6)
    {
        const uint32 V = FParse::HexNumber(*Hex);
        Color.R = ((V >> 16) & 0xFF) / 255.0f;
        Color.G = ((V >> 8) & 0xFF) / 255.0f;
        Color.B = (V & 0xFF) / 255.0f;
        Color.A = 1.0f;
    }
    else if (Hex.Len() == 8)
    {
        const uint32 V = FParse::HexNumber(*Hex);
        Color.R = ((V >> 24) & 0xFF) / 255.0f;
        Color.G = ((V >> 16) & 0xFF) / 255.0f;
        Color.B = ((V >> 8) & 0xFF) / 255.0f;
        Color.A = (V & 0xFF) / 255.0f;
    }
    return Color;
}

bool FSimpleSlateStylerParser::ReadNumberList(TArray<float>& Out)
{
    TCHAR Close = 0;
    bool bHasBrackets = false;

    if (Peek() == '(') { Get(); Close = ')'; bHasBrackets = true; }
    else if (Peek() == '[') { Get(); Close = ']'; bHasBrackets = true; }

    while (Pos < Source.Len())
    {
        SkipWS();
        if (bHasBrackets && Peek() == Close)
        {
            Get();
            break;
        }

        const TCHAR C = Peek();
        if (!FChar::IsDigit(C) && C != '-' && C != '+' && C != '.')
        {
            break;
        }

        Out.Add(ReadNumber());
        SkipWS();
        MatchChar(',');
        SkipWS();
    }
    return Out.Num() > 0;
}

// ---------------------------------------------------------------------------
// Grammar
// ---------------------------------------------------------------------------

bool FSimpleSlateStylerParser::ParseFile(
    const FString& FilePath,
    TArray<FSimpleSlateStylerEntry>& OutEntries,
    TArray<FString>& OutErrors)
{
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *FilePath))
    {
        OutErrors.Add(FString::Printf(TEXT("Failed to read file: %s"), *FilePath));
        return false;
    }
    return ParseString(Text, OutEntries, OutErrors);
}

bool FSimpleSlateStylerParser::ParseString(
    const FString& InSource,
    TArray<FSimpleSlateStylerEntry>& OutEntries,
    TArray<FString>& OutErrors)
{
    FSimpleSlateStylerParser Parser(InSource);
    return Parser.ParseAll(OutEntries, OutErrors);
}

bool FSimpleSlateStylerParser::ParseAll(
    TArray<FSimpleSlateStylerEntry>& OutEntries,
    TArray<FString>& OutErrors)
{
    // Optional file-level scope header:
    //   scope MyPlugin
    // Any entry declared after this line has its name prefixed with "MyPlugin."
    // unless it already starts with that prefix.
    FName DefaultScope = NAME_None;

    SkipWS();
    if (ConsumeKeyword(TEXT("scope")))
    {
        SkipWS();
        // Allow "scope = MyPlugin" as well.
        MatchChar('=');
        SkipWS();

        FString ScopeStr = (Peek() == '"' || Peek() == '\'')
            ? ReadQuotedString()
            : ReadBareWord();

        if (ScopeStr.IsEmpty())
        {
            OutErrors.Add(TEXT("'scope' keyword requires a name."));
            return false;
        }
        DefaultScope = FName(*ScopeStr);
        SkipWS();
    }

    while (Pos < Source.Len())
    {
        FSimpleSlateStylerEntry Entry;
        FString Error;
        if (!ParseEntry(Entry, Error))
        {
            OutErrors.Add(FString::Printf(TEXT("Line %d: %s"), GetLine(), *Error));
            return false;
        }

        // Apply the file-level scope: prefix entry names that don't already
        // start with "<Scope>.".
        if (!DefaultScope.IsNone())
        {
            const FString ScopeStr = DefaultScope.ToString();
            const FString NameStr = Entry.Name.ToString();
            if (!NameStr.StartsWith(ScopeStr + TEXT(".")))
            {
                Entry.Name = FName(*(ScopeStr + TEXT(".") + NameStr));
            }
        }

        OutEntries.Add(MoveTemp(Entry));
        SkipWS();
    }
    return true;
}

bool FSimpleSlateStylerParser::ParseEntry(
    FSimpleSlateStylerEntry& OutEntry,
    FString& OutError)
{
    SkipWS();

    // Optional keyword before the entry name (ignored, purely cosmetic).
    if (!ConsumeKeyword(TEXT("style")))
    {
        ConsumeKeyword(TEXT("entry"));
    }

    SkipWS();

    FString Name = (Peek() == '"' || Peek() == '\'')
        ? ReadQuotedString()
        : ReadBareWord();

    if (Name.IsEmpty())
    {
        OutError = TEXT("Expected style name.");
        return false;
    }

    OutEntry.Name = FName(*Name);

    SkipWS();
    if (!MatchChar('{'))
    {
        OutError = FString::Printf(TEXT("Style '%s' is missing '{'."), *Name);
        return false;
    }

    return ParseBlock(OutEntry, OutError);
}

bool FSimpleSlateStylerParser::ParseBlock(
    FSimpleSlateStylerEntry& OutEntry,
    FString& OutError)
{
    while (true)
    {
        SkipWS();

        if (MatchChar('}'))
        {
            return true;
        }
        if (Pos >= Source.Len())
        {
            OutError = FString::Printf(
                TEXT("Style '%s' block is not closed."), *OutEntry.Name.ToString());
            return false;
        }
        if (!ParseProperty(OutEntry, OutError))
        {
            return false;
        }
    }
}

bool FSimpleSlateStylerParser::ParseProperty(
    FSimpleSlateStylerEntry& OutEntry,
    FString& OutError)
{
    SkipWS();

    FString Key = (Peek() == '"' || Peek() == '\'')
        ? ReadQuotedString()
        : ReadBareWord();

    if (Key.IsEmpty())
    {
        OutError = TEXT("Expected property name.");
        return false;
    }

    SkipWS();
    // '=' or ':' are both accepted.
    if (!MatchChar('='))
    {
        MatchChar(':');
    }
    SkipWS();

    // ----- Reserved keys -----
    if (Key.Equals(TEXT("Type"), ESearchCase::IgnoreCase))
    {
        OutEntry.Type = (Peek() == '"' || Peek() == '\'')
            ? ReadQuotedString()
            : ReadBareWord();
    }
    else if (Key.Equals(TEXT("Resource"), ESearchCase::IgnoreCase))
    {
        OutEntry.Resource = (Peek() == '"' || Peek() == '\'')
            ? ReadQuotedString()
            : ReadBareWord();
    }
    else if (Key.Equals(TEXT("Size"), ESearchCase::IgnoreCase))
    {
        TArray<float> Nums;
        if (!ReadNumberList(Nums) || Nums.Num() < 2)
        {
            OutError = FString::Printf(
                TEXT("Style '%s': Size needs at least 2 numbers."),
                *OutEntry.Name.ToString());
            return false;
        }
        OutEntry.Size = FVector2D(Nums[0], Nums[1]);
    }
    // ----- Material parameters -----
    else if (Peek() == '#')
    {
        OutEntry.VectorParams.Add(FName(*Key), ReadColor());
    }
    else if (FChar::IsDigit(Peek()) || Peek() == '-' || Peek() == '+' ||
        Peek() == '.' || Peek() == '(' || Peek() == '[')
    {
        TArray<float> Nums;
        ReadNumberList(Nums);

        if (Nums.Num() == 1)
        {
            OutEntry.ScalarParams.Add(FName(*Key), Nums[0]);
        }
        else if (Nums.Num() == 3 || Nums.Num() == 4)
        {
            FLinearColor C;
            C.R = Nums[0];
            C.G = Nums[1];
            C.B = Nums[2];
            C.A = (Nums.Num() >= 4) ? Nums[3] : 1.0f;
            OutEntry.VectorParams.Add(FName(*Key), C);
        }
        else
        {
            OutError = FString::Printf(
                TEXT("Style '%s': property '%s' has unsupported number count (%d)."),
                *OutEntry.Name.ToString(), *Key, Nums.Num());
            return false;
        }
    }
    else
    {
        // Unknown key with a string value: consume and ignore.
        if (Peek() == '"' || Peek() == '\'')
        {
            ReadQuotedString();
        }
        else
        {
            ReadBareWord();
        }
    }

    SkipWS();
    MatchChar(';');
    MatchChar(',');
    return true;
}

int32 FSimpleSlateStylerParser::GetLine() const
{
    int32 Line = 1;
    const int32 Limit = FMath::Min(Pos, Source.Len());
    for (int32 i = 0; i < Limit; ++i)
    {
        if (Source[i] == '\n')
        {
            ++Line;
        }
    }
    return Line;
}