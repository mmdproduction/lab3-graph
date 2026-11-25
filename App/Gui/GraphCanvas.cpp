#include "GraphCanvas.hpp"
#include "Theme.hpp"

#include <QContextMenuEvent>
#include <QGraphicsObject>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {
QPointF Unit(QPointF vector) {
    const auto length = std::hypot(vector.x(), vector.y());
    return length < 0.001 ? QPointF(1, 0) : vector / length;
}
QColor Mix(QColor a, QColor b, double t) {
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}
}  // namespace
class GraphCanvas::NodeItem : public QGraphicsObject {
private:
    GraphCanvas& owner_;
    std::string id_;
    VertexVisual visual_;
    bool current_ = false;
    QColor fill_{"#263448"};
    QColor from_{"#263448"};
    QColor target_{"#263448"};
    QVariantAnimation fade_;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
        if (change == ItemPositionChange && scene()) {
            const auto p = value.toPointF();
            return QPointF(std::clamp(p.x(), -9000.0, 9000.0), std::clamp(p.y(), -9000.0, 9000.0));
        }
        if (change == ItemPositionHasChanged && owner_.document_) {
            owner_.document_->set_position(id_, pos());
            owner_.refresh_edges();
            if (owner_.position_changed) owner_.position_changed();
        }
        return QGraphicsObject::itemChange(change, value);
    }

