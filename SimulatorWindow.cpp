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

    // Toggles and labels
    pipelineToggle = new QCheckBox("Enable Pipeline");
    pipelineToggle->setChecked(true);
    cacheToggle = new QCheckBox("Enable Cache");
    cacheToggle->setChecked(true);
    modeLabel = new QLabel();
    cpiLabel = new QLabel("CPI: 0.0");
    hitMissLabel = new QLabel("Hits: 0  Misses: 0  Hit Rate: 0%");

    // Buttons
    QPushButton* loadButton = new QPushButton("Load Program");
    QPushButton* runButton = new QPushButton("Run Cycles");
    QPushButton* runToEndButton = new QPushButton("Run to End");
    QPushButton* runToBpButton = new QPushButton("Run to Breakpoint");
    QPushButton* stepButton = new QPushButton("Step");
    QPushButton* viewRegButton = new QPushButton("View Registers");
    QPushButton* viewMemButton = new QPushButton("View Memory");
    QPushButton* resetButton = new QPushButton("Reset");

    // Inputs
    cycleInput = new QLineEdit();
    cycleInput->setPlaceholderText("Enter cycles");

    memLevelInput = new QLineEdit();
    memLevelInput->setPlaceholderText("Level (0=RAM,1=Cache)");

    memLineInput = new QLineEdit();
    memLineInput->setPlaceholderText("Start Line");

    breakpointInput = new QLineEdit();
    breakpointInput->setPlaceholderText("Breakpoint PC");

    // Labels
    cycleLabel = new QLabel("Cycles: 0");
    pcLabel = new QLabel("PC: 0");

    registerDisplay = new QTextEdit();
    registerDisplay->setReadOnly(true);
    memoryDisplay = new QTextEdit();
    memoryDisplay->setReadOnly(true);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(pipelineToggle);
    layout->addWidget(cacheToggle);
    layout->addWidget(modeLabel);
    layout->addWidget(cpiLabel);
    layout->addWidget(hitMissLabel);
    layout->addWidget(loadButton);
    layout->addWidget(runButton);
    layout->addWidget(runToEndButton);
    layout->addWidget(runToBpButton);
    layout->addWidget(stepButton);
    layout->addWidget(viewRegButton);
    layout->addWidget(viewMemButton);
    layout->addWidget(resetButton);
    layout->addWidget(cycleInput);
    layout->addWidget(memLevelInput);
    layout->addWidget(memLineInput);
    layout->addWidget(breakpointInput);
    layout->addWidget(cycleLabel);
    layout->addWidget(pcLabel);

    const QString stageNames[5] = { "Fetch", "Decode", "Execute", "Memory", "Writeback" };
    for (int i = 0; i < 5; ++i) {
        pipelineLabels[i] = new QLabel(stageNames[i] + ": empty");
        layout->addWidget(pipelineLabels[i]);
    }

    QLabel* regLabel = new QLabel("Register File");
    regLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(regLabel);
    layout->addWidget(registerDisplay);

    QLabel* memLabel = new QLabel("Memory / Cache View");
    memLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(memLabel);
    layout->addWidget(memoryDisplay);

    central->setLayout(layout);
    setWindowTitle("CacheFlow Simulator");

    // Connect buttons to slots
    connect(loadButton, &QPushButton::clicked, this, &SimulatorWindow::loadProgram);
    connect(runButton, &QPushButton::clicked, this, &SimulatorWindow::runCycles);
    connect(runToEndButton, &QPushButton::clicked, this, &SimulatorWindow::runToCompletion);
    connect(runToBpButton, &QPushButton::clicked, this, &SimulatorWindow::runToBreakpoint);
    connect(stepButton, &QPushButton::clicked, this, &SimulatorWindow::stepCycle);
    connect(viewRegButton, &QPushButton::clicked, this, &SimulatorWindow::viewRegisters);
    connect(viewMemButton, &QPushButton::clicked, this, &SimulatorWindow::viewMemory);
    connect(resetButton, &QPushButton::clicked, this, &SimulatorWindow::resetSimulator);
    connect(pipelineToggle, &QCheckBox::stateChanged, this, &SimulatorWindow::updateModeLabel);
    connect(cacheToggle, &QCheckBox::stateChanged, this, &SimulatorWindow::updateModeLabel);

    updateModeLabel();
}

// buttons slots 

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
    while (simulator.step() == FLAG_RUNNING) {}
    updatePipelineDisplay();
}

void SimulatorWindow::runToBreakpoint() {
    bool ok;
    int bp = breakpointInput->text().toInt(&ok);
    if (!ok) return;

    while (simulator.getProgramCounter() != bp && simulator.step() == FLAG_RUNNING) {}
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
    int startLine = memLineInput->text().toInt();

    if (level != 0 && level != 1) {
        memoryDisplay->setPlainText("Invalid level. Use 0 for RAM or 1 for Cache.");
        return;
    }

    std::ostringstream out;
    std::streambuf* old = std::cout.rdbuf(out.rdbuf());

    for (int i = startLine; i < startLine + 10; ++i) {
        simulator.viewMemory(level, i);
    }

    std::cout.rdbuf(old);
    memoryDisplay->setPlainText(QString::fromStdString(out.str()));
}

void SimulatorWindow::resetSimulator() {
    simulator = Simulator(pipelineToggle->isChecked(), cacheToggle->isChecked());
    updatePipelineDisplay();
    registerDisplay->clear();
    memoryDisplay->clear();
}

// display update logic ==========

void SimulatorWindow::updatePipelineDisplay() {
    cycleLabel->setText("Cycles: " + QString::number(simulator.getCycleCount()));
    pcLabel->setText("PC: " + QString::number(simulator.getProgramCounter()));

    const QString stageNames[5] = { "Fetch", "Decode", "Execute", "Memory", "Writeback" };
    for (int i = 0; i < 5; ++i) {
        pipelineLabels[i]->setText(stageNames[i] + ": " +
            QString::fromStdString(simulator.getStageDisplayText(i)));
    }

    // CPI and Cache stats
    int instrs = simulator.getInstructionCount();
    int cycles = simulator.getCycleCount();
    double cpi = (instrs == 0) ? 0.0 : static_cast<double>(cycles) / instrs;
    cpiLabel->setText("CPI: " + QString::number(cpi, 'f', 2));

    int hits = simulator.getCacheHits();
    int misses = simulator.getCacheMisses();
    int total = hits + misses;
    double hitRate = (total == 0) ? 0.0 : (100.0 * hits / total);
    hitMissLabel->setText(QString("Hits: %1  Misses: %2  Hit Rate: %3%")
                          .arg(hits).arg(misses).arg(hitRate, 0, 'f', 1));

    // Memory summary
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

void SimulatorWindow::updateModeLabel() {
    QString modeText = "Mode: ";
    if (pipelineToggle->isChecked() && cacheToggle->isChecked()) {
        modeText += "Pipeline + Cache";
    } else if (pipelineToggle->isChecked()) {
        modeText += "Pipeline Only";
    } else if (cacheToggle->isChecked()) {
        modeText += "Cache Only";
    } else {
        modeText += "No Pipeline, No Cache";
    }
    modeLabel->setText(modeText);
}
