#include "SimulatorWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <sstream>

SimulatorWindow::SimulatorWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QPushButton* loadButton = new QPushButton("Load Program");
    QPushButton* runButton = new QPushButton("Run Cycles");
    QPushButton* runToEndButton = new QPushButton("Run to End"); // optional extra
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

    memoryDisplay = new QTextEdit(); // new
    memoryDisplay->setReadOnly(true);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(loadButton);
    layout->addWidget(runButton);
    layout->addWidget(runToEndButton);
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
    layout->addWidget(memoryDisplay); // show live memory + cache here

    connect(loadButton, &QPushButton::clicked, this, &SimulatorWindow::loadProgram);
    connect(runButton, &QPushButton::clicked, this, &SimulatorWindow::runCycles);
    connect(runToEndButton, &QPushButton::clicked, this, &SimulatorWindow::runToCompletion);
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

void SimulatorWindow::runToCompletion() {
    while (simulator.step() == FLAG_RUNNING) {
        // optionally: QCoreApplication::processEvents();
    }
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
    simulator.viewMemory(level, line); // still uses console
}

void SimulatorWindow::resetSimulator() {
    simulator = Simulator();
    updatePipelineDisplay();
    registerDisplay->clear();
    memoryDisplay->clear();
}

void SimulatorWindow::updatePipelineDisplay() {
    cycleLabel->setText("Cycles: " + QString::number(simulator.getCycleCount()));
    pcLabel->setText("PC: " + QString::number(simulator.getProgramCounter()));

    for (int i = 0; i < 5; ++i) {
        pipelineLabels[i]->setText("Stage " + QString::number(i) + ": " +
            QString::fromStdString(simulator.getStageDisplayText(i)));
    }

    // 💡 New: Real-time memory + cache display
    QString memText = "CACHE (Lines 0–3):\n";
    for (int i = 0; i < 4; ++i) {
        std::ostringstream out;
        std::streambuf* old = std::cout.rdbuf(out.rdbuf());
        simulator.viewMemory(1, i);
        std::cout.rdbuf(old);
        memText += QString::fromStdString(out.str());
    }

    memText += "\nRAM (Lines 0–3):\n";
    for (int i = 0; i < 4; ++i) {
        std::ostringstream out;
        std::streambuf* old = std::cout.rdbuf(out.rdbuf());
        simulator.viewMemory(0, i);
        std::cout.rdbuf(old);
        memText += QString::fromStdString(out.str());
    }

    memoryDisplay->setPlainText(memText);
}
