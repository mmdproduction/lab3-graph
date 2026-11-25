#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <string>
#include <cstdint>
#include "Graph/Graph.hpp"

using GuiGraph = Graph<std::string, std::string, double>;

struct EdgeRef {
    std::string from;
    std::string to;
    std::size_t index = 0;
};

class GraphDocument {
private:
    GuiGraph graph_{false};
    AvlDictionary<std::string, QPointF> positions_;

public:
    static constexpr std::size_t kMaxVertices = 50;
    static constexpr std::size_t kMaxEdges = 150;
    const GuiGraph& graph() const noexcept { return graph_; }
    QPointF position(const std::string& id) const { return positions_.get(id); }
    void reset(bool directed);
    void add_vertex(const std::string& id, QPointF position);
    void rename_vertex(const std::string& id, const std::string& name);
    void remove_vertex(const std::string& id);
    void set_position(const std::string& id, QPointF position);
    void add_edge(const std::string& from, const std::string& to, double weight);
    void remove_edge(const EdgeRef& edge);
    void set_weight(const EdgeRef& edge, double weight);
    void load_example(int index);
    static std::size_t max_generated_edges(std::size_t vertices, bool directed);
    void generate(std::size_t vertices, std::size_t edges, bool directed, std::uint32_t seed);
    QByteArray to_json() const;
    void load_json(const QByteArray& bytes);
    void swap(GraphDocument& other) noexcept;
};
