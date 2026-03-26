#include "IBacktestSessionSwitchPrompt.h"

#include <QMessageBox>
#include <QPushButton>

BacktestSessionSwitchChoice QtBacktestSessionSwitchPrompt::askContinueCancel(
    QWidget* parent,
    const QString& title,
    const QString& message)
{
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(message);
    box.setIcon(QMessageBox::Warning);
    QPushButton* continueBtn =
        box.addButton(QStringLiteral("Continue"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Cancel);

    box.exec();
    if (box.clickedButton() == continueBtn)
        return BacktestSessionSwitchChoice::ContinueSwitch;
    return BacktestSessionSwitchChoice::CancelSwitch;
}
