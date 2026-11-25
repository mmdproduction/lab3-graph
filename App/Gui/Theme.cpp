#include "Theme.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

namespace ui {
QColor Color(int index) {
    static const char* colors[] = {"#92b4d7", "#b2a1d1", "#91bca9", "#d0b185",
                                   "#ca9ba6", "#85b7bd", "#b5bc88", "#a4acd0"};
    if (index < 8) return QColor(colors[index < 0 ? 0 : index]);
    return QColor::fromHslF(std::fmod(index * 0.61803398875, 1.0), 0.30, 0.67);
}
QPalette DarkPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#171c25"));
    palette.setColor(QPalette::WindowText, QColor("#e4eaf3"));
    palette.setColor(QPalette::Base, QColor("#161c26"));
    palette.setColor(QPalette::AlternateBase, QColor("#202936"));
    palette.setColor(QPalette::Text, QColor("#e4eaf3"));
    palette.setColor(QPalette::Button, QColor("#222c3a"));
    palette.setColor(QPalette::ButtonText, QColor("#dce5f0"));
    palette.setColor(QPalette::Highlight, QColor("#92b4d7"));
    palette.setColor(QPalette::HighlightedText, QColor("#17222e"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#657186"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#657186"));
    return palette;
}
QString StyleSheet() {
    return R"(
QWidget { color:#dce5f0; font-family:"DejaVu Sans"; font-size:12px; }
QMainWindow, QWidget#root { background:#171c25; }
QFrame#sidebar { background:#1d2430; border:1px solid #303a4b; border-radius:16px; }
QFrame#canvasCard { background:#131923; border:1px solid #303a4b; border-radius:16px; }
QFrame#explanationCard, QFrame#transport { background:#1d2532; border:1px solid #303b4d; border-radius:13px; }
QLabel#brand { font-size:23px; font-weight:700; color:#ecf0f8; }
QLabel#eyebrow { color:#8495ab; font-size:10px; font-weight:600; letter-spacing:2px; }
QLabel#title { font-size:22px; font-weight:600; }
QLabel#subtle { color:#91a0b5; }
QLabel#description { color:#a0aec1; font-size:12px; }
QLabel#stateLabel { color:#9cb9dc; font-size:10px; font-weight:600; letter-spacing:1px; }
QLabel#explanation { color:#e0e7f1; font-size:13px; }
QPushButton, QToolButton { background:#252f3e; border:1px solid #38465a; border-radius:8px; padding:8px 11px; }
QPushButton:hover, QToolButton:hover { background:#303d50; border-color:#53667f; }
QPushButton:pressed, QToolButton:pressed { background:#34445a; }
QPushButton:disabled, QToolButton:disabled { color:#647289; background:#202936; border-color:#2e394a; }
QPushButton#primary { background:#a0badd; color:#182434; border:1px solid #b2c8e4; font-weight:600; }
QPushButton#primary:hover { background:#b1c8e6; }
QPushButton#playButton { background:#a0badd; color:#182434; border:1px solid #b2c8e4; font-weight:600; }
QPushButton#playButton:hover { background:#b1c8e6; }
QPushButton#algorithmButton { background:transparent; border:1px solid transparent; text-align:left; padding:11px 13px; font-size:12px; }
QPushButton#algorithmButton:hover { background:#263141; }
QPushButton#algorithmButton:checked { background:#2d3c52; border:1px solid #526d90; color:#d8e7fb; }
QPushButton:checked { background:#32465e; border-color:#718eb2; }
QComboBox { background:#232e3d; border:1px solid #3a485c; border-radius:7px; padding:7px 10px; min-height:17px; }
QComboBox:hover { border-color:#6e86a6; }
QComboBox:disabled { color:#66758c; background:#202936; border-color:#303c4e; }
QComboBox::down-arrow { image:none; }
QComboBox::drop-down { border:none; width:24px; }
QComboBox QAbstractItemView { background:#222e3e; color:#dce5f0; selection-background-color:#435c7c; padding:5px; }
QLineEdit, QDoubleSpinBox, QSpinBox { background:#202b3a; border:1px solid #465671; border-radius:6px; padding:7px; }
QSlider::groove:horizontal { height:5px; background:#364154; border-radius:2px; }
QSlider::sub-page:horizontal { background:#8daed6; border-radius:2px; }
QSlider::handle:horizontal { width:14px; height:14px; margin:-5px 0; background:#c2d4eb; border:2px solid #1b2635; border-radius:8px; }
QSlider:disabled::handle:horizontal { background:#66768c; }
QMenu { background:#222d3c; border:1px solid #465671; padding:5px; }
QMenu::item { padding:8px 22px; border-radius:4px; }
QMenu::item:selected { background:#3b506d; }
QToolTip { background:#29374a; color:#e4eaf3; border:1px solid #536985; padding:7px; }
QDialog, QMessageBox { background:#1d2633; }
QStatusBar { background:#171c25; color:#889ab2; }
)";
}
QIcon LineIcon(Icon icon, QColor color) {
    QPixmap pixmap(48, 48);
    pixmap.setDevicePixelRatio(2);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path;
    switch (icon) {
        case Icon::Play:
            path.moveTo(8, 5);
            path.lineTo(19, 12);
            path.lineTo(8, 19);
            path.closeSubpath();
            p.setBrush(color);
            break;
        case Icon::Pause:
            p.drawLine(9, 6, 9, 18);
            p.drawLine(16, 6, 16, 18);
            break;
        case Icon::Next:
            path.moveTo(7, 6);
            path.lineTo(15, 12);
            path.lineTo(7, 18);
            p.drawLine(18, 6, 18, 18);
            break;
        case Icon::Previous:
            path.moveTo(17, 6);
            path.lineTo(9, 12);
            path.lineTo(17, 18);
            p.drawLine(6, 6, 6, 18);
            break;
        case Icon::Reset:
            p.drawArc(QRectF(5, 5, 14, 14), 35 * 16, 290 * 16);
            path.moveTo(4, 5);
            path.lineTo(4, 11);
            path.lineTo(10, 11);
            break;
        case Icon::Fit:
            path.moveTo(4, 9);
            path.lineTo(4, 4);
            path.lineTo(9, 4);
            path.moveTo(15, 4);
            path.lineTo(20, 4);
            path.lineTo(20, 9);
            path.moveTo(20, 15);
            path.lineTo(20, 20);
            path.lineTo(15, 20);
            path.moveTo(9, 20);
            path.lineTo(4, 20);
            path.lineTo(4, 15);
            break;
        case Icon::Plus:
            p.drawLine(5, 12, 19, 12);
            p.drawLine(12, 5, 12, 19);
            break;
        case Icon::Edge:
            p.drawEllipse(QPointF(5, 18), 2.5, 2.5);
            p.drawEllipse(QPointF(19, 6), 2.5, 2.5);
            p.drawLine(7, 16, 17, 8);
            break;
        case Icon::Cursor:
            path.moveTo(6, 4);
            path.lineTo(18, 13);
            path.lineTo(12, 14);
            path.lineTo(9, 20);
            path.closeSubpath();
            break;
        case Icon::Open:
            path.moveTo(3, 8);
            path.lineTo(3, 5);
            path.lineTo(10, 5);
            path.lineTo(12, 8);
            path.lineTo(21, 8);
            path.lineTo(18, 19);
            path.lineTo(3, 19);
            path.lineTo(6, 10);
            path.lineTo(20, 10);
            break;
        case Icon::Save:
            p.drawRoundedRect(QRectF(4, 4, 16, 16), 2, 2);
            p.drawRect(QRectF(8, 4, 8, 6));
            p.drawRect(QRectF(8, 14, 8, 6));
            break;
        case Icon::NewGraph:
            p.drawLine(7, 7, 18, 12);
            p.drawLine(7, 17, 18, 12);
            p.drawEllipse(QPointF(5, 6), 2.5, 2.5);
            p.drawEllipse(QPointF(5, 18), 2.5, 2.5);
            p.drawEllipse(QPointF(19, 12), 2.5, 2.5);
            break;
    }
    p.drawPath(path);
    return QIcon(pixmap);
}
}  // namespace ui
