#pragma once

#include <QGraphicsView>
#include <functional>
#include "GraphDocument.hpp"
#include "TraceRunner.hpp"

class GraphCanvas : public QGraphicsView {
private:
    class NodeItem;
    class EdgeItem;
    GraphDocument* document_ = nullptr;
    ArraySequence<NodeItem*> nodes_;
    ArraySequence<EdgeItem*> edges_;
    bool edge_mode_ = false;
    void refresh_edges();
    NodeItem* find_node(const std::string& id) const;

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

public:
    std::function<void(QPointF)> create_vertex;
    std::function<void(const std::string&)> click_vertex;
    std::function<void(const std::string&, const std::optional<EdgeRef>&, QPointF, QPoint)> context;
    std::function<void()> position_changed;
    explicit GraphCanvas(QWidget* parent = nullptr);
    void rebuild(GraphDocument& document);
    void apply(const PlaybackState& state);
    void set_edge_mode(bool enabled);
    void fit_graph();
};
