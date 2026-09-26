
#include "Modules/ModuleManager.h"

#include "DirectoryWatcherModule.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "IDirectoryWatcher.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "SimpleSlateStylerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSimpleSlateStylerModule, Log, All);

class FSimpleSlateStylerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        RegisterConsoleCommands();
        RegisterDirectoryWatchers();
    }

    virtual void ShutdownModule() override
    {
        UnregisterDirectoryWatchers();
    }

private:
    // ------------------------------------------------------------------
    // Console commands
    // ------------------------------------------------------------------

    void RegisterConsoleCommands()
    {
        ReloadCommand = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("SimpleSlateStyler.Reload"),
            TEXT("Reload every .SimpleSlateStyler file."),
            FConsoleCommandDelegate::CreateRaw(this, &FSimpleSlateStylerModule::HandleReload),
            ECVF_Default);

        DumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("SimpleSlateStyler.Dump"),
            TEXT("Dump every style registered by SimpleSlateStyler."),
            FConsoleCommandDelegate::CreateRaw(this, &FSimpleSlateStylerModule::HandleDump),
            ECVF_Default);
    }

    void HandleReload()
    {
        if (!GEditor)
        {
            return;
        }
        if (USimpleSlateStylerSubsystem* Subsystem =
            GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>())
        {
            Subsystem->ReloadAll();
            UE_LOG(LogSimpleSlateStylerModule, Log, TEXT("Reload requested."));
        }
    }

    void HandleDump()
    {
        if (!GEditor)
        {
            return;
        }
        if (USimpleSlateStylerSubsystem* Subsystem =
            GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>())
        {
            Subsystem->DumpRegisteredStyles();
        }
    }

    // ------------------------------------------------------------------
    // Directory watchers
    // ------------------------------------------------------------------

    void RegisterDirectoryWatchers()
    {
        FDirectoryWatcherModule& WatcherModule =
            FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));

        IDirectoryWatcher* Watcher = WatcherModule.Get();
        if (!Watcher)
        {
            return;
        }

        // 1. Project config folder.
        {
            const FString ProjectDir =
                FPaths::ProjectConfigDir() / TEXT("SimpleSlateStyler");
            IFileManager::Get().MakeDirectory(*ProjectDir, /*Tree=*/true);

            Watcher->RegisterDirectoryChangedCallback_Handle(
                ProjectDir,
                IDirectoryWatcher::FDirectoryChanged::CreateRaw(
                    this, &FSimpleSlateStylerModule::OnDirectoryChanged),
                ProjectWatchHandle);
        }

        // 2. Every discovered plugin's own config folder.
        const TArray<TSharedRef<IPlugin>> Plugins =
            IPluginManager::Get().GetDiscoveredPlugins();
        for (const TSharedRef<IPlugin>& Plugin : Plugins)
        {
            const FString PluginDir =
                FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config/SimpleSlateStyler"));
            if (!IFileManager::Get().DirectoryExists(*PluginDir))
            {
                continue;
            }

            FDelegateHandle Handle;
            Watcher->RegisterDirectoryChangedCallback_Handle(
                PluginDir,
                IDirectoryWatcher::FDirectoryChanged::CreateRaw(
                    this, &FSimpleSlateStylerModule::OnDirectoryChanged),
                Handle);

            PluginWatchHandles.Add(PluginDir, Handle);
        }
    }

    void UnregisterDirectoryWatchers()
    {
        FDirectoryWatcherModule* WatcherModule =
            FModuleManager::GetModulePtr<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
        if (!WatcherModule)
        {
            return;
        }
        IDirectoryWatcher* Watcher = WatcherModule->Get();
        if (!Watcher)
        {
            return;
        }

        if (ProjectWatchHandle.IsValid())
        {
            Watcher->UnregisterDirectoryChangedCallback_Handle(
                FPaths::ProjectConfigDir() / TEXT("SimpleSlateStyler"),
                ProjectWatchHandle);
            ProjectWatchHandle.Reset();
        }

        for (const TPair<FString, FDelegateHandle>& Pair : PluginWatchHandles)
        {
            if (Pair.Value.IsValid())
            {
                Watcher->UnregisterDirectoryChangedCallback_Handle(Pair.Key, Pair.Value);
            }
        }
        PluginWatchHandles.Reset();
    }

    void OnDirectoryChanged(const TArray<FFileChangeData>& Changes)
    {
        bool bShouldReload = false;
        for (const FFileChangeData& Change : Changes)
        {
            if (Change.Filename.EndsWith(TEXT(".SimpleSlateStyler")))
            {
                bShouldReload = true;
                break;
            }
        }

        if (!bShouldReload)
        {
            return;
        }

        // DirectoryWatcher may fire from any thread; hop back to the game thread.
        AsyncTask(ENamedThreads::GameThread, []()
            {
                if (!GEditor)
                {
                    return;
                }
                if (USimpleSlateStylerSubsystem* Subsystem =
                    GEditor->GetEditorSubsystem<USimpleSlateStylerSubsystem>())
                {
                    Subsystem->ReloadAll();
                }
            });
    }

    IConsoleCommand* ReloadCommand = nullptr;
    IConsoleCommand* DumpCommand = nullptr;

    FDelegateHandle ProjectWatchHandle;
    TMap<FString, FDelegateHandle> PluginWatchHandles;
};

IMPLEMENT_MODULE(FSimpleSlateStylerModule, SimpleSlateStyler)