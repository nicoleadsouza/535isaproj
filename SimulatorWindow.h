#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include "basicsimulator.cpp" // your existing simulator

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

private:
    Simulator simulator; // your existing Simulator class
    QLabel* cycleLabel;
    QLabel* pcLabel;
    QLabel* pipelineLabels[5];
    QTextEdit* registerDisplay;
    QLineEdit* cycleInput;
    QLineEdit* memLevelInput;
    QLineEdit* memLineInput;

    void updatePipelineDisplay();
    void updateRegisterDisplay();
};

#endif // SIMULATORWINDOW_H
