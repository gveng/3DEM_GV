#include "MainWindow.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QSplitter>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QColor colorForMaterial(const QString& material) {
    if (material == "Copper") {
        return QColor(232, 172, 115, 200);
    }
    if (material == "FR4") {
        return QColor(103, 146, 84, 190);
    }
    return QColor(164, 189, 204, 165);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setupUi();
    setupData();
    rebuildProjectTree();
    updateViewport();
    updateInspector();
    updateActionState();
    updateValidationText();
}

void MainWindow::setupToolbar() {
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    newAction_ = toolbar->addAction("New");
    openAction_ = toolbar->addAction("Open");
    saveAction_ = toolbar->addAction("Save");
    connect(newAction_, &QAction::triggered, this, &MainWindow::onNewProject);
    connect(openAction_, &QAction::triggered, this, &MainWindow::onOpenProject);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::onSaveProject);
    toolbar->addSeparator();
    toolbar->addAction("Undo");
    toolbar->addAction("Redo");
    toolbar->addSeparator();

    hideAction_ = toolbar->addAction("Hide");
    showAction_ = toolbar->addAction("Show");
    connect(hideAction_, &QAction::triggered, this, &MainWindow::onHideSelected);
    connect(showAction_, &QAction::triggered, this, &MainWindow::onShowSelected);

    toolbar->addSeparator();
    toolbar->addAction("Precheck");
    solveAction_ = toolbar->addAction("Solve");
    connect(solveAction_, &QAction::triggered, this, &MainWindow::onSolveClicked);
}

