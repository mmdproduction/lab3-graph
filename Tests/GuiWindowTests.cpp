#include <gtest/gtest.h>
#include <QApplication>
#include <QComboBox>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QContextMenuEvent>
#include <QTemporaryFile>
#include <QAction>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QTest>
#include <QTimer>
#include "App/Gui/MainWindow.hpp"

TEST(GraphWindow, PlaybackPauseAndSliderRestorePastState) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    auto* start = window.findChild<QPushButton*>("playButton");
    auto* slider = window.findChild<QSlider*>("timeline");
    auto* timer = window.findChild<QTimer*>("playbackTimer");
    ASSERT_NE(start, nullptr);
    ASSERT_NE(slider, nullptr);
    ASSERT_NE(timer, nullptr);
    window.choose_algorithm(Algorithm::Coloring);
    start->click();
    EXPECT_TRUE(timer->isActive());
    EXPECT_GT(window.runner().size(), 0u);
    start->click();
    EXPECT_FALSE(timer->isActive());
    slider->setValue(slider->maximum());
    EXPECT_EQ(window.runner().cursor(), window.runner().size());
    EXPECT_GE(window.runner().state().color_count, 2u);
    slider->setValue(0);
    EXPECT_EQ(window.runner().cursor(), 0u);
    EXPECT_EQ(window.runner().state().color_count, 0u);
}
TEST(GraphWindow, SwitchingExampleInvalidatesAnimation) {
    MainWindow window;
    window.prepare_trace();
    ASSERT_GT(window.runner().size(), 0u);
    window.load_example(1);
    EXPECT_EQ(window.runner().size(), 0u);
    window.choose_algorithm(Algorithm::Topological);
    window.prepare_trace();
    EXPECT_FALSE(window.runner().has_error());
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    ASSERT_NE(canvas, nullptr);
    EXPECT_GT(canvas->scene()->items().size(), 6);
}
TEST(GraphWindow, MouseDoubleClickCreatesVertex) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    ASSERT_NE(canvas, nullptr);
    const auto before = window.document().graph().vertex_count();
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue("X");
            dialog->accept();
        }
    });
    QTest::mouseDClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                       canvas->mapFromScene(QPointF(0, 260)));
    EXPECT_EQ(window.document().graph().vertex_count(), before + 1);
    EXPECT_TRUE(window.document().graph().contains_vertex("X"));
    EXPECT_EQ(window.runner().size(), 0u);
}
TEST(GraphWindow, AllAlgorithmsCanBeSelectedAndRendered) {
    MainWindow window;
    window.show();
    for (int a = 0; a < 7; ++a) {
        window.load_example(a == 4 ? 1 : a == 3 ? 2 : 0);
        window.choose_algorithm(static_cast<Algorithm>(a));
        window.prepare_trace();
        ASSERT_GT(window.runner().size(), 0u);
        EXPECT_FALSE(window.runner().has_error());
        window.seek(window.runner().size());
        QApplication::processEvents();
        const auto shot = window.grab();
        EXPECT_FALSE(shot.isNull());
    }
}

