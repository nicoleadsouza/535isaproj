#include "SimulatorWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

SimulatorWindow::SimulatorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QPushButton* loadButton = new QPushButton("Load Program");
    QPushButton* runButton = new QPushButton("Run Cycles");
    QPushButton* stepButton = new QPushButton("Step");
    QPushButton* viewRegButton = new QPushButton("View Registers");
    QPushButton* viewMemButton = new QPushButton("View Memory");
    QPushButton* resetButton = new QPushButton("Reset");

    cycleInput = new QLineEdit();
    cycleInput->setPlaceholderText("Enter cycles");

    memLevelInput = new QLineEdit();
    memLevelInput->setPlaceholderText("Level (0=RAM,1=Cache)");
    memLineInput = new QLineEdit();
    memLineInput->setPlaceholderText("Line Number");

    cycleLabel = new QLabel("Cycles: 0");
    pcLabel = new QLabel("PC: 0");
    registerDisplay = new QTextEdit();
    registerDisplay->setReadOnly(true);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(loadButton);
    layout->addWidget(runButton);
    layout->addWidget(stepButton);
    layout->addWidget(viewRegButton);
    layout->addWidget(viewMemButton);
    layout->addWidget(resetButton);
    layout->addWidget(cycleInput);
    layout->addWidget(memLevelInput);
    layout->addWidget(memLineInput);
    layout->addWidget(cycleLabel);
    layout->addWidget(pcLabel);

    for (int i = 0; i < 5; ++i) {
        pipelineLabels[i] = new QLabel("Stage " + QString::number(i) + ": empty");
        layout->addWidget(pipelineLabels[i]);
    }

    layout->addWidget(registerDisplay);

    connect(loadButton, &QPushButton::clicked, this, &SimulatorWindow::loadProgram);
    connect(runButton, &QPushButton::clicked, this, &SimulatorWindow::runCycles);
    connect(stepButton, &QPushButton::clicked, this, &SimulatorWindow::stepCycle);
    connect(viewRegButton, &QPushButton::clicked, this, &SimulatorWindow::viewRegisters);
    connect(viewMemButton, &QPushButton::clicked, this, &SimulatorWindow::viewMemory);
    connect(resetButton, &QPushButton::clicked, this, &SimulatorWindow::resetSimulator);

    central->setLayout(layout);
    setWindowTitle("CacheFlow Simulator");
}

void SimulatorWindow::loadProgram() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Program File", "", "Program Files (*.txt *.bin)");
    if (!fileName.isEmpty()) {
        simulator.loadProgramFromFile(fileName.toStdString());
        updatePipelineDisplay();
    }
}

void SimulatorWindow::runCycles() {
    int cycles = cycleInput->text().toInt();
    for (int i = 0; i < cycles; ++i) {
        if (simulator.step() == FLAG_HALT) break;
    }
    updatePipelineDisplay();
}

void SimulatorWindow::stepCycle() {
    simulator.step();
    updatePipelineDisplay();
}

void SimulatorWindow::viewRegisters() {
    QString text;
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        text += QString("R%1: %2\n").arg(i, 2, 10, QLatin1Char('0')).arg(simulator.viewRegister(i));
    }
    registerDisplay->setText(text);
}

void SimulatorWindow::viewMemory() {
    int level = memLevelInput->text().toInt();
    int line = memLineInput->text().toInt();
    simulator.viewMemory(level, line);
}

void SimulatorWindow::resetSimulator() {
    simulator = Simulator();
    updatePipelineDisplay();
    registerDisplay->clear();
}

void SimulatorWindow::updatePipelineDisplay() {
    cycleLabel->setText("Cycles: " + QString::number(simulator.getCycleCount()));
    pcLabel->setText("PC: " + QString::number(simulator.getProgramCounter()));

    for (int i = 0; i < 5; ++i) {
        pipelineLabels[i]->setText("Stage " + QString::number(i) + ": " +
                                   QString::fromStdString(simulator.getStageDisplayText(i)));
    }
}
