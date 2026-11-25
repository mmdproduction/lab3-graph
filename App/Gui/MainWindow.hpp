#pragma once

#include <QMainWindow>
#include "GraphDocument.hpp"
#include "TraceRunner.hpp"

class QComboBox;
class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QButtonGroup;
class GraphCanvas;

class MainWindow : public QMainWindow {
private:
    GraphDocument document_;
    TraceRunner runner_;
    Algorithm algorithm_ = Algorithm::Dijkstra;
    GraphCanvas* canvas_ = nullptr;
    QTimer* timer_ = nullptr;
    QButtonGroup* algorithms_ = nullptr;
    QComboBox* examples_ = nullptr;
    QComboBox* start_ = nullptr;
    QComboBox* finish_ = nullptr;
    QComboBox* speed_ = nullptr;
    QLabel* title_ = nullptr;
    QLabel* description_ = nullptr;
    QLabel* explanation_ = nullptr;
    QLabel* step_label_ = nullptr;
    QLabel* graph_info_ = nullptr;
    QLabel* state_label_ = nullptr;
    QPushButton* play_ = nullptr;
    QPushButton* edge_tool_ = nullptr;
    QSlider* timeline_ = nullptr;
    std::string edge_from_;
    QString file_path_;
    bool dirty_ = false;
    int example_index_ = 0;
    void build_ui();
    void refresh_document(bool fit = false);
    void invalidate_trace();
    void show_frame();
    void play_pause();
    void add_vertex(QPointF position);
    void node_clicked(const std::string& id);
    void show_context(const std::string& vertex, const std::optional<EdgeRef>& edge,
                      QPointF position, QPoint global);
    void open_file();
    void save_file();
    void new_graph();
    bool discard_changes();
    void report_error(const std::exception& error);

protected:
    void closeEvent(QCloseEvent* event) override;

public:
    explicit MainWindow(QWidget* parent = nullptr);
    const GraphDocument& document() const noexcept { return document_; }
    const TraceRunner& runner() const noexcept { return runner_; }
    void load_example(int index);
    void choose_algorithm(Algorithm algorithm);
    void prepare_trace();
    void seek(std::size_t step);
};
