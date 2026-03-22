#ifndef IUNSAVEDCHANGESPROMPT_H
#define IUNSAVEDCHANGESPROMPT_H

#include <QString>
#include <QWidget>

enum class UnsavedPromptChoice {
    Save,
    Discard,
    Cancel
};

class IUnsavedChangesPrompt {
public:
    virtual ~IUnsavedChangesPrompt() = default;

    virtual UnsavedPromptChoice askSaveDiscardCancel(QWidget* parent,
                                                       const QString& title,
                                                       const QString& message) = 0;
};

class QtUnsavedChangesPrompt final : public IUnsavedChangesPrompt {
public:
    UnsavedPromptChoice askSaveDiscardCancel(QWidget* parent,
                                             const QString& title,
                                             const QString& message) override;
};

#endif // IUNSAVEDCHANGESPROMPT_H
