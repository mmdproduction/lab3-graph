#include "TraceRunner.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
QString Id(const std::string& id) { return QString::fromStdString(id); }
QString Number(double value) { return QString::number(value, 'g', 7); }
QString Join(const ArraySequence<std::string>& ids) {
    QString result;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i) result += " → ";
        result += Id(ids.get(i));
    }
    return result;
}
QString ErrorText(const std::exception& error) {
    const std::string text = error.what();
    if (text == "Dijkstra requires nonnegative weights")
        return "Дейкстра требует неотрицательных весов. Для этого графа выберите Беллмана–Форда.";
    if (text == "topological sort requires a directed graph")
        return "Топологическая сортировка работает с ориентированным графом. Откройте пример "
               "«Зависимости».";
    if (text == "Kruskal requires an undirected graph")
        return "Краскал работает с неориентированным графом. Откройте пример «Взвешенная сеть».";
    if (text == "reachable negative cycle")
        return "Обнаружен достижимый отрицательный цикл. Кратчайший путь не определён.";
    if (text == "graph contains a directed cycle")
        return "В графе есть ориентированный цикл: топологический порядок построить нельзя.";
    if (text == "a graph with a self-loop cannot be properly colored")
        return "У вершины есть петля. Правильная раскраска такого графа невозможна.";
    if (text == "vertex not found") return "Выберите существующую начальную и конечную вершины.";
    if (text == "path weight overflow") return "Сумма весов вышла за допустимый диапазон.";
    return QString::fromUtf8(error.what());
}
}  // namespace
QString TraceRunner::name(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::BFS:
            return "Обход в ширину";
        case Algorithm::DFS:
            return "Обход в глубину";
        case Algorithm::Dijkstra:
            return "Дейкстра";
        case Algorithm::BellmanFord:
            return "Беллман–Форд";
        case Algorithm::Topological:
            return "Топологическая сортировка";
        case Algorithm::Kruskal:
            return "Краскал";
        case Algorithm::Coloring:
            return "Жадная раскраска";
    }
    return {};
}
QString TraceRunner::description(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::BFS:
            return "Исследуем граф по слоям, начиная с выбранной вершины.";
        case Algorithm::DFS:
            return "Углубляемся по связям, затем возвращаемся к оставшимся ветвям.";
        case Algorithm::Dijkstra:
            return "Находим кратчайший путь при неотрицательных весах рёбер.";
        case Algorithm::BellmanFord:
            return "Улучшаем расстояния проходами по рёбрам. Допускаются отрицательные веса.";
        case Algorithm::Topological:
            return "Выстраиваем зависимости: начало каждого ребра идёт раньше его конца.";
        case Algorithm::Kruskal:
            return "Соединяем компоненты самыми дешёвыми рёбрами, не создавая циклы.";
        case Algorithm::Coloring:
            return "Каждой вершине назначаем первый цвет, свободный у её соседей.";
    }
    return {};
}
void TraceRunner::clear() {
    steps_.clear();
    cursor_ = 0;
    has_error_ = false;
    result_.clear();
    graph_.clear();
}
void TraceRunner::seek(std::size_t cursor) noexcept { cursor_ = std::min(cursor, steps_.size()); }
void TraceRunner::prepare(const GuiGraph& graph, Algorithm algorithm, const std::string& start,
                          const std::string& finish) {
    clear();
    graph_ = graph;
    algorithm_ = algorithm;
    const auto observer = [&](const GraphStep<std::string, double>& step) {
        if (steps_.size() >= 30000)
            throw std::length_error(
                "История достигла 30 000 шагов. Уменьшите граф для наглядного воспроизведения.");
        steps_.append({step, {}});
    };
    try {
        switch (algorithm) {
            case Algorithm::BFS:
                result_ = "Порядок обхода: " + Join(graph_.BFS(start, observer));
                break;
            case Algorithm::DFS:
                result_ = "Порядок обхода: " + Join(graph_.DFS(start, observer));
                break;
            case Algorithm::Dijkstra:
            case Algorithm::BellmanFord: {
                const auto path = algorithm == Algorithm::Dijkstra
                                      ? graph_.Dijkstra(start, finish, observer)
                                      : graph_.BellmanFord(start, finish, observer);
                result_ =
                    path.reachable
                        ? "Путь: " + Join(path.vertices) + " · вес " + Number(path.total_weight)
                        : "Вершина " + Id(finish) + " недостижима из " + Id(start) + ".";
                break;
            }
            case Algorithm::Topological:
                result_ = "Порядок: " + Join(graph_.TopologicalSort(observer));
                break;
            case Algorithm::Kruskal: {
                const auto forest = graph_.Kruskal(observer);
                result_ = QString("Остовный лес · вес %1 · рёбер %2 · компонент %3")
                              .arg(Number(forest.total_weight))
                              .arg(forest.edges.size())
                              .arg(forest.component_count);
                break;
            }
            case Algorithm::Coloring: {
                const auto coloring = graph_.GreedyColoring(observer);
                result_ = QString("Раскраска завершена · цветов: %1. Соседи имеют разные цвета.")
                              .arg(coloring.color_count);
                break;
            }
        }
    } catch (const std::exception& error) {
        has_error_ = true;
        result_ = ErrorText(error);
        steps_.append({{}, result_});
    }
}
QString TraceRunner::describe(const GraphStep<std::string, double>& step) const {
    switch (step.kind) {
        case GraphStepKind::Begin:
            return "Начинаем: " + name(algorithm_) +
                   ". Следите за подсвеченными вершинами и рёбрами.";
        case GraphStepKind::DiscoverVertex:
            return "Вершина " + Id(step.vertex) + " найдена и ожидает обработки.";
        case GraphStepKind::VisitVertex:
            if (algorithm_ == Algorithm::Topological)
                return "Добавляем " + Id(step.vertex) +
                       " в порядок: все её входящие зависимости сняты.";
            return "Текущая вершина — " + Id(step.vertex) + ". Рассматриваем её связи.";
        case GraphStepKind::FinishVertex:
            return "Обработка вершины " + Id(step.vertex) + " завершена.";
        case GraphStepKind::ExamineEdge:
            return "Рассматриваем связь " + Id(step.from) + " → " + Id(step.to) +
                   (step.edge_index == kAllParallelEdges
                        ? "."
                        : " с весом " +
                              Number(graph_.get_edge_weight(step.from, step.to, step.edge_index)) +
                              ".");
        case GraphStepKind::UpdateDistance:
            return "Расстояние до " + Id(step.vertex) + " теперь " + Number(step.value) + ".";
        case GraphStepKind::BeginPass:
            return QString("Проход %1: проверяем, можно ли улучшить расстояния по рёбрам.")
                .arg(step.number);
        case GraphStepKind::AcceptEdge:
            return "Добавляем " + Id(step.from) + " — " + Id(step.to) +
                   ": оно соединяет разные компоненты.";
        case GraphStepKind::RejectEdge:
            return "Пропускаем " + Id(step.from) + " — " + Id(step.to) +
                   ": это ребро создало бы цикл.";
        case GraphStepKind::AssignColor:
            return QString("Вершина %1 получает цвет %2 — минимальный свободный у соседей.")
                .arg(Id(step.vertex))
                .arg(step.number + 1);
        case GraphStepKind::PathVertex:
            return "Восстанавливаем путь: вершина " + Id(step.vertex) + " входит в результат.";
        case GraphStepKind::PathEdge:
            return "Выбираем ребро " + Id(step.from) + " → " + Id(step.to) +
                   " для кратчайшего пути.";
        case GraphStepKind::Unreachable:
            return "До вершины " + Id(step.vertex) + " нет пути из выбранного начала.";
        case GraphStepKind::Cycle:
            return algorithm_ == Algorithm::Coloring
                       ? "Обнаружена петля: вершину нельзя раскрасить правильно."
                   : algorithm_ == Algorithm::BellmanFord
                       ? "Ребро указывает на достижимый отрицательный цикл."
                       : "Оставшиеся зависимости содержат цикл; порядок невозможен.";
        case GraphStepKind::Complete:
            return result_;
    }
    return {};
}
PlaybackState TraceRunner::state() const {
    PlaybackState result;
    result.explanation = steps_.size()
                             ? "Готово к запуску. Нажмите «Старт» или передвиньте ползунок шагов."
                             : "Выберите алгоритм и нажмите «Старт».";
    const auto ids = graph_.vertex_ids();
    for (std::size_t i = 0; i < ids.size(); ++i) result.vertices.add(ids.get(i), {});
    const auto keys = graph_.edge_keys();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys.get(i);
        const auto& weights = graph_.get_edge_weights(key.from, key.to);
        for (std::size_t j = 0; j < weights.size(); ++j)
            result.edges.append({{key.from, key.to, j}, EdgeTone::Idle});
    }
    const auto mark_edge = [&](const GraphStep<std::string, double>& step, EdgeTone tone) {
        const auto key = MakeEdgeKey(step.from, step.to, graph_.is_directed());
        for (std::size_t i = 0; i < result.edges.size(); ++i) {
            auto& edge = result.edges.get(i);
            if (edge.ref.from == key.from && edge.ref.to == key.to &&
                (step.edge_index == kAllParallelEdges || step.edge_index == edge.ref.index))
                edge.tone = tone;
        }
    };
    for (std::size_t i = 0; i < cursor_; ++i) {
        const auto& recorded = steps_.get(i);
        const auto& step = recorded.event;
        result.examined.reset();
        if (!recorded.error.isEmpty()) {
            result.explanation = recorded.error;
            result.summary = recorded.error;
            result.is_error = true;
            result.current.reset();
            continue;
        }
        result.explanation = describe(step);
        switch (step.kind) {
            case GraphStepKind::DiscoverVertex:
                if (result.vertices.get(step.vertex).tone == VertexTone::Idle)
                    result.vertices.get(step.vertex).tone = VertexTone::Discovered;
                break;
            case GraphStepKind::VisitVertex:
                result.current = step.vertex;
                result.vertices.get(step.vertex).tone = VertexTone::Visited;
                if (algorithm_ == Algorithm::Topological)
                    result.vertices.get(step.vertex).caption = QString("№ %1").arg(step.number);
                break;
            case GraphStepKind::FinishVertex:
                result.current.reset();
                break;
            case GraphStepKind::ExamineEdge:
                result.examined = EdgeRef{step.from, step.to, step.edge_index};
                if (algorithm_ != Algorithm::Coloring) result.current = step.from;
                break;
            case GraphStepKind::UpdateDistance:
                result.current = step.vertex;
                result.vertices.get(step.vertex).caption = "d = " + Number(step.value);
                if (result.vertices.get(step.vertex).tone == VertexTone::Idle)
                    result.vertices.get(step.vertex).tone = VertexTone::Discovered;
                break;
            case GraphStepKind::AcceptEdge:
            case GraphStepKind::PathEdge:
                mark_edge(step, EdgeTone::Accepted);
                break;
            case GraphStepKind::RejectEdge:
                mark_edge(step, EdgeTone::Rejected);
                break;
            case GraphStepKind::AssignColor:
                result.vertices.get(step.vertex).color = static_cast<int>(step.number);
                result.vertices.get(step.vertex).caption = QString("Цвет %1").arg(step.number + 1);
                result.current = step.vertex;
                result.color_count = std::max(result.color_count, step.number + 1);
                break;
            case GraphStepKind::PathVertex:
                result.vertices.get(step.vertex).tone = VertexTone::Path;
                result.current = step.vertex;
                break;
            case GraphStepKind::Cycle:
                if (result.vertices.contains_key(step.vertex))
                    result.vertices.get(step.vertex).tone = VertexTone::Error;
                mark_edge(step, EdgeTone::Error);
                result.current = step.vertex;
                break;
            case GraphStepKind::BeginPass:
                result.current.reset();
                break;
            case GraphStepKind::Complete:
                result.current.reset();
                result.summary = result_;
                break;
            default:
                break;
        }
    }
    return result;
}
