#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include "basicsimulator.cpp"

class SimulatorWindow : public QMainWindow {
    Q_OBJECT

public:
    SimulatorWindow(QWidget *parent = nullptr);

private slots:
    void loadProgram();
    void runCycles();
    void stepCycle();
    void viewRegisters();
    void viewMemory();
    void resetSimulator();
    void runToCompletion();

private:
    Simulator simulator;
    QLabel* cycleLabel;
    QLabel* pcLabel;
    QLabel* pipelineLabels[5];
    QTextEdit* registerDisplay;
    QTextEdit* memoryDisplay; // <-- new memory output widget
    QLineEdit* cycleInput;
    QLineEdit* memLevelInput;
    QLineEdit* memLineInput;

    void updatePipelineDisplay();
};

#endif // SIMULATORWINDOW_H
