// ============================================================================
//  MioNo - 应用入口
// ============================================================================
#include <JuceHeader.h>
#include "MainWindow.h"
#include "AppleLookAndFeel.h"

class MioNoApplication : public juce::JUCEApplication
{
public:
    MioNoApplication() = default;

    const juce::String getApplicationName() override    { return "MioNo"; }
    const juce::String getApplicationVersion() override { return "6.0"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        lookAndFeel = std::make_unique<miono::AppleLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel.get());

        mainWindow = std::make_unique<miono::MainWindow> ("MioNo");
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

    void anotherInstanceStarted (const juce::String&) override {}

private:
    std::unique_ptr<miono::AppleLookAndFeel> lookAndFeel;
    std::unique_ptr<miono::MainWindow>       mainWindow;
};

START_JUCE_APPLICATION (MioNoApplication)
