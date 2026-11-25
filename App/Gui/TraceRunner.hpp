#pragma once

#include <QString>
#include <optional>
#include "GraphDocument.hpp"

enum class Algorithm { BFS, DFS, Dijkstra, BellmanFord, Topological, Kruskal, Coloring };
enum class VertexTone { Idle, Discovered, Visited, Path, Error };
enum class EdgeTone { Idle, Accepted, Rejected, Error };
struct VertexVisual {
    VertexTone tone = VertexTone::Idle;
    int color = -1;
    QString caption;
};
struct EdgeVisual {
    EdgeRef ref;
    EdgeTone tone = EdgeTone::Idle;
};
struct PlaybackState {
    AvlDictionary<std::string, VertexVisual> vertices;
    ArraySequence<EdgeVisual> edges;
    std::optional<std::string> current;
    std::optional<EdgeRef> examined;
    QString explanation;
    QString summary;
    bool is_error = false;
    std::size_t color_count = 0;
};

class TraceRunner {
private:
    struct RecordedStep {
        GraphStep<std::string, double> event;
        QString error;
    };
    GuiGraph graph_{false};
    ArraySequence<RecordedStep> steps_;
    Algorithm algorithm_ = Algorithm::BFS;
    std::size_t cursor_ = 0;
    bool has_error_ = false;
    QString result_;
    QString describe(const GraphStep<std::string, double>& step) const;

public:
    void prepare(const GuiGraph& graph, Algorithm algorithm, const std::string& start,
                 const std::string& finish);
    void clear();
    void seek(std::size_t cursor) noexcept;
    std::size_t cursor() const noexcept { return cursor_; }
    std::size_t size() const noexcept { return steps_.size(); }
    bool has_error() const noexcept { return has_error_; }
    PlaybackState state() const;
    static QString name(Algorithm algorithm);
    static QString description(Algorithm algorithm);
};
