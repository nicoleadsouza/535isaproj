#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QCheckBox>
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
    void updateModeLabel();
    void runToBreakpoint();

private:
    Simulator simulator;

    QLabel* cycleLabel;
    QLabel* pcLabel;
    QLabel* pipelineLabels[5];
    QLabel* modeLabel;
    QLabel* cpiLabel;
    QLabel* hitMissLabel;

    QTextEdit* registerDisplay;
    QTextEdit* memoryDisplay;

    QLineEdit* cycleInput;
    QLineEdit* memLevelInput;
    QLineEdit* memLineInput;
    QLineEdit* breakpointInput;

    QCheckBox* pipelineToggle;
    QCheckBox* cacheToggle;

    void updatePipelineDisplay();
};

#endif // SIMULATORWINDOW_H