public:
    NodeItem(GraphCanvas& owner, std::string id) : owner_(owner), id_(std::move(id)) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setZValue(2);
        setCursor(Qt::OpenHandCursor);
        setToolTip(QString::fromStdString(id_));
        fade_.setDuration(160);
        QObject::connect(&fade_, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
            fill_ = Mix(from_, target_, v.toDouble());
            update();
        });
    }
    const std::string& id() const { return id_; }
    QRectF boundingRect() const override { return {-45, -45, 90, 108}; }
    QPainterPath shape() const override {
        QPainterPath path;
        path.addEllipse(QRectF(-28, -28, 56, 56));
        return path;
    }
    void set_visual(VertexVisual visual, bool current) {
        visual_ = std::move(visual);
        current_ = current;
        target_ = visual_.color >= 0                       ? ui::Color(visual_.color)
                  : visual_.tone == VertexTone::Path       ? QColor("#486f66")
                  : visual_.tone == VertexTone::Error      ? QColor("#774b5b")
                  : visual_.tone == VertexTone::Visited    ? QColor("#3a526e")
                  : visual_.tone == VertexTone::Discovered ? QColor("#353d5a")
                                                           : QColor("#263448");
        if (fill_ != target_) {
            fade_.stop();
            from_ = fill_;
            fade_.setStartValue(0.0);
            fade_.setEndValue(1.0);
            fade_.start();
        }
        update();
    }
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override {
        p->setRenderHint(QPainter::Antialiasing);
        if (current_) {
            p->setBrush(QColor(205, 182, 142, 22));
            p->setPen(QPen(QColor("#d1b78e"), 1.5));
            p->drawEllipse(QRectF(-36, -36, 72, 72));
        }
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(0, 0, 0, 55));
        p->drawEllipse(QRectF(-28, -25, 56, 56));
        const QColor border = visual_.tone == VertexTone::Error ? QColor("#d298a6")
                              : current_                        ? QColor("#d8bf97")
                              : visual_.color >= 0 ? ui::Color(visual_.color).lighter(115)
                              : visual_.tone == VertexTone::Path ? QColor("#99c7b3")
                                                                 : QColor("#7088a5");
        p->setPen(QPen(border, isSelected() ? 2.8 : 1.6));
        p->setBrush(fill_);
        p->drawEllipse(QRectF(-28, -28, 56, 56));
        p->setPen(visual_.color >= 0 ? QColor("#192635") : QColor("#edf2fa"));
        QFont font("DejaVu Sans", 11, QFont::DemiBold);
        p->setFont(font);
        const QString text =
            p->fontMetrics().elidedText(QString::fromStdString(id_), Qt::ElideRight, 47);
        p->drawText(QRectF(-25, -15, 50, 30), Qt::AlignCenter, text);
        if (!visual_.caption.isEmpty()) {
            p->setFont(QFont("DejaVu Sans", 8));
            p->setPen(QColor("#aebed3"));
            p->drawText(QRectF(-44, 36, 88, 20), Qt::AlignHCenter | Qt::AlignTop, visual_.caption);
        }
    }
};
class GraphCanvas::EdgeItem : public QGraphicsItem {
private:
    EdgeRef ref_;
    NodeItem* from_;
    NodeItem* to_;
    bool directed_;
    double weight_;
    double offset_;
    QPainterPath path_;
    QRectF label_;
    EdgeTone tone_ = EdgeTone::Idle;
    bool examined_ = false;

public:
    EdgeItem(EdgeRef ref, NodeItem* from, NodeItem* to, bool directed, double weight, double offset)
        : ref_(std::move(ref)),
          from_(from),
          to_(to),
          directed_(directed),
          weight_(weight),
          offset_(offset) {
        setZValue(0);
        setFlag(ItemIsSelectable);
        refresh();
    }
    const EdgeRef& ref() const { return ref_; }
    void refresh() {
        prepareGeometryChange();
        const auto a = from_->pos(), b = to_->pos();
        path_ = QPainterPath();
        if (from_ == to_) {
            const double height = 70 + ref_.index * 30;
            path_.moveTo(a + QPointF(-20, -20));
            path_.cubicTo(a + QPointF(-75, -height), a + QPointF(75, -height),
                          a + QPointF(20, -20));
        } else {
            const auto delta = b - a;
            const auto normal = QPointF(-Unit(delta).y(), Unit(delta).x());
            const auto control = (a + b) / 2 + normal * offset_;
            const auto start = a + Unit(control - a) * 29, end = b - Unit(b - control) * 29;
            path_.moveTo(start);
            path_.quadTo(control, end);
        }
        const auto center = path_.pointAtPercent(0.5);
        const double width =
            std::max(38.0, QFontMetricsF(QFont("DejaVu Sans", 9))
                                   .horizontalAdvance(QString::number(weight_, 'g', 4)) +
                               16);
        label_ = QRectF(center.x() - width / 2, center.y() - 11, width, 22);
        update();
    }
    QRectF boundingRect() const override {
        return path_.boundingRect().united(label_).adjusted(-12, -12, 12, 12);
    }
    QPainterPath shape() const override {
        QPainterPathStroker stroker;
        stroker.setWidth(15);
        auto result = stroker.createStroke(path_);
        result.addRect(label_);
        return result;
    }
    void set_visual(EdgeTone tone, bool examined) {
        tone_ = tone;
        examined_ = examined;
        setZValue(examined ? 1 : 0);
        update();
    }
    void paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override {
        p->setRenderHint(QPainter::Antialiasing);
        QColor color = examined_                     ? QColor("#d1b78e")
                       : tone_ == EdgeTone::Accepted ? QColor("#9bc8b5")
                       : tone_ == EdgeTone::Rejected ? QColor("#8c6978")
                       : tone_ == EdgeTone::Error    ? QColor("#d298a6")
                                                     : QColor("#596c86");
        p->setPen(QPen(color, examined_ || tone_ == EdgeTone::Accepted ? 2.8 : 1.6,
                       tone_ == EdgeTone::Rejected ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap));
        p->setBrush(Qt::NoBrush);
        p->drawPath(path_);
        if (directed_) {
            const auto tip = path_.pointAtPercent(1),
                       direction = Unit(tip - path_.pointAtPercent(0.97)),
                       normal = QPointF(-direction.y(), direction.x());
            QPolygonF arrow;
            arrow << tip << tip - direction * 11 + normal * 4.5
                  << tip - direction * 11 - normal * 4.5;
            p->setPen(Qt::NoPen);
            p->setBrush(color);
            p->drawPolygon(arrow);
        }
        p->setPen(QPen(examined_ ? QColor("#76664e") : QColor("#303c4d"), 1));
        p->setBrush(QColor("#1a2330"));
        p->drawRoundedRect(label_, 6, 6);
        p->setPen(examined_ ? QColor("#e4cda8") : QColor("#bbcadf"));
        p->setFont(QFont("DejaVu Sans", 9));
        p->drawText(label_, Qt::AlignCenter, QString::number(weight_, 'g', 4));
    }
};
GraphCanvas::GraphCanvas(QWidget* parent) : QGraphicsView(parent) {
    setObjectName("graphCanvas");
    setScene(new QGraphicsScene(this));
    setFrameShape(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setViewportUpdateMode(FullViewportUpdate);
    setBackgroundBrush(QColor("#131923"));
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);
    setDragMode(RubberBandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(440, 300);
}
GraphCanvas::NodeItem* GraphCanvas::find_node(const std::string& id) const {
    for (std::size_t i = 0; i < nodes_.size(); ++i)
        if (nodes_.get(i)->id() == id) return nodes_.get(i);
    return nullptr;
}
void GraphCanvas::refresh_edges() {
    for (std::size_t i = 0; i < edges_.size(); ++i) edges_.get(i)->refresh();
    viewport()->update();
}
void GraphCanvas::rebuild(GraphDocument& document) {
    document_ = nullptr;
    nodes_.clear();
    edges_.clear();
    scene()->clear();
    const auto ids = document.graph().vertex_ids();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        auto* node = new NodeItem(*this, ids.get(i));
        node->setPos(document.position(ids.get(i)));
        scene()->addItem(node);
        nodes_.append(node);
    }
    const auto keys = document.graph().edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys.get(i);
        const auto& weights = document.graph().get_edge_weights(key.from, key.to);
        const bool reverse = document.graph().is_directed() && key.from != key.to &&
                             document.graph().contains_edge(key.to, key.from);
        for (std::size_t j = 0; j < weights.size(); ++j) {
            const double offset =
                reverse ? 55.0 + j * 55.0
                        : (static_cast<double>(j) - (weights.size() - 1) / 2.0) * 65.0;
            auto* edge = new EdgeItem({key.from, key.to, j}, find_node(key.from), find_node(key.to),
                                      document.graph().is_directed(), weights.get(j), offset);
            scene()->addItem(edge);
            edges_.append(edge);
        }
    }
    document_ = &document;
    set_edge_mode(edge_mode_);
    scene()->setSceneRect(scene()
                              ->itemsBoundingRect()
                              .united(QRectF(-440, -300, 880, 600))
                              .adjusted(-90, -70, 90, 70));
}
void GraphCanvas::apply(const PlaybackState& state) {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        auto* node = nodes_.get(i);
        node->set_visual(state.vertices.contains_key(node->id()) ? state.vertices.get(node->id())
                                                                 : VertexVisual{},
                         state.current && *state.current == node->id());
    }
    for (std::size_t i = 0; i < edges_.size(); ++i) {
        auto* edge = edges_.get(i);
        EdgeTone tone = EdgeTone::Idle;
        for (std::size_t j = 0; j < state.edges.size(); ++j) {
            const auto& visual = state.edges.get(j);
            if (visual.ref.from == edge->ref().from && visual.ref.to == edge->ref().to &&
                visual.ref.index == edge->ref().index) {
                tone = visual.tone;
                break;
            }
        }
        bool examined = false;
        if (state.examined && document_) {
            const auto key = MakeEdgeKey(state.examined->from, state.examined->to,
                                         document_->graph().is_directed());
            examined = key.from == edge->ref().from && key.to == edge->ref().to &&
                       (state.examined->index == kAllParallelEdges ||
                        state.examined->index == edge->ref().index);
        }
        edge->set_visual(tone, examined);
    }
}
void GraphCanvas::set_edge_mode(bool enabled) {
    edge_mode_ = enabled;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        nodes_.get(i)->setFlag(QGraphicsItem::ItemIsMovable, !enabled);
        nodes_.get(i)->setCursor(enabled ? Qt::CrossCursor : Qt::OpenHandCursor);
    }
}
void GraphCanvas::fit_graph() {
    scene()->setSceneRect(scene()
                              ->itemsBoundingRect()
                              .united(QRectF(-440, -300, 880, 600))
                              .adjusted(-90, -70, 90, 70));
    resetTransform();
    fitInView(scene()
                  ->itemsBoundingRect()
                  .united(QRectF(-380, -230, 760, 460))
                  .adjusted(-65, -60, 65, 65),
              Qt::KeepAspectRatio);
}
void GraphCanvas::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor("#131923"));
    painter->setPen(QPen(QColor("#27303d"), 0));
    constexpr int grid = 28;
    for (double x = std::floor(rect.left() / grid) * grid; x < rect.right(); x += grid)
        for (double y = std::floor(rect.top() / grid) * grid; y < rect.bottom(); y += grid)
            painter->drawPoint(QPointF(x, y));
}
void GraphCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !itemAt(event->pos())) {
        if (create_vertex) create_vertex(mapToScene(event->pos()));
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}
void GraphCanvas::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        if (auto* node = dynamic_cast<NodeItem*>(itemAt(event->pos()))) {
            if (click_vertex) click_vertex(node->id());
            if (edge_mode_) {
                event->accept();
                return;
            }
        }
    QGraphicsView::mousePressEvent(event);
}
void GraphCanvas::contextMenuEvent(QContextMenuEvent* event) {
    if (!context) return;
    const auto* item = itemAt(event->pos());
    if (const auto* node = dynamic_cast<const NodeItem*>(item))
        context(node->id(), {}, mapToScene(event->pos()), event->globalPos());
    else if (const auto* edge = dynamic_cast<const EdgeItem*>(item))
        context({}, edge->ref(), mapToScene(event->pos()), event->globalPos());
    else
        context({}, {}, mapToScene(event->pos()), event->globalPos());
}
void GraphCanvas::wheelEvent(QWheelEvent* event) {
    const double factor = event->angleDelta().y() > 0 ? 1.12 : 1 / 1.12;
    if (transform().m11() * factor > 0.2 && transform().m11() * factor < 3.5) scale(factor, factor);
    event->accept();
}
