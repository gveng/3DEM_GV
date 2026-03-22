#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QColor>
#include <QHash>
#include <QMouseEvent>
#include <QPoint>
#include <QRectF>
#include <QString>

#include <vector>

class ViewportWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    struct DrawableObject {
        QString id;
        QString name;
        QRectF normalizedRect;
        QColor color;
        bool visible = true;
        bool selected = false;
    };

    explicit ViewportWidget(QWidget* parent = nullptr);

    void setObjects(const std::vector<DrawableObject>& objects);

signals:
    void objectClicked(const QString& objectId, bool additiveSelection);
    void contextMenuRequestedAt(const QPoint& globalPos);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QRectF toCanvasRect(const QRectF& normalized) const;

    std::vector<DrawableObject> objects_;
    std::vector<QString> drawOrder_;
    QHash<QString, QRectF> paintedRects_;
};