void MainWindow::setupUi() {
    setWindowTitle("FEM EM Studio (Qt Native)");
    setupToolbar();

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(6, 6, 6, 6);
    rootLayout->setSpacing(6);

    auto* mainSplitter = new QSplitter(Qt::Horizontal, central);

    auto* leftPanel = new QWidget(mainSplitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* treeBox = new QGroupBox("Project Tree (grouped by material)", leftPanel);
    auto* treeLayout = new QVBoxLayout(treeBox);
    projectTree_ = new QTreeWidget(treeBox);
    projectTree_->setHeaderHidden(true);
    projectTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeLayout->addWidget(projectTree_);
    leftLayout->addWidget(treeBox);

    auto* materialsInfo = new QLabel("Tip: right-click in the 3D viewport with selected objects to open Material Window.", leftPanel);
    materialsInfo->setWordWrap(true);
    leftLayout->addWidget(materialsInfo);

    connect(projectTree_, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onProjectTreeSelectionChanged);

    viewport_ = new ViewportWidget(mainSplitter);
    connect(viewport_, &ViewportWidget::objectClicked, this, &MainWindow::onViewportObjectClicked);
    connect(viewport_, &ViewportWidget::contextMenuRequestedAt, this, &MainWindow::onViewportContextMenuRequested);

    auto* rightPanel = new QWidget(mainSplitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* inspectorBox = new QGroupBox("Inspector", rightPanel);
    auto* inspectorForm = new QFormLayout(inspectorBox);
    objectNameEdit_ = new QLineEdit(inspectorBox);
    xEdit_ = new QLineEdit(inspectorBox);
    yEdit_ = new QLineEdit(inspectorBox);
    zEdit_ = new QLineEdit(inspectorBox);
    wEdit_ = new QLineEdit(inspectorBox);
    hEdit_ = new QLineEdit(inspectorBox);
    lEdit_ = new QLineEdit(inspectorBox);

    for (auto* field : {objectNameEdit_, xEdit_, yEdit_, zEdit_, wEdit_, hEdit_, lEdit_}) {
        field->setReadOnly(true);
    }

    inspectorForm->addRow("Name", objectNameEdit_);
    inspectorForm->addRow("X", xEdit_);
    inspectorForm->addRow("Y", yEdit_);
    inspectorForm->addRow("Z", zEdit_);
    inspectorForm->addRow("W", wEdit_);
    inspectorForm->addRow("H", hEdit_);
    inspectorForm->addRow("L", lEdit_);
    rightLayout->addWidget(inspectorBox);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(viewport_);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(1, 1);

    rootLayout->addWidget(mainSplitter, 1);

    auto* tabs = new QTabWidget(central);

    auto* analysisTab = new QWidget(tabs);
    auto* analysisLayout = new QFormLayout(analysisTab);
    fMinEdit_ = new QLineEdit("1e9", analysisTab);
    fMaxEdit_ = new QLineEdit("8e9", analysisTab);
    sweepModeCombo_ = new QComboBox(analysisTab);
    sweepModeCombo_->addItems({"Linear", "Adaptive"});
    stepOrTolEdit_ = new QLineEdit("1e8", analysisTab);
    analysisLayout->addRow("fmin [Hz]", fMinEdit_);
    analysisLayout->addRow("fmax [Hz]", fMaxEdit_);
    analysisLayout->addRow("Sweep Mode", sweepModeCombo_);
    analysisLayout->addRow("Step / Tol", stepOrTolEdit_);
    tabs->addTab(analysisTab, "Analysis Setup");

    auto* portsTab = new QWidget(tabs);
    auto* portsLayout = new QFormLayout(portsTab);
    portNameEdit_ = new QLineEdit("P1", portsTab);
    portImpedanceEdit_ = new QLineEdit("50", portsTab);
    portTypeCombo_ = new QComboBox(portsTab);
    portTypeCombo_->addItems({"Wave", "Lumped", "Coax", "Differential"});
    portTargetEdit_ = new QLineEdit("face:12345", portsTab);
    portsLayout->addRow("Port Name (*)", portNameEdit_);
    portsLayout->addRow("Impedance [ohm] (*)", portImpedanceEdit_);
    portsLayout->addRow("Port Type", portTypeCombo_);
    portsLayout->addRow("Target", portTargetEdit_);
    tabs->addTab(portsTab, "Ports");

    validationConsole_ = new QPlainTextEdit(tabs);
    validationConsole_->setReadOnly(true);
    tabs->addTab(validationConsole_, "Validation Console");

    jobMonitor_ = new QPlainTextEdit(tabs);
    jobMonitor_->setReadOnly(true);
    tabs->addTab(jobMonitor_, "Simulation Monitor");

    rootLayout->addWidget(tabs);

    auto onValidationInputChanged = [this]() {
        updateValidationText();
    };

    connect(fMinEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);
    connect(fMaxEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);
    connect(stepOrTolEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);
    connect(sweepModeCombo_, &QComboBox::currentTextChanged, this, onValidationInputChanged);
    connect(portNameEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);
    connect(portImpedanceEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);
    connect(portTypeCombo_, &QComboBox::currentTextChanged, this, onValidationInputChanged);
    connect(portTargetEdit_, &QLineEdit::textChanged, this, onValidationInputChanged);

    setCentralWidget(central);
}

void MainWindow::setupData() {
    objects_.clear();
    selectedIds_.clear();

    objects_.insert("microstrip", Object3D{"microstrip", "MicrostripLine", "Copper", QRectF(0.12, 0.24, 0.52, 0.10), colorForMaterial("Copper"), true, 0, 0, 0, 1.2, 0.035, 28});
    objects_.insert("gnd", Object3D{"gnd", "GroundPlane", "Copper", QRectF(0.08, 0.52, 0.74, 0.12), colorForMaterial("Copper"), true, 0, -1, 0, 40, 0.035, 80});
    objects_.insert("substrate", Object3D{"substrate", "SubstrateBlock", "FR4", QRectF(0.06, 0.38, 0.78, 0.22), colorForMaterial("FR4"), true, 0, -0.8, 0, 40, 0.8, 80});
    objects_.insert("airbox", Object3D{"airbox", "AirBox", "Air", QRectF(0.03, 0.06, 0.9, 0.84), colorForMaterial("Air"), true, 0, 0, 0, 100, 50, 100});

    selectedIds_.insert("microstrip");
}

QString MainWindow::activeObjectId() const {
    if (selectedIds_.isEmpty()) {
        return {};
    }
    return *selectedIds_.constBegin();
}

void MainWindow::rebuildProjectTree() {
    syncingTree_ = true;
    projectTree_->clear();

    auto* root = new QTreeWidgetItem(projectTree_, QStringList() << "Design_01");
    root->setExpanded(true);
    root->setFlags(root->flags() & ~Qt::ItemIsSelectable);

    QStringList materials;
    for (const auto& object : objects_) {
        if (!materials.contains(object.material)) {
            materials.push_back(object.material);
        }
    }
    materials.sort();

    for (const auto& material : materials) {
        auto* materialGroup = new QTreeWidgetItem(root, QStringList() << QString("Material: %1").arg(material));
        materialGroup->setExpanded(true);
        materialGroup->setFlags(materialGroup->flags() & ~Qt::ItemIsSelectable);

        for (const auto& object : objects_) {
            if (object.material != material) {
                continue;
            }

            QString label = QString("Geometry / %1").arg(object.name);
            if (!object.visible) {
                label += " [Hidden]";
            }

            auto* child = new QTreeWidgetItem(materialGroup, QStringList() << label);
            child->setData(0, Qt::UserRole, object.id);

            if (!object.visible) {
                child->setForeground(0, QBrush(QColor(120, 120, 120)));
            }

            if (selectedIds_.contains(object.id)) {
                child->setSelected(true);
            }
        }
    }

    root->setExpanded(true);
    projectTree_->expandAll();
    syncingTree_ = false;
}

void MainWindow::updateViewport() {
    std::vector<ViewportWidget::DrawableObject> drawables;
    drawables.reserve(static_cast<size_t>(objects_.size()));

    for (const auto& object : objects_) {
        ViewportWidget::DrawableObject d;
        d.id = object.id;
        d.name = object.name;
        d.normalizedRect = object.normalizedRect;
        d.color = object.color;
        d.visible = object.visible;
        d.selected = selectedIds_.contains(object.id);
        drawables.push_back(d);
    }

    viewport_->setObjects(drawables);
}

void MainWindow::updateInspector() {
    const QString id = activeObjectId();
    if (id.isEmpty() || !objects_.contains(id)) {
        objectNameEdit_->setText("-");
        xEdit_->setText("-");
        yEdit_->setText("-");
        zEdit_->setText("-");
        wEdit_->setText("-");
        hEdit_->setText("-");
        lEdit_->setText("-");
        return;
    }

    const auto& object = objects_[id];
    objectNameEdit_->setText(object.name);
    xEdit_->setText(QString::number(object.x));
    yEdit_->setText(QString::number(object.y));
    zEdit_->setText(QString::number(object.z));
    wEdit_->setText(QString::number(object.w));
    hEdit_->setText(QString::number(object.h));
    lEdit_->setText(QString::number(object.l));
}

void MainWindow::updateActionState() {
    if (!hideAction_ || !showAction_) {
        return;
    }

    const bool single = selectedIds_.size() == 1;
    hideAction_->setEnabled(false);
    showAction_->setEnabled(false);

    if (!single) {
        return;
    }

    const QString id = activeObjectId();
    if (id.isEmpty() || !objects_.contains(id)) {
        return;
    }

    const bool visible = objects_[id].visible;
    hideAction_->setEnabled(visible);
    showAction_->setEnabled(!visible);
}

MainWindow::ValidationState MainWindow::buildValidationState() const {
    ValidationState state;
    state.lines << "OK: Project units are coherent.";

    const QString portName = portNameEdit_->text().trimmed();
    const QString portTarget = portTargetEdit_->text().trimmed();

    bool impedanceNumeric = false;
    const double impedance = portImpedanceEdit_->text().toDouble(&impedanceNumeric);

    bool fMinNumeric = false;
    bool fMaxNumeric = false;
    const double fMin = fMinEdit_->text().toDouble(&fMinNumeric);
    const double fMax = fMaxEdit_->text().toDouble(&fMaxNumeric);

    bool stepTolNumeric = false;
    const double stepOrTol = stepOrTolEdit_->text().toDouble(&stepTolNumeric);

    if (portName.isEmpty()) {
        state.lines << "ERROR: Port Name is required.";
        state.hasBlockingIssue = true;
    }

    if (!impedanceNumeric || impedance <= 0.0) {
        state.lines << "ERROR: Impedance must be a numeric value greater than 0 ohm.";
        state.hasBlockingIssue = true;
    } else if (impedance < 20.0 || impedance > 120.0) {
        state.lines << "WARNING: Impedance is outside typical SI range (20-120 ohm).";
    }

    if (portTarget.isEmpty()) {
        state.lines << "ERROR: Port Target is required.";
        state.hasBlockingIssue = true;
    }

    if (!fMinNumeric || !fMaxNumeric || fMin <= 0.0 || fMax <= 0.0) {
        state.lines << "ERROR: fmin and fmax must be numeric values > 0.";
        state.hasBlockingIssue = true;
    } else if (fMin >= fMax) {
        state.lines << "ERROR: fmin must be smaller than fmax.";
        state.hasBlockingIssue = true;
    } else {
        state.lines << "OK: Frequency range is valid.";
    }

    const bool adaptive = sweepModeCombo_->currentText() == "Adaptive";
    if (!stepTolNumeric || stepOrTol <= 0.0) {
        state.lines << QString("ERROR: %1 must be numeric and greater than 0.")
                           .arg(adaptive ? "Adaptive tolerance" : "Linear step");
        state.hasBlockingIssue = true;
    }

    if (!state.hasBlockingIssue) {
        state.lines << "OK: Solve is enabled.";
    }

    return state;
}

void MainWindow::updateValidationText() {
    const ValidationState state = buildValidationState();
    validationConsole_->setPlainText(state.lines.join("\n"));
    if (solveAction_) {
        solveAction_->setEnabled(!state.hasBlockingIssue);
    }
}

QJsonObject MainWindow::toProjectJson() const {
    QJsonObject root;
    root.insert("schemaVersion", "0.0.2");

    QJsonArray objectsArray;
    for (const auto& object : objects_) {
        QJsonObject o;
        o.insert("id", object.id);
        o.insert("name", object.name);
        o.insert("material", object.material);
        o.insert("visible", object.visible);

        QJsonObject rect;
        rect.insert("x", object.normalizedRect.x());
        rect.insert("y", object.normalizedRect.y());
        rect.insert("w", object.normalizedRect.width());
        rect.insert("h", object.normalizedRect.height());
        o.insert("normalizedRect", rect);

        QJsonObject dims;
        dims.insert("x", object.x);
        dims.insert("y", object.y);
        dims.insert("z", object.z);
        dims.insert("w", object.w);
        dims.insert("h", object.h);
        dims.insert("l", object.l);
        o.insert("dims", dims);

        objectsArray.push_back(o);
    }
    root.insert("objects", objectsArray);

    QJsonObject analysis;
    analysis.insert("fminHz", fMinEdit_->text().toDouble());
    analysis.insert("fmaxHz", fMaxEdit_->text().toDouble());
    analysis.insert("sweepMode", sweepModeCombo_->currentText());
    analysis.insert("stepOrTol", stepOrTolEdit_->text().toDouble());
    root.insert("analysis", analysis);

    QJsonObject port;
    port.insert("name", portNameEdit_->text().trimmed());
    port.insert("impedanceOhm", portImpedanceEdit_->text().toDouble());
    port.insert("type", portTypeCombo_->currentText());
    port.insert("target", portTargetEdit_->text().trimmed());
    root.insert("port", port);

    QJsonArray selected;
    for (const auto& id : selectedIds_) {
        selected.push_back(id);
    }
    root.insert("selectedIds", selected);

    return root;
}

bool MainWindow::loadFromProjectJson(const QJsonObject& root) {
    if (!root.contains("objects") || !root.value("objects").isArray()) {
        return false;
    }

    QMap<QString, Object3D> loadedObjects;
    const QJsonArray objectsArray = root.value("objects").toArray();

    for (const QJsonValue& value : objectsArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject o = value.toObject();
        const QString id = o.value("id").toString().trimmed();
        if (id.isEmpty()) {
            continue;
        }

        Object3D object;
        object.id = id;
        object.name = o.value("name").toString(id);
        object.material = o.value("material").toString("Air");
        object.visible = o.value("visible").toBool(true);

        const QJsonObject rect = o.value("normalizedRect").toObject();
        object.normalizedRect = QRectF(
            rect.value("x").toDouble(0.1),
            rect.value("y").toDouble(0.1),
            rect.value("w").toDouble(0.2),
            rect.value("h").toDouble(0.2));

        const QJsonObject dims = o.value("dims").toObject();
        object.x = dims.value("x").toDouble(0.0);
        object.y = dims.value("y").toDouble(0.0);
        object.z = dims.value("z").toDouble(0.0);
        object.w = dims.value("w").toDouble(1.0);
        object.h = dims.value("h").toDouble(1.0);
        object.l = dims.value("l").toDouble(1.0);
        object.color = colorForMaterial(object.material);

        loadedObjects.insert(object.id, object);
    }

    if (loadedObjects.isEmpty()) {
        return false;
    }

    objects_ = loadedObjects;
    selectedIds_.clear();

    const QJsonArray selected = root.value("selectedIds").toArray();
    for (const QJsonValue& value : selected) {
        const QString id = value.toString();
        if (objects_.contains(id)) {
            selectedIds_.insert(id);
        }
    }
    if (selectedIds_.isEmpty()) {
        selectedIds_.insert(objects_.first().id);
    }

    const QJsonObject analysis = root.value("analysis").toObject();
    if (!analysis.isEmpty()) {
        fMinEdit_->setText(QString::number(analysis.value("fminHz").toDouble(1.0e9)));
        fMaxEdit_->setText(QString::number(analysis.value("fmaxHz").toDouble(8.0e9)));
        const QString sweepMode = analysis.value("sweepMode").toString("Linear");
        const int sweepIdx = sweepModeCombo_->findText(sweepMode);
        sweepModeCombo_->setCurrentIndex(sweepIdx >= 0 ? sweepIdx : 0);
        stepOrTolEdit_->setText(QString::number(analysis.value("stepOrTol").toDouble(1.0e8)));
    }

    const QJsonObject port = root.value("port").toObject();
    if (!port.isEmpty()) {
        portNameEdit_->setText(port.value("name").toString("P1"));
        portImpedanceEdit_->setText(QString::number(port.value("impedanceOhm").toDouble(50.0)));
        const QString portType = port.value("type").toString("Wave");
        const int typeIdx = portTypeCombo_->findText(portType);
        portTypeCombo_->setCurrentIndex(typeIdx >= 0 ? typeIdx : 0);
        portTargetEdit_->setText(port.value("target").toString("face:12345"));
    }

    rebuildProjectTree();
    updateViewport();
    updateInspector();
    updateActionState();
    updateValidationText();
    jobMonitor_->setPlainText("Status: Project loaded.");
    return true;
}

void MainWindow::onNewProject() {
    setupData();
    projectFilePath_.clear();
    fMinEdit_->setText("1e9");
    fMaxEdit_->setText("8e9");
    sweepModeCombo_->setCurrentText("Linear");
    stepOrTolEdit_->setText("1e8");
    portNameEdit_->setText("P1");
    portImpedanceEdit_->setText("50");
    portTypeCombo_->setCurrentText("Wave");
    portTargetEdit_->setText("face:12345");

    rebuildProjectTree();
    updateViewport();
    updateInspector();
    updateActionState();
    updateValidationText();
    jobMonitor_->setPlainText("Status: New project created.");
}

void MainWindow::onOpenProject() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Project", QString(), "3DEM Project (*.json)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Open Project", "Cannot open selected file.");
        return;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::critical(this, "Open Project", "Invalid JSON project file.");
        return;
    }

    if (!loadFromProjectJson(doc.object())) {
        QMessageBox::critical(this, "Open Project", "Project file is missing required fields.");
        return;
    }

    projectFilePath_ = path;
}

