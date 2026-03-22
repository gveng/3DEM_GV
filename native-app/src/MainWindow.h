#pragma once

#include "FemCoreBridge.h"
#include "SimulationConfig.h"
#include "ViewportWidget.h"

#include <QAction>
#include <QColor>
#include <QJsonObject>
#include <QMainWindow>
#include <QMap>
#include <QRectF>
#include <QSet>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QTabWidget;
class QTreeWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onProjectTreeSelectionChanged();
    void onViewportObjectClicked(const QString& objectId, bool additiveSelection);
    void onViewportContextMenuRequested(const QPoint& globalPos);
    void onHideSelected();
    void onShowSelected();
    void onSolveClicked();

private:
    struct ValidationState {
        bool hasBlockingIssue = false;
        QStringList lines;
    };

    struct Object3D {
        QString id;
        QString name;
        QString material;
        QRectF normalizedRect;
        QColor color;
        bool visible = true;

        // Inspector placeholders
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double w = 1.0;
        double h = 1.0;
        double l = 1.0;
    };

    void setupUi();
    void setupToolbar();
    void setupData();
    void rebuildProjectTree();
    void updateViewport();
    void updateInspector();
    void updateActionState();
    ValidationState buildValidationState() const;
    void updateValidationText();
    void openMaterialDialog(const QPoint& globalPos);
    QJsonObject toProjectJson() const;
    bool loadFromProjectJson(const QJsonObject& root);

    QString activeObjectId() const;
    void selectOnly(const QString& objectId);
    void toggleOrAddSelection(const QString& objectId);

    bool syncingTree_ = false;

    QMap<QString, Object3D> objects_;
    QSet<QString> selectedIds_;

    FemCoreBridge femBridge_;

    QAction* newAction_ = nullptr;
    QAction* openAction_ = nullptr;
    QAction* saveAction_ = nullptr;
    QAction* hideAction_ = nullptr;
    QAction* showAction_ = nullptr;
    QAction* solveAction_ = nullptr;

    QTreeWidget* projectTree_ = nullptr;
    ViewportWidget* viewport_ = nullptr;

    QLineEdit* objectNameEdit_ = nullptr;
    QLineEdit* xEdit_ = nullptr;
    QLineEdit* yEdit_ = nullptr;
    QLineEdit* zEdit_ = nullptr;
    QLineEdit* wEdit_ = nullptr;
    QLineEdit* hEdit_ = nullptr;
    QLineEdit* lEdit_ = nullptr;

    QLineEdit* fMinEdit_ = nullptr;
    QLineEdit* fMaxEdit_ = nullptr;
    QComboBox* sweepModeCombo_ = nullptr;
    QLineEdit* stepOrTolEdit_ = nullptr;

    QLineEdit* portNameEdit_ = nullptr;
    QLineEdit* portImpedanceEdit_ = nullptr;
    QComboBox* portTypeCombo_ = nullptr;
    QLineEdit* portTargetEdit_ = nullptr;

    QPlainTextEdit* validationConsole_ = nullptr;
    QPlainTextEdit* jobMonitor_ = nullptr;

    QString projectFilePath_;
};
