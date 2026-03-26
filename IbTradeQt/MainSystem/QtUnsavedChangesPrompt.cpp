#include "IUnsavedChangesPrompt.h"

#include <QAbstractButton>
#include <QMessageBox>

UnsavedPromptChoice QtUnsavedChangesPrompt::askSaveDiscardCancel(QWidget* parent,
                                                                   const QString& title,
                                                                   const QString& message)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(message);
    box.setIcon(QMessageBox::Question);
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    box.button(QMessageBox::Discard)->setText(QStringLiteral("Discard"));

    const int ret = box.exec();
    if (ret == QMessageBox::Save)
        return UnsavedPromptChoice::Save;
    if (ret == QMessageBox::Discard)
        return UnsavedPromptChoice::Discard;
    return UnsavedPromptChoice::Cancel;
}