void MainWindow::onSaveProject() {
    QString path = projectFilePath_;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, "Save Project", QString(), "3DEM Project (*.json)");
        if (path.isEmpty()) {
            return;
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Save Project", "Cannot write project file.");
        return;
    }

    const QJsonDocument doc(toProjectJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    projectFilePath_ = path;
    jobMonitor_->setPlainText(QString("Status: Project saved to %1").arg(path));
}

void MainWindow::openMaterialDialog(const QPoint& globalPos) {
    if (selectedIds_.isEmpty()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Material Window");
    dialog.move(globalPos + QPoint(8, 8));

    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("Selected objects:", &dialog));

    auto* selectedList = new QListWidget(&dialog);
    for (const auto& id : selectedIds_) {
        if (!objects_.contains(id)) {
            continue;
        }
        selectedList->addItem(QString("%1 (%2)").arg(objects_[id].name, objects_[id].material));
    }
    layout->addWidget(selectedList);

    auto* materialCombo = new QComboBox(&dialog);
    materialCombo->addItems({"Copper", "FR4", "Air"});

    QString firstMaterial;
    bool sameMaterial = true;
    for (const auto& id : selectedIds_) {
        if (!objects_.contains(id)) {
            continue;
        }
        if (firstMaterial.isEmpty()) {
            firstMaterial = objects_[id].material;
        } else if (objects_[id].material != firstMaterial) {
            sameMaterial = false;
            break;
        }
    }
    if (sameMaterial && !firstMaterial.isEmpty()) {
        materialCombo->setCurrentText(firstMaterial);
    }

    auto* form = new QFormLayout();
    form->addRow("Assign material", materialCombo);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString chosen = materialCombo->currentText();
    for (const auto& id : selectedIds_) {
        if (!objects_.contains(id)) {
            continue;
        }
        auto& object = objects_[id];
        object.material = chosen;
        object.color = colorForMaterial(chosen);
    }

    rebuildProjectTree();
    updateViewport();
}

