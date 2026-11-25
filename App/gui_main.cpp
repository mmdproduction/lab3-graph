#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <iostream>
#include "Gui/MainWindow.hpp"
#include "Gui/Theme.hpp"

int main(int argc, char** argv) {
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
#endif
    QApplication application(argc, argv);
    QApplication::setApplicationName("Graph Studio");
    QApplication::setStyle("Fusion");
    application.setPalette(ui::DarkPalette());
    QCommandLineParser parser;
    parser.setApplicationDescription("Пошаговая визуализация графовых алгоритмов");
    parser.addHelpOption();
    parser.addOption({"screenshot", "Сохранить снимок окна и выйти", "path"});
    parser.addOption({"example", "Номер примера 0–4", "number", "0"});
    parser.addOption({"algorithm", "Алгоритм: bfs, dfs, dijkstra, bellman, topo, kruskal, coloring",
                      "name", "dijkstra"});
    parser.addOption({"step", "Шаг для снимка: число или last", "step", "0"});
    parser.process(application);
    try {
        MainWindow window;
        window.load_example(parser.value("example").toInt());
        const QStringList names = {"bfs",  "dfs",     "dijkstra", "bellman",
                                   "topo", "kruskal", "coloring"};
        const int algorithm = names.indexOf(parser.value("algorithm"));
        if (algorithm < 0) throw std::invalid_argument("Неизвестный алгоритм.");
        window.choose_algorithm(static_cast<Algorithm>(algorithm));
        window.show();
        if (parser.isSet("screenshot")) {
            window.prepare_trace();
            window.seek(parser.value("step") == "last" ? window.runner().size()
                                                       : parser.value("step").toUInt());
            QTimer::singleShot(500, &window, [&] {
                const bool saved = window.grab().save(parser.value("screenshot"));
                application.exit(saved ? 0 : 2);
            });
        }
        return application.exec();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