TEST(GraphWindow, MouseEdgeToolAddsParallelEdgeAndSelfLoop) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    window.prepare_trace();
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    auto* tool = window.findChild<QPushButton*>("edgeTool");
    ASSERT_NE(canvas, nullptr);
    ASSERT_NE(tool, nullptr);
    const auto before = window.document().graph().edge_count();
    tool->setChecked(true);
    const auto click = [&](const std::string& id) {
        QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                          canvas->mapFromScene(window.document().position(id)));
    };
    click("A");
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue("2.25");
            dialog->accept();
        }
    });
    click("B");
    EXPECT_EQ(window.document().graph().edge_count(), before + 1);
    EXPECT_EQ(window.document().graph().edge_count("A", "B"), 2u);
    EXPECT_EQ(window.document().graph().get_edge_weight("A", "B", 1), 2.25);
    EXPECT_EQ(window.runner().size(), 0u);
    click("A");
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue("1");
            dialog->accept();
        }
    });
    click("A");
    EXPECT_TRUE(window.document().graph().contains_edge("A", "A"));
    window.choose_algorithm(Algorithm::Coloring);
    window.prepare_trace();
    window.seek(window.runner().size());
    EXPECT_TRUE(window.runner().state().is_error);
}
TEST(GraphWindow, DraggingNodeUpdatesPositionAndPreservesTrace) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    window.prepare_trace();
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    ASSERT_NE(canvas, nullptr);
    const auto before = window.document().position("A");
    const auto start = canvas->mapFromScene(before), end = start + QPoint(40, 25);
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, start);
    QMouseEvent move(QEvent::MouseMove, QPointF(end), QPointF(canvas->viewport()->mapToGlobal(end)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas->viewport(), &move);
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, end);
    EXPECT_GT(QLineF(before, window.document().position("A")).length(), 10);
    EXPECT_GT(window.runner().size(), 0u);
}
TEST(GraphWindow, TimerAdvancesAndChangingEndpointsStopsAndInvalidates) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    auto* speed = window.findChild<QComboBox*>("speedSelector");
    auto* play = window.findChild<QPushButton*>("playButton");
    auto* timer = window.findChild<QTimer*>("playbackTimer");
    ASSERT_NE(speed, nullptr);
    ASSERT_NE(play, nullptr);
    ASSERT_NE(timer, nullptr);
    speed->setCurrentIndex(4);
    play->click();
    QTest::qWait(520);
    EXPECT_GE(window.runner().cursor(), 1u);
    window.findChild<QComboBox*>("finishVertex")->setCurrentIndex(1);
    EXPECT_FALSE(timer->isActive());
    EXPECT_EQ(window.runner().size(), 0u);
}
TEST(GraphWindow, CancelledPresetSwitchKeepsSelectorAndGraphConsistent) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    ASSERT_NE(canvas, nullptr);
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
            dialog->setTextValue("X");
            dialog->accept();
        }
    });
    QTest::mouseDClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                       canvas->mapFromScene(QPointF(0, 260)));
    ASSERT_TRUE(window.document().graph().contains_vertex("X"));
    auto* examples = window.findChild<QComboBox*>("exampleSelector");
    ASSERT_NE(examples, nullptr);
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
            dialog->done(QMessageBox::Cancel);
    });
    examples->setCurrentIndex(1);
    QMetaObject::invokeMethod(examples, "activated", Qt::DirectConnection, Q_ARG(int, 1));
    EXPECT_TRUE(window.document().graph().contains_vertex("X"));
    EXPECT_EQ(examples->currentIndex(), 0);
}
TEST(GraphWindow, CancelledVertexDialogLeavesPlaybackPausedVisibly) {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    auto* play = window.findChild<QPushButton*>("playButton");
    ASSERT_NE(play, nullptr);
    play->click();
    QTimer::singleShot(0, [] {
        if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget()))
            dialog->reject();
    });
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    QTest::mouseDClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier,
                       canvas->mapFromScene(QPointF(0, 260)));
    EXPECT_FALSE(window.findChild<QTimer*>("playbackTimer")->isActive());
    EXPECT_NE(play->text(), QString("Пауза"));
}

TEST(GraphWindow, EditingLoadedWeightsWithoutChangesPreservesPrecisionAndRange) {
    QTemporaryFile file;
    ASSERT_TRUE(file.open());
    const QByteArray json =
        R"({"schema":1,"directed":false,"vertices":[{"id":"A","x":-200,"y":-100},{"id":"B","x":200,"y":-100},{"id":"C","x":-200,"y":100},{"id":"D","x":200,"y":100}],"edges":[{"from":"A","to":"B","weight":0.00001},{"from":"C","to":"D","weight":1000000000000}]})";
    ASSERT_EQ(file.write(json), json.size());
    file.flush();
    MainWindow window;
    window.show();
    QApplication::processEvents();
    QTimer::singleShot(0, [&] {
        if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
            dialog->selectFile(file.fileName());
            QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
        }
    });
    QAction* open = nullptr;
    for (auto* action : window.actions())
        if (action->shortcut() == QKeySequence::Open) open = action;
    ASSERT_NE(open, nullptr);
    open->trigger();
    ASSERT_EQ(window.document().graph().vertex_count(), 4u);
    auto* canvas = window.findChild<QGraphicsView*>("graphCanvas");
    ASSERT_NE(canvas, nullptr);
    for (double y : {-100.0, 100.0}) {
        QTimer::singleShot(0, [] {
            if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
                QTimer::singleShot(0, [] {
                    if (auto* dialog =
                            qobject_cast<QInputDialog*>(QApplication::activeModalWidget()))
                        dialog->accept();
                });
                QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier,
                                  menu->actionGeometry(menu->actions().first()).center());
            }
        });
        const auto point = canvas->mapFromScene(QPointF(0, y));
        QContextMenuEvent event(QContextMenuEvent::Mouse, point,
                                canvas->viewport()->mapToGlobal(point));
        QApplication::sendEvent(canvas->viewport(), &event);
    }
    EXPECT_DOUBLE_EQ(window.document().graph().get_edge_weight("A", "B", 0), 0.00001);
    EXPECT_DOUBLE_EQ(window.document().graph().get_edge_weight("C", "D", 0), 1000000000000.0);
}