void MainWindow::selectOnly(const QString& objectId) {
    selectedIds_.clear();
    selectedIds_.insert(objectId);
}

void MainWindow::toggleOrAddSelection(const QString& objectId) {
    if (selectedIds_.contains(objectId) && selectedIds_.size() > 1) {
        selectedIds_.remove(objectId);
    } else {
        selectedIds_.insert(objectId);
    }
}

void MainWindow::onProjectTreeSelectionChanged() {
    if (syncingTree_) {
        return;
    }

    QSet<QString> newSelection;
    const auto selectedItems = projectTree_->selectedItems();
    for (auto* item : selectedItems) {
        const QString id = item->data(0, Qt::UserRole).toString();
        if (!id.isEmpty()) {
            newSelection.insert(id);
        }
    }

    if (newSelection.isEmpty()) {
        return;
    }

    selectedIds_ = newSelection;
    updateViewport();
    updateInspector();
    updateActionState();
}

void MainWindow::onViewportObjectClicked(const QString& objectId, bool additiveSelection) {
    if (!objects_.contains(objectId)) {
        return;
    }

    if (additiveSelection) {
        toggleOrAddSelection(objectId);
    } else {
        selectOnly(objectId);
    }

    rebuildProjectTree();
    updateViewport();
    updateInspector();
    updateActionState();
}

