#include "ViewportWidget.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

ViewportWidget::ViewportWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(640, 420);
}

void ViewportWidget::setObjects(const std::vector<DrawableObject>& objects) {
    objects_ = objects;
    update();
}

void ViewportWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.88f, 0.93f, 0.95f, 1.0f);
}

void ViewportWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect full = rect();
    QLinearGradient bg(0.0, 0.0, static_cast<double>(full.width()), static_cast<double>(full.height()));
    bg.setColorAt(0.0, QColor(225, 240, 246));
    bg.setColorAt(1.0, QColor(252, 243, 229));
    painter.fillRect(full, bg);

    painter.setPen(QPen(QColor(190, 200, 205), 1));
    const int gridStep = 28;
    for (int x = 0; x < full.width(); x += gridStep) {
        painter.drawLine(x, 0, x, full.height());
    }
    for (int y = 0; y < full.height(); y += gridStep) {
        painter.drawLine(0, y, full.width(), y);
    }

    drawOrder_.clear();
    paintedRects_.clear();

    for (const auto& object : objects_) {
        if (!object.visible) {
            continue;
        }

        const QRectF box = toCanvasRect(object.normalizedRect);
        drawOrder_.push_back(object.id);
        paintedRects_[object.id] = box;

        painter.setPen(QPen(object.selected ? QColor(6, 92, 124) : QColor(58, 88, 104), object.selected ? 3 : 1));
        painter.setBrush(QBrush(object.color));
        painter.drawRoundedRect(box, 8.0, 8.0);

        painter.setPen(QPen(QColor(25, 33, 38)));
        painter.drawText(box.adjusted(8, 8, -8, -8), Qt::AlignTop | Qt::AlignLeft, object.name);
    }

    painter.setPen(QPen(QColor(60, 70, 76), 1));
    painter.setBrush(QBrush(QColor(255, 255, 255, 230)));
    painter.drawRoundedRect(QRectF(10, 10, 120, 28), 6.0, 6.0);
    painter.drawText(QRectF(10, 10, 120, 28), Qt::AlignCenter, "3D Viewport");

    painter.end();
}

void ViewportWidget::resizeGL(int, int) {
}

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        emit contextMenuRequestedAt(event->globalPosition().toPoint());
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }

    const QPointF pos = event->position();
    const bool additive = event->modifiers().testFlag(Qt::ControlModifier);

    for (auto it = drawOrder_.rbegin(); it != drawOrder_.rend(); ++it) {
        const auto rectIt = paintedRects_.find(*it);
        if (rectIt == paintedRects_.end()) {
            continue;
        }

        if (rectIt.value().contains(pos)) {
            emit objectClicked(*it, additive);
            event->accept();
            return;
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

QRectF ViewportWidget::toCanvasRect(const QRectF& normalized) const {
    const double margin = 40.0;
    const double usableW = std::max(1.0, width() - 2.0 * margin);
    const double usableH = std::max(1.0, height() - 2.0 * margin);

    const double x = margin + normalized.x() * usableW;
    const double y = margin + normalized.y() * usableH;
    const double w = normalized.width() * usableW;
    const double h = normalized.height() * usableH;

    return QRectF(x, y, w, h);
}
