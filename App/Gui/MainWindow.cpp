#include "MainWindow.hpp"
#include "GraphCanvas.hpp"
#include "Theme.hpp"

#include <QAction>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QPainter>
#include <QPaintEvent>
#include <QLineEdit>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QRandomGenerator>
#include <stdexcept>
#include <cmath>
#include <limits>

namespace {
class ChevronComboBox : public QComboBox {
protected:
    void paintEvent(QPaintEvent* event) override {
        QComboBox::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(isEnabled() ? QColor("#dddddd") : QColor("#777777"), 1.5, Qt::SolidLine,
                            Qt::RoundCap, Qt::RoundJoin));
        const QPointF center(width() - 14, height() / 2.0);
        painter.drawLine(center + QPointF(-3, -1.5), center + QPointF(0, 1.5));
        painter.drawLine(center + QPointF(0, 1.5), center + QPointF(3, -1.5));
    }

public:
    explicit ChevronComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}
};
class CountSpinBox : public QSpinBox {
protected:
    void paintEvent(QPaintEvent* event) override {
        QSpinBox::paintEvent(event);
        QStyleOptionSpinBox option;
        initStyleOption(&option);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const auto draw_arrow = [&](QStyle::SubControl control, bool up, bool enabled) {
            const auto rect = style()->subControlRect(QStyle::CC_SpinBox, &option, control, this);
            const QPointF center = rect.center();
            painter.setPen(QPen(enabled ? QColor("#dedede") : QColor("#777777"), 1.4, Qt::SolidLine,
                                Qt::RoundCap, Qt::RoundJoin));
            const double direction = up ? -1.0 : 1.0;
            painter.drawLine(center + QPointF(-2.5, -direction), center + QPointF(0, direction));
            painter.drawLine(center + QPointF(0, direction), center + QPointF(2.5, -direction));
        };
        draw_arrow(QStyle::SC_SpinBoxUp, true, stepEnabled().testFlag(StepUpEnabled));
        draw_arrow(QStyle::SC_SpinBoxDown, false, stepEnabled().testFlag(StepDownEnabled));
    }

public:
    explicit CountSpinBox(QWidget* parent = nullptr) : QSpinBox(parent) {}
};
QLabel* Label(const QString& text, const char* name, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setObjectName(name);
    label->setTextFormat(Qt::PlainText);
    return label;
}
QPushButton* Button(const QString& text, ui::Icon icon, const QString& tooltip,
                    QWidget* parent = nullptr) {
    auto* button = new QPushButton(text, parent);
    button->setIcon(ui::LineIcon(icon));
    button->setIconSize({20, 20});
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}
double PromptWeight(QWidget* parent, const QString& title, const QString& label, double initial,
                    bool* accepted) {
    const auto text = QInputDialog::getText(
        parent, title, label + "\nМожно использовать точку, запятую или запись 1e-5.",
        QLineEdit::Normal, QString::number(initial, 'g', std::numeric_limits<double>::max_digits10),
        accepted);
    if (!*accepted) return initial;
    bool valid = false;
    const double value = QString(text).trimmed().replace(',', '.').toDouble(&valid);
    if (!valid || !std::isfinite(value)) {
        throw std::invalid_argument("Введите конечный числовой вес, например 2.5 или 1e-5.");
    }
    return value;
}
}  // namespace
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Graph Studio");
    resize(1380, 890);
    setMinimumSize(1100, 760);
    setPalette(ui::DarkPalette());
    setStyleSheet(ui::StyleSheet());
    timer_ = new QTimer(this);
    timer_->setObjectName("playbackTimer");
    timer_->setInterval(680);
    build_ui();
    connect(timer_, &QTimer::timeout, this, [this] {
        if (runner_.cursor() < runner_.size()) seek(runner_.cursor() + 1);
        if (runner_.cursor() >= runner_.size()) {
            timer_->stop();
            show_frame();
        }
    });
    load_example(0);
    choose_algorithm(Algorithm::Dijkstra);
    QTimer::singleShot(0, this, [this] { canvas_->fit_graph(); });
}
void MainWindow::build_ui() {
    auto* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);
    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(24, 18, 24, 12);
    outer->setSpacing(16);
    auto* header = new QHBoxLayout;
    header->addStretch();
    auto* generate =
        Button("Сгенерировать", ui::Icon::NewGraph, "Создать случайный граф по параметрам");
    generate->setObjectName("generateGraphButton");
    auto* new_button = Button("Новый граф", ui::Icon::NewGraph, "Создать пустой граф");
    auto* open = Button("Открыть", ui::Icon::Open, "Открыть граф из JSON (Ctrl+O)");
    auto* save =
        Button("Сохранить", ui::Icon::Save, "Сохранить граф и расположение вершин (Ctrl+S)");
    header->addWidget(generate);
    header->addWidget(new_button);
    header->addWidget(open);
    header->addWidget(save);
    outer->addLayout(header);
    connect(generate, &QPushButton::clicked, this, [this] { generate_graph(); });
    connect(new_button, &QPushButton::clicked, this, [this] { new_graph(); });
    connect(open, &QPushButton::clicked, this, [this] { open_file(); });
    connect(save, &QPushButton::clicked, this, [this] { save_file(); });
    auto* save_action = new QAction(this);
    save_action->setShortcut(QKeySequence::Save);
    addAction(save_action);
    connect(save_action, &QAction::triggered, this, [this] { save_file(); });
    auto* open_action = new QAction(this);
    open_action->setShortcut(QKeySequence::Open);
    addAction(open_action);
    connect(open_action, &QAction::triggered, this, [this] { open_file(); });
    auto* body = new QHBoxLayout;
    body->setSpacing(16);
    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(256);
    auto* left = new QVBoxLayout(sidebar);
    left->setContentsMargins(16, 20, 16, 18);
    left->setSpacing(8);
    left->addWidget(Label("АЛГОРИТМЫ", "eyebrow"));
    left->addSpacing(8);
    algorithms_ = new QButtonGroup(this);
    algorithms_->setExclusive(true);
    const QString short_names[] = {"BFS · Обход в ширину", "DFS · Обход в глубину",     "Дейкстра",
                                   "Беллман–Форд",         "Топологическая сортировка", "Краскал",
                                   "Жадная раскраска"};
    for (int i = 0; i < 7; ++i) {
        auto* button = new QPushButton(short_names[i], sidebar);
        button->setObjectName("algorithmButton");
        button->setCheckable(true);
        button->setMinimumHeight(44);
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(TraceRunner::description(static_cast<Algorithm>(i)));
        algorithms_->addButton(button, i);
        left->addWidget(button);
    }
    connect(algorithms_, &QButtonGroup::idClicked, this,
            [this](int id) { choose_algorithm(static_cast<Algorithm>(id)); });
    left->addSpacing(18);
    left->addWidget(Label("ПАРАМЕТРЫ", "eyebrow"));
    left->addSpacing(5);
    left->addWidget(Label("Начальная вершина", "subtle"));
    start_ = new ChevronComboBox(sidebar);
    start_->setObjectName("startVertex");
    left->addWidget(start_);
    left->addSpacing(5);
    left->addWidget(Label("Конечная вершина", "subtle"));
    finish_ = new ChevronComboBox(sidebar);
    finish_->setObjectName("finishVertex");
    left->addWidget(finish_);
    connect(start_, &QComboBox::currentIndexChanged, this, [this] { invalidate_trace(); });
    connect(finish_, &QComboBox::currentIndexChanged, this, [this] { invalidate_trace(); });
    left->addStretch(1);
    auto* legend = Label("СОСТОЯНИЯ ВЕРШИН", "eyebrow");
    left->addWidget(legend);
    left->addSpacing(7);
    const char* labels[] = {"Текущая вершина", "Найдена / обработана", "Входит в результат"};
    const char* colors[] = {"#e1b77a", "#82b4ef", "#84c9a7"};
    for (int i = 0; i < 3; ++i) {
        auto* row = new QHBoxLayout;
        auto* dot = Label("●", "subtle");
        dot->setStyleSheet(QString("color:%1;font-size:15px;").arg(colors[i]));
        row->addWidget(dot);
        row->addWidget(Label(labels[i], "subtle"), 1);
        left->addLayout(row);
    }
    body->addWidget(sidebar);
    auto* right = new QVBoxLayout;
    right->setSpacing(12);
    auto* title_row = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    titles->setSpacing(6);
    title_ = Label("", "title");
    description_ = Label("", "description");
    description_->setWordWrap(true);
    titles->addWidget(title_);
    titles->addWidget(description_);
    title_row->addLayout(titles, 1);
    examples_ = new ChevronComboBox;
    examples_->setObjectName("exampleSelector");
    examples_->setMinimumWidth(214);
    examples_->setToolTip("Готовые графы для экспериментов");
    examples_->addItems({"Взвешенная сеть", "Зависимости · DAG", "Отрицательные веса",
                         "Петли и параллельные рёбра", "Граф для раскраски"});
    title_row->addWidget(examples_);
    right->addLayout(title_row);
    examples_->setPlaceholderText("Свой граф");
    connect(examples_, &QComboBox::activated, this, [this](int index) {
        timer_->stop();
        show_frame();
        if (discard_changes())
            load_example(index);
        else {
            QSignalBlocker block(examples_);
            examples_->setCurrentIndex(example_index_);
        }
    });
    auto* card = new QFrame;
    card->setObjectName("canvasCard");
    auto* graph_layout = new QVBoxLayout(card);
    graph_layout->setContentsMargins(14, 12, 14, 12);
    graph_layout->setSpacing(8);
    auto* tools = new QHBoxLayout;
    graph_info_ = Label("", "subtle");
    tools->addWidget(graph_info_, 1);
    auto* vertex =
        Button("Вершина", ui::Icon::Plus, "Добавить вершину (также двойной щелчок по сцене)");
    edge_tool_ =
        Button("Ребро", ui::Icon::Edge, "Включить режим соединения: выберите начало и конец ребра");
    edge_tool_->setCheckable(true);
    edge_tool_->setObjectName("edgeTool");
    auto* fit = Button("", ui::Icon::Fit, "Показать весь граф");
    fit->setFixedSize(36, 36);
    tools->addWidget(vertex);
    tools->addWidget(edge_tool_);
    tools->addWidget(fit);
    graph_layout->addLayout(tools);
    canvas_ = new GraphCanvas(card);
    graph_layout->addWidget(canvas_, 1);
    auto* hint = Label(
        "Двойной щелчок — вершина   ·   Перетаскивание — положение   ·   ПКМ — действия   ·   "
        "Колесо — масштаб",
        "subtle");
    hint->setStyleSheet("font-size:10px;color:#9a9a9a;");
    graph_layout->addWidget(hint);
    right->addWidget(card, 1);
    canvas_->create_vertex = [this](QPointF p) { add_vertex(p); };
    canvas_->click_vertex = [this](const std::string& id) { node_clicked(id); };
    canvas_->context = [this](const std::string& id, const std::optional<EdgeRef>& edge, QPointF p,
                              QPoint global) { show_context(id, edge, p, global); };
    canvas_->position_changed = [this] {
        dirty_ = true;
        setWindowModified(true);
    };
    connect(vertex, &QPushButton::clicked, this,
            [this] { add_vertex(canvas_->mapToScene(canvas_->viewport()->rect().center())); });
    connect(edge_tool_, &QPushButton::toggled, this, [this](bool enabled) {
        edge_from_.clear();
        canvas_->set_edge_mode(enabled);
        if (enabled) {
            timer_->stop();
            explanation_->setText(
                "Выберите начальную вершину, затем конечную. Повторный выбор той же вершины "
                "создаст петлю.");
        }
        show_frame();
        if (enabled)
            explanation_->setText(
                "Выберите начальную вершину, затем конечную. Повторный выбор той же вершины "
                "создаст петлю.");
    });
    connect(fit, &QPushButton::clicked, this, [this] { canvas_->fit_graph(); });
    auto* explanation_card = new QFrame;
    explanation_card->setObjectName("explanationCard");
    auto* explain_layout = new QVBoxLayout(explanation_card);
    explain_layout->setContentsMargins(18, 14, 18, 14);
    explain_layout->setSpacing(7);
    state_label_ = Label("ГОТОВО К ЗАПУСКУ", "stateLabel");
    explanation_ = Label("", "explanation");
    explanation_->setWordWrap(true);
    explanation_->setMinimumHeight(40);
    explain_layout->addWidget(state_label_);
    explain_layout->addWidget(explanation_);
    right->addWidget(explanation_card);
    auto* transport = new QFrame;
    transport->setObjectName("transport");
    auto* controls = new QHBoxLayout(transport);
    controls->setContentsMargins(12, 10, 12, 10);
    controls->setSpacing(9);
    auto* previous = Button("", ui::Icon::Previous, "Предыдущий шаг");
    previous->setObjectName("previousButton");
    previous->setFixedSize(34, 36);
    play_ = Button("Старт", ui::Icon::Play, "Запуск / пауза (Пробел)");
    play_->setObjectName("playButton");
    play_->setMinimumWidth(106);
    play_->setMinimumHeight(36);
    auto* next = Button("", ui::Icon::Next, "Следующий шаг");
    next->setObjectName("nextButton");
    next->setFixedSize(34, 36);
    auto* reset = Button("", ui::Icon::Reset, "Вернуться к началу анимации");
    reset->setObjectName("resetButton");
    reset->setFixedSize(34, 36);
    controls->addWidget(previous);
    controls->addWidget(play_);
    controls->addWidget(next);
    controls->addWidget(reset);
    timeline_ = new QSlider(Qt::Horizontal);
    timeline_->setObjectName("timeline");
    timeline_->setRange(0, 0);
    timeline_->setToolTip("Перемотка к любому шагу; изменение позиции ставит анимацию на паузу");
    controls->addWidget(timeline_, 1);
    step_label_ = Label("0 / 0", "subtle");
    step_label_->setMinimumWidth(76);
    step_label_->setAlignment(Qt::AlignCenter);
    controls->addWidget(step_label_);
    speed_ = new ChevronComboBox;
    speed_->setObjectName("speedSelector");
    speed_->addItem("0.5×", 0.5);
    speed_->addItem("1×", 1.0);
    speed_->addItem("1.5×", 1.5);
    speed_->addItem("2×", 2.0);
    speed_->addItem("3×", 3.0);
    speed_->setCurrentIndex(1);
    speed_->setFixedWidth(82);
    speed_->setToolTip("Скорость воспроизведения");
    controls->addWidget(speed_);
    right->addWidget(transport);
    body->addLayout(right, 1);
    outer->addLayout(body, 1);
    auto* footer = Label(
        "Редактор: до 50 вершин и 150 рёбер. Историю можно просматривать в обе стороны.", "subtle");
    footer->setStyleSheet("font-size:10px;color:#939393;");
    outer->addWidget(footer);
    connect(play_, &QPushButton::clicked, this, [this] { play_pause(); });
    connect(previous, &QPushButton::clicked, this, [this] {
        timer_->stop();
        if (!runner_.size()) prepare_trace();
        seek(runner_.cursor() ? runner_.cursor() - 1 : 0);
    });
    connect(next, &QPushButton::clicked, this, [this] {
        timer_->stop();
        if (!runner_.size()) prepare_trace();
        seek(runner_.cursor() + 1);
    });
    connect(reset, &QPushButton::clicked, this, [this] {
        timer_->stop();
        seek(0);
    });
    connect(timeline_, &QSlider::valueChanged, this, [this](int value) {
        timer_->stop();
        seek(static_cast<std::size_t>(value));
    });
    connect(speed_, &QComboBox::currentIndexChanged, this, [this] {
        timer_->setInterval(static_cast<int>(680 / speed_->currentData().toDouble()));
    });
    auto* play_action = new QAction(this);
    play_action->setShortcut(Qt::Key_Space);
    addAction(play_action);
    connect(play_action, &QAction::triggered, this, [this] { play_pause(); });
}
void MainWindow::refresh_document(bool fit) {
    const auto old_start = start_->currentText(), old_finish = finish_->currentText();
    QSignalBlocker block_start(start_), block_finish(finish_);
    start_->clear();
    finish_->clear();
    const auto ids = document_.graph().vertex_ids();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        start_->addItem(QString::fromStdString(ids.get(i)));
        finish_->addItem(QString::fromStdString(ids.get(i)));
    }
    if (start_->findText(old_start) >= 0) start_->setCurrentText(old_start);
    if (finish_->findText(old_finish) >= 0)
        finish_->setCurrentText(old_finish);
    else if (finish_->count())
        finish_->setCurrentIndex(finish_->count() - 1);
    graph_info_->setText(
        QString("%1 · вершин: %2 · рёбер: %3")
            .arg(document_.graph().is_directed() ? "Ориентированный" : "Неориентированный")
            .arg(document_.graph().vertex_count())
            .arg(document_.graph().edge_count()));
    canvas_->rebuild(document_);
    if (fit) canvas_->fit_graph();
    show_frame();
}
void MainWindow::invalidate_trace() {
    timer_->stop();
    runner_.clear();
    edge_from_.clear();
    show_frame();
}
void MainWindow::choose_algorithm(Algorithm algorithm) {
    algorithm_ = algorithm;
    if (auto* button = algorithms_->button(static_cast<int>(algorithm))) button->setChecked(true);
    title_->setText(TraceRunner::name(algorithm));
    description_->setText(TraceRunner::description(algorithm));
    const bool source = algorithm == Algorithm::BFS || algorithm == Algorithm::DFS ||
                        algorithm == Algorithm::Dijkstra || algorithm == Algorithm::BellmanFord;
    start_->setEnabled(source);
    finish_->setEnabled(algorithm == Algorithm::Dijkstra || algorithm == Algorithm::BellmanFord);
    invalidate_trace();
}
void MainWindow::load_example(int index) {
    timer_->stop();
    document_.load_example(index);
    runner_.clear();
    edge_from_.clear();
    edge_tool_->setChecked(false);
    {
        QSignalBlocker block(examples_);
        examples_->setCurrentIndex(index);
        example_index_ = index;
    }
    file_path_.clear();
    dirty_ = false;
    setWindowModified(false);
    setWindowTitle("Graph Studio — пример[*]");
    refresh_document(true);
}
void MainWindow::prepare_trace() {
    timer_->stop();
    edge_tool_->setChecked(false);
    try {
        runner_.prepare(document_.graph(), algorithm_, start_->currentText().toStdString(),
                        finish_->currentText().toStdString());
        show_frame();
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::seek(std::size_t step) {
    runner_.seek(step);
    show_frame();
}
void MainWindow::show_frame() {
    const auto state = runner_.state();
    canvas_->apply(state);
    {
        QSignalBlocker blocker(timeline_);
        timeline_->setRange(0, static_cast<int>(runner_.size()));
        timeline_->setValue(static_cast<int>(runner_.cursor()));
    }
    step_label_->setText(QString("%1 / %2").arg(runner_.cursor()).arg(runner_.size()));
    explanation_->setText(state.explanation);
    state_label_->setText(state.is_error ? "ВЫПОЛНЕНИЕ ОСТАНОВЛЕНО"
                          : runner_.size() && runner_.cursor() == runner_.size() ? "РЕЗУЛЬТАТ"
                          : runner_.cursor()                                     ? "ШАГ АЛГОРИТМА"
                                             : "ГОТОВО К ЗАПУСКУ");
    state_label_->setStyleSheet(state.is_error ? "color:#e5a0b0;" : "color:#b0c9eb;");
    play_->setText(timer_->isActive()                                       ? "Пауза"
                   : runner_.cursor() && runner_.cursor() == runner_.size() ? "Повтор"
                                                                            : "Старт");
    play_->setIcon(
        ui::LineIcon(timer_->isActive() ? ui::Icon::Pause : ui::Icon::Play, QColor("#182434")));
}
void MainWindow::play_pause() {
    if (timer_->isActive()) {
        timer_->stop();
        show_frame();
        return;
    }
    edge_tool_->setChecked(false);
    if (!runner_.size()) prepare_trace();
    if (!runner_.size()) return;
    if (runner_.has_error() && runner_.size() == 1) {
        seek(1);
        return;
    }
    if (runner_.cursor() >= runner_.size()) seek(0);
    timer_->start();
    show_frame();
}
void MainWindow::add_vertex(QPointF position) {
    timer_->stop();
    show_frame();
    bool ok = false;
    const auto name =
        QInputDialog::getText(this, "Новая вершина",
                              "Имя вершины (до 32 символов):", QLineEdit::Normal, {}, &ok)
            .trimmed();
    if (!ok) return;
    try {
        document_.add_vertex(name.toStdString(), position);
        invalidate_trace();
        dirty_ = true;
        setWindowModified(true);
        refresh_document();
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::node_clicked(const std::string& id) {
    if (!edge_tool_->isChecked()) return;
    const std::string target = id;
    if (edge_from_.empty()) {
        edge_from_ = target;
        explanation_->setText("Начало: " + QString::fromStdString(target) +
                              ". Теперь выберите конец ребра.");
        return;
    }
    const std::string from = edge_from_;
    edge_from_.clear();
    bool ok = false;
    try {
        const double weight =
            PromptWeight(this, "Новое ребро",
                         QString("Вес связи %1 → %2:")
                             .arg(QString::fromStdString(from), QString::fromStdString(target)),
                         1, &ok);
        if (!ok) return;
        document_.add_edge(from, target, weight);
        invalidate_trace();
        dirty_ = true;
        setWindowModified(true);
        refresh_document();
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::show_context(const std::string& vertex, const std::optional<EdgeRef>& edge,
                              QPointF position, QPoint global) {
    timer_->stop();
    show_frame();
    QMenu menu(this);
    const std::string vertex_copy = vertex;
    const auto edge_copy = edge;
    QAction* edit = nullptr;
    QAction* remove = nullptr;
    QAction* add = nullptr;
    if (!vertex_copy.empty()) {
        edit = menu.addAction("Переименовать вершину…");
        remove = menu.addAction("Удалить вершину и её рёбра");
    } else if (edge_copy) {
        edit = menu.addAction("Изменить вес…");
        remove = menu.addAction("Удалить это ребро");
    } else
        add = menu.addAction("Добавить вершину…");
    const auto* chosen = menu.exec(global);
    if (!chosen) return;
    if (chosen == add) {
        add_vertex(position);
        return;
    }
    try {
        if (chosen == remove) {
            if (edge_copy)
                document_.remove_edge(*edge_copy);
            else
                document_.remove_vertex(vertex_copy);
        } else if (chosen == edit) {
            bool ok = false;
            if (edge_copy) {
                const auto weight =
                    PromptWeight(this, "Вес ребра", "Новый вес:",
                                 document_.graph().get_edge_weight(edge_copy->from, edge_copy->to,
                                                                   edge_copy->index),
                                 &ok);
                if (!ok) return;
                document_.set_weight(*edge_copy, weight);
            } else {
                const auto name =
                    QInputDialog::getText(this, "Имя вершины", "Новое имя:", QLineEdit::Normal,
                                          QString::fromStdString(vertex_copy), &ok)
                        .trimmed();
                if (!ok) return;
                document_.rename_vertex(vertex_copy, name.toStdString());
            }
        }
        invalidate_trace();
        dirty_ = true;
        setWindowModified(true);
        refresh_document();
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::report_error(const std::exception& error) {
    timer_->stop();
    show_frame();
    state_label_->setText("НЕ УДАЛОСЬ ВЫПОЛНИТЬ ДЕЙСТВИЕ");
    state_label_->setStyleSheet("color:#e5a0b0;");
    explanation_->setText(QString::fromUtf8(error.what()));
}
bool MainWindow::discard_changes() {
    if (!dirty_) return true;
    const auto answer = QMessageBox::question(
        this, "Несохранённый граф", "Сохранить изменения графа?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) {
        save_file();
        return !dirty_;
    }
    return answer == QMessageBox::Discard;
}
void MainWindow::new_graph() {
    timer_->stop();
    show_frame();
    if (!discard_changes()) return;
    bool ok = false;
    const auto kind = QInputDialog::getItem(
        this, "Новый граф", "Тип графа:", {"Неориентированный", "Ориентированный"}, 0, false, &ok);
    if (!ok) return;
    document_.reset(kind == "Ориентированный");
    invalidate_trace();
    file_path_.clear();
    example_index_ = -1;
    {
        QSignalBlocker block(examples_);
        examples_->setCurrentIndex(-1);
    }
    setWindowTitle("Graph Studio — новый граф[*]");
    dirty_ = false;
    setWindowModified(false);
    refresh_document(true);
}
void MainWindow::open_file() {
    timer_->stop();
    show_frame();
    if (!discard_changes()) return;
    const auto path = QFileDialog::getOpenFileName(this, "Открыть граф", {}, "Граф JSON (*.json)");
    if (path.isEmpty()) return;
    try {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Не удалось открыть файл.");
        if (file.size() > 2 * 1024 * 1024) throw std::runtime_error("Файл превышает 2 МБ.");
        document_.load_json(file.readAll());
        invalidate_trace();
        example_index_ = -1;
        {
            QSignalBlocker block(examples_);
            examples_->setCurrentIndex(-1);
        }
        file_path_ = path;
        dirty_ = false;
        setWindowModified(false);
        setWindowTitle("Graph Studio — " + QFileInfo(path).fileName() + "[*]");
        refresh_document(true);
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::save_file() {
    timer_->stop();
    show_frame();
    QString path = file_path_;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, "Сохранить граф", "graph.json",
                                            "Граф JSON (*.json)");
    }
    if (path.isEmpty()) return;
    try {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Не удалось создать файл.");
        const auto bytes = document_.to_json();
        if (file.write(bytes) != bytes.size() || !file.commit())
            throw std::runtime_error("Не удалось сохранить граф.");
        file_path_ = path;
        dirty_ = false;
        setWindowModified(false);
        setWindowTitle("Graph Studio — " + QFileInfo(path).fileName() + "[*]");
        statusBar()->showMessage("Граф сохранён", 2500);
    } catch (const std::exception& error) {
        report_error(error);
    }
}
void MainWindow::closeEvent(QCloseEvent* event) {
    timer_->stop();
    show_frame();
    if (discard_changes())
        event->accept();
    else
        event->ignore();
}

void MainWindow::generate_graph() {
    timer_->stop();
    show_frame();
    QDialog dialog(this);
    dialog.setObjectName("graphGeneratorDialog");
    dialog.setWindowTitle("Генерация графа");
    dialog.setMinimumWidth(420);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(16);
    auto* explanation =
        Label("Случайный граф без петель и повторяющихся рёбер. Веса — от 1 до 9.", "description");
    explanation->setWordWrap(true);
    layout->addWidget(explanation);
    auto* form = new QFormLayout;
    form->setSpacing(12);
    auto* vertices = new CountSpinBox(&dialog);
    vertices->setObjectName("generationVertexCount");
    vertices->setRange(0, static_cast<int>(GraphDocument::kMaxVertices));
    vertices->setValue(10);
    auto* edges = new CountSpinBox(&dialog);
    edges->setObjectName("generationEdgeCount");
    edges->setRange(0, static_cast<int>(GraphDocument::kMaxEdges));
    edges->setValue(15);
    auto* direction = new ChevronComboBox(&dialog);
    direction->setObjectName("generationDirection");
    direction->addItems({"Неориентированный", "Ориентированный"});
    direction->setCurrentIndex(document_.graph().is_directed() ? 1 : 0);
    form->addRow("Количество вершин", vertices);
    form->addRow("Количество рёбер", edges);
    form->addRow("Тип графа", direction);
    layout->addLayout(form);
    auto* limits = Label("", "subtle");
    layout->addWidget(limits);
    auto* connectivity = Label("Связность и отсутствие циклов не гарантируются.", "subtle");
    connectivity->setWordWrap(true);
    layout->addWidget(connectivity);
    const auto update_limit = [vertices, edges, direction, limits] {
        const auto maximum = GraphDocument::max_generated_edges(
            static_cast<std::size_t>(vertices->value()), direction->currentIndex() == 1);
        edges->setMaximum(static_cast<int>(maximum));
        limits->setText(QString("Для этих параметров доступно от 0 до %1 рёбер.").arg(maximum));
    };
    connect(vertices, &QSpinBox::valueChanged, &dialog, update_limit);
    connect(direction, &QComboBox::currentIndexChanged, &dialog, update_limit);
    update_limit();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Создать граф");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("generateConfirmButton");
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted || !discard_changes()) return;
    try {
        document_.generate(static_cast<std::size_t>(vertices->value()),
                           static_cast<std::size_t>(edges->value()), direction->currentIndex() == 1,
                           QRandomGenerator::global()->generate());
        invalidate_trace();
        file_path_.clear();
        example_index_ = -1;
        {
            QSignalBlocker block(examples_);
            examples_->setCurrentIndex(-1);
        }
        dirty_ = true;
        setWindowTitle("Graph Studio — сгенерированный граф[*]");
        setWindowModified(true);
        refresh_document(true);
    } catch (const std::exception& error) {
        report_error(error);
    }
}