void MainWindow::onViewportContextMenuRequested(const QPoint& globalPos) {
    if (selectedIds_.isEmpty()) {
        return;
    }

    openMaterialDialog(globalPos);
}

void MainWindow::onHideSelected() {
    const QString id = activeObjectId();
    if (id.isEmpty() || !objects_.contains(id)) {
        return;
    }

    objects_[id].visible = false;
    rebuildProjectTree();
    updateViewport();
    updateActionState();
}

void MainWindow::onShowSelected() {
    const QString id = activeObjectId();
    if (id.isEmpty() || !objects_.contains(id)) {
        return;
    }

    objects_[id].visible = true;
    rebuildProjectTree();
    updateViewport();
    updateActionState();
}

void MainWindow::onSolveClicked() {
    const ValidationState validation = buildValidationState();
    if (validation.hasBlockingIssue) {
        updateValidationText();
        jobMonitor_->setPlainText("Status: Blocked\nReason: Fix validation errors before solving.");
        return;
    }

    FrequencySweepConfig sweep;
    sweep.fMinHz = fMinEdit_->text().toDouble();
    sweep.fMaxHz = fMaxEdit_->text().toDouble();
    sweep.mode = sweepModeCombo_->currentText() == "Adaptive" ? FrequencySweepMode::Adaptive : FrequencySweepMode::Linear;

    const double numeric = stepOrTolEdit_->text().toDouble();
    if (sweep.mode == FrequencySweepMode::Linear) {
        sweep.stepHz = numeric;
    } else {
        sweep.tolerance = numeric > 0.0 ? numeric : 0.02;
    }

    PortConfig port;
    port.name = portNameEdit_->text().trimmed();
    port.impedanceOhm = portImpedanceEdit_->text().toDouble();

    updateValidationText();

    const auto summary = femBridge_.runFrequencySweep(sweep, port);
    jobMonitor_->setPlainText(QString("Status: %1\nSolved points: %2\nElapsed (sim): %3 s")
                                  .arg(summary.ok ? "Completed" : "Failed")
                                  .arg(summary.solvedPoints)
                                  .arg(summary.elapsedSeconds, 0, 'f', 3));
}
