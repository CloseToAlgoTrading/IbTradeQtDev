#ifndef IBACKTESTSESSIONSWITCHPROMPT_H
#define IBACKTESTSESSIONSWITCHPROMPT_H

#include <QString>
#include <QWidget>

enum class BacktestSessionSwitchChoice {
    ContinueSwitch,
    CancelSwitch
};

class IBacktestSessionSwitchPrompt
{
public:
    virtual ~IBacktestSessionSwitchPrompt() = default;

    virtual BacktestSessionSwitchChoice askContinueCancel(QWidget* parent,
                                                          const QString& title,
                                                          const QString& message) = 0;
};

class QtBacktestSessionSwitchPrompt final : public IBacktestSessionSwitchPrompt
{
public:
    BacktestSessionSwitchChoice askContinueCancel(QWidget* parent,
                                                  const QString& title,
                                                  const QString& message) override;
};

#endif // IBACKTESTSESSIONSWITCHPROMPT_H
