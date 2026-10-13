// ============================================================================
//  MioNo - 应用入口
//  v6.9：增加 Voicemeeter Banana 虚拟声卡一键路由助手
// ============================================================================
#include <JuceHeader.h>
#include "MainWindow.h"
#include "AppleLookAndFeel.h"

class MioNoApplication : public juce::JUCEApplication
{
public:
    MioNoApplication() = default;

    const juce::String getApplicationName() override    { return "MioNo"; }
    const juce::String getApplicationVersion() override { return "6.9"; }

    // 只允许一个实例：已经在状态栏里跑着的时候，再点 exe 不会开出第二个，
    // 而是把原来那个窗口叫出来（音频设备是独占的，开第二个也没意义）。
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String& commandLine) override
    {
        lookAndFeel = std::make_unique<miono::AppleLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel.get());

        // 开机自启写的是 `"MioNo.exe" --tray`，带这个参数就【不弹窗口】，
        // 直接缩到状态栏安静地跑。
        const bool startInTray = commandLine.containsIgnoreCase ("--tray")
                              || commandLine.containsIgnoreCase ("--minimised")
                              || commandLine.containsIgnoreCase ("--minimized");

        mainWindow = std::make_unique<miono::MainWindow> ("MioNo", startInTray);
    }

    void shutdown() override
    {
        mainWindow.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        lookAndFeel.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    // 第二次启动时会被叫到这里：把已在跑的窗口叫回来就行
    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr)
            mainWindow->showFromTray();
    }

private:
    std::unique_ptr<miono::AppleLookAndFeel> lookAndFeel;
    std::unique_ptr<miono::MainWindow>       mainWindow;
};

START_JUCE_APPLICATION (MioNoApplication)
