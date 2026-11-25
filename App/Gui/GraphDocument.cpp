#include "GraphDocument.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {
void ValidateName(const std::string& id) {
    const auto name = QString::fromStdString(id);
    if (name.trimmed().isEmpty() || name.size() > 32 || id.find('\0') != std::string::npos)
        throw std::invalid_argument("Имя вершины должно содержать от 1 до 32 символов.");
}
void ValidatePosition(QPointF p) {
    if (!std::isfinite(p.x()) || !std::isfinite(p.y()) || std::abs(p.x()) > 10000 ||
        std::abs(p.y()) > 10000)
        throw std::invalid_argument(
            "Координаты должны быть конечными и находиться в диапазоне ±10 000.");
}
void ValidateWeight(double weight) {
    if (!std::isfinite(weight))
        throw std::invalid_argument("Вес ребра должен быть конечным числом.");
}
}  // namespace
void GraphDocument::swap(GraphDocument& other) noexcept {
    graph_.swap(other.graph_);
    positions_.swap(other.positions_);
}
void GraphDocument::reset(bool directed) {
    GraphDocument replacement;
    GuiGraph empty(directed);
    replacement.graph_.swap(empty);
    swap(replacement);
}
void GraphDocument::add_vertex(const std::string& id, QPointF position) {
    ValidateName(id);
    ValidatePosition(position);
    if (graph_.contains_vertex(id))
        throw std::invalid_argument("Вершина с таким именем уже существует.");
    if (graph_.vertex_count() >= kMaxVertices)
        throw std::invalid_argument("Для наглядной анимации допускается не более 50 вершин.");
    GraphDocument replacement(*this);
    replacement.graph_.add_vertex(id, {});
    replacement.positions_.add(id, position);
    swap(replacement);
}
void GraphDocument::set_position(const std::string& id, QPointF position) {
    ValidatePosition(position);
    positions_.get(id) = position;
}
void GraphDocument::rename_vertex(const std::string& id, const std::string& name) {
    ValidateName(name);
    if (!graph_.contains_vertex(id)) throw std::out_of_range("Вершина не найдена.");
    if (id == name) return;
    if (graph_.contains_vertex(name))
        throw std::invalid_argument("Вершина с таким именем уже существует.");
    GraphDocument replacement;
    replacement.reset(graph_.is_directed());
    const auto ids = graph_.vertex_ids();
    for (std::size_t i = 0; i < ids.size(); ++i)
        replacement.add_vertex(ids.get(i) == id ? name : ids.get(i), position(ids.get(i)));
    const auto keys = graph_.edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys.get(i);
        const auto& weights = graph_.get_edge_weights(key.from, key.to);
        for (std::size_t j = 0; j < weights.size(); ++j)
            replacement.add_edge(key.from == id ? name : key.from, key.to == id ? name : key.to,
                                 weights.get(j));
    }
    swap(replacement);
}
void GraphDocument::remove_vertex(const std::string& id) {
    GraphDocument replacement(*this);
    replacement.graph_.remove_vertex(id);
    replacement.positions_.remove(id);
    swap(replacement);
}
void GraphDocument::add_edge(const std::string& from, const std::string& to, double weight) {
    ValidateWeight(weight);
    if (graph_.edge_count() >= kMaxEdges)
        throw std::invalid_argument("Для наглядной анимации допускается не более 150 рёбер.");
    graph_.add_edge(from, to, weight);
}
void GraphDocument::remove_edge(const EdgeRef& edge) {
    graph_.remove_edge(edge.from, edge.to, edge.index);
}
void GraphDocument::set_weight(const EdgeRef& edge, double weight) {
    ValidateWeight(weight);
    graph_.set_edge_weight(edge.from, edge.to, edge.index, weight);
}
void GraphDocument::load_example(int index) {
    if (index < 0 || index > 4) throw std::invalid_argument("Неизвестный пример.");
    GraphDocument replacement;
    replacement.reset(index == 1 || index == 2 || index == 3);
    const QPointF points[] = {{-300, 0},   {-150, -150}, {-150, 150},
                              {100, -150}, {100, 150},   {300, 0}};
    for (int i = 0; i < 6; ++i)
        replacement.add_vertex(std::string(1, static_cast<char>('A' + i)), points[i]);
    const auto edge = [&](const char* from, const char* to, double weight) {
        replacement.add_edge(from, to, weight);
    };
    if (index == 0) {
        edge("A", "B", 4);
        edge("A", "C", 2);
        edge("B", "C", 1);
        edge("B", "D", 5);
        edge("C", "E", 6);
        edge("D", "E", 2);
        edge("D", "F", 6);
        edge("E", "F", 3);
    } else if (index == 1) {
        edge("A", "B", 2);
        edge("A", "C", 3);
        edge("B", "D", 1);
        edge("C", "D", 4);
        edge("C", "E", 2);
        edge("D", "F", 3);
        edge("E", "F", 1);
    } else if (index == 2) {
        edge("A", "B", 4);
        edge("A", "C", 5);
        edge("B", "C", -2);
        edge("B", "D", 3);
        edge("C", "E", 2);
        edge("E", "D", -1);
        edge("D", "F", 2);
        edge("E", "F", 5);
    } else if (index == 3) {
        edge("A", "B", 2);
        edge("A", "B", 5);
        edge("B", "A", 3);
        edge("B", "B", 1);
        edge("B", "D", 4);
        edge("A", "C", 3);
        edge("C", "E", 1);
        edge("E", "F", 2);
        edge("D", "F", 1);
    } else {
        edge("A", "B", 1);
        edge("A", "C", 1);
        edge("B", "C", 1);
        edge("B", "D", 1);
        edge("C", "E", 1);
        edge("D", "E", 1);
        edge("D", "F", 1);
        edge("E", "F", 1);
    }
    swap(replacement);
}
QByteArray GraphDocument::to_json() const {
    QJsonArray vertices, edges;
    const auto ids = graph_.vertex_ids();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto p = position(ids.get(i));
        vertices.append(
            QJsonObject{{"id", QString::fromStdString(ids.get(i))}, {"x", p.x()}, {"y", p.y()}});
    }
    const auto keys = graph_.edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys.get(i);
        const auto& weights = graph_.get_edge_weights(key.from, key.to);
        for (std::size_t j = 0; j < weights.size(); ++j)
            edges.append(QJsonObject{{"from", QString::fromStdString(key.from)},
                                     {"to", QString::fromStdString(key.to)},
                                     {"weight", weights.get(j)}});
    }
    return QJsonDocument(QJsonObject{{"schema", 1},
                                     {"directed", graph_.is_directed()},
                                     {"vertices", vertices},
                                     {"edges", edges}})
        .toJson();
}
void GraphDocument::load_json(const QByteArray& bytes) {
    if (bytes.size() > 2 * 1024 * 1024)
        throw std::invalid_argument("Файл графа слишком большой (максимум 2 МБ).");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    const auto object = document.object();
    if (error.error != QJsonParseError::NoError || !document.isObject() ||
        object.value("schema").toInt(-1) != 1 || !object.value("directed").isBool() ||
        !object.value("vertices").isArray() || !object.value("edges").isArray())
        throw std::invalid_argument("Некорректный формат файла графа.");
    const auto vertices = object.value("vertices").toArray(),
               edges = object.value("edges").toArray();
    if (vertices.size() > static_cast<int>(kMaxVertices) ||
        edges.size() > static_cast<int>(kMaxEdges))
        throw std::invalid_argument("Граф превышает ограничение: 50 вершин и 150 рёбер.");
    GraphDocument replacement;
    replacement.reset(object.value("directed").toBool());
    for (const auto& value : vertices) {
        const auto vertex = value.toObject();
        if (!value.isObject() || !vertex.value("id").isString() || !vertex.value("x").isDouble() ||
            !vertex.value("y").isDouble())
            throw std::invalid_argument("Некорректная вершина в файле.");
        replacement.add_vertex(vertex.value("id").toString().toStdString(),
                               {vertex.value("x").toDouble(), vertex.value("y").toDouble()});
    }
    for (const auto& value : edges) {
        const auto edge = value.toObject();
        if (!value.isObject() || !edge.value("from").isString() || !edge.value("to").isString() ||
            !edge.value("weight").isDouble())
            throw std::invalid_argument("Некорректное ребро в файле.");
        replacement.add_edge(edge.value("from").toString().toStdString(),
                             edge.value("to").toString().toStdString(),
                             edge.value("weight").toDouble());
    }
    swap(replacement);
}
