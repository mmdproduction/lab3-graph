#include <QApplication>
#include <gtest/gtest.h>
int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
