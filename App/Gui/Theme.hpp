#pragma once
#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

namespace ui {
enum class Icon {
    Play,
    Pause,
    Next,
    Previous,
    Reset,
    Fit,
    Plus,
    Edge,
    Cursor,
    Open,
    Save,
    NewGraph
};
QColor Color(int index);
QPalette DarkPalette();
QString StyleSheet();
QIcon LineIcon(Icon icon, QColor color = QColor("#becbdc"));
}  // namespace ui
