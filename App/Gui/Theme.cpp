#include "Theme.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

namespace ui {
QColor Color(int index) {
    static const char* colors[] = {"#82b4ef", "#baa0e4", "#84c9a7", "#e1b77a",
                                   "#dc94a8", "#76c6cc", "#c4ca7b", "#9baef0"};
    if (index < 8) return QColor(colors[index < 0 ? 0 : index]);
    return QColor::fromHslF(std::fmod(index * 0.61803398875, 1.0), 0.42, 0.70);
}
QPalette DarkPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202020"));
    palette.setColor(QPalette::WindowText, QColor("#eeeeee"));
    palette.setColor(QPalette::Base, QColor("#1c1c1c"));
    palette.setColor(QPalette::AlternateBase, QColor("#292929"));
    palette.setColor(QPalette::Text, QColor("#eeeeee"));
    palette.setColor(QPalette::Button, QColor("#303030"));
    palette.setColor(QPalette::ButtonText, QColor("#e8e8e8"));
    palette.setColor(QPalette::Highlight, QColor("#82b4ef"));
    palette.setColor(QPalette::HighlightedText, QColor("#202020"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#777777"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#777777"));
    return palette;
}
QString StyleSheet() {
    return R"(
QWidget { color:#e8e8e8; font-family:"DejaVu Sans"; font-size:12px; }
QMainWindow, QWidget#root { background:#202020; }
QFrame#sidebar { background:#282828; border:1px solid #414141; border-radius:16px; }
QFrame#canvasCard { background:#191919; border:1px solid #414141; border-radius:16px; }
QFrame#explanationCard, QFrame#transport { background:#292929; border:1px solid #444444; border-radius:13px; }
QLabel#eyebrow { color:#ababab; font-size:10px; font-weight:600; letter-spacing:2px; }
QLabel#title { font-size:22px; font-weight:600; }
QLabel#subtle { color:#b6b6b6; }
QLabel#description { color:#bcbcbc; font-size:12px; }
QLabel#stateLabel { color:#b0c9eb; font-size:10px; font-weight:600; letter-spacing:1px; }
QLabel#explanation { color:#ededed; font-size:13px; }
QPushButton, QToolButton { background:#333333; border:1px solid #4c4c4c; border-radius:8px; padding:8px 11px; }
QPushButton:hover, QToolButton:hover { background:#404040; border-color:#747474; }
QPushButton:pressed, QToolButton:pressed { background:#4a4a4a; }
QPushButton:disabled, QToolButton:disabled { color:#777777; background:#292929; border-color:#3c3c3c; }
QPushButton#primary { background:#a0c4f1; color:#18202a; border:1px solid #c0d7f5; font-weight:600; }
QPushButton#primary:hover { background:#bbd5f6; }
QPushButton#playButton { background:#a0c4f1; color:#18202a; border:1px solid #c0d7f5; font-weight:600; }
QPushButton#playButton:hover { background:#bbd5f6; }
QPushButton#algorithmButton { background:transparent; border:1px solid transparent; text-align:left; padding:11px 13px; font-size:12px; }
QPushButton#algorithmButton:hover { background:#363636; }
QPushButton#algorithmButton:checked { background:#36485f; border:1px solid #7899bf; color:#e7f0ff; }
QPushButton:checked { background:#3c526a; border-color:#8eb0d9; }
QComboBox { background:#323232; border:1px solid #505050; border-radius:7px; padding:7px 10px; min-height:17px; }
QComboBox:hover { border-color:#828282; }
QComboBox:disabled { color:#777777; background:#292929; border-color:#414141; }
QComboBox::down-arrow { image:none; }
QComboBox::drop-down { border:none; width:24px; }
QComboBox QAbstractItemView { background:#303030; color:#e8e8e8; selection-background-color:#4d6685; padding:5px; }
QLineEdit, QDoubleSpinBox, QSpinBox { background:#303030; border:1px solid #5c5c5c; border-radius:6px; padding:7px; }
QSlider::groove:horizontal { height:5px; background:#4a4a4a; border-radius:2px; }
QSlider::sub-page:horizontal { background:#9bbfee; border-radius:2px; }
QSlider::handle:horizontal { width:14px; height:14px; margin:-5px 0; background:#d6e3f5; border:2px solid #252525; border-radius:8px; }
QSlider:disabled::handle:horizontal { background:#828282; }
QMenu { background:#303030; border:1px solid #5c5c5c; padding:5px; }
QMenu::item { padding:8px 22px; border-radius:4px; }
QMenu::item:selected { background:#465f7e; }
QToolTip { background:#353535; color:#eeeeee; border:1px solid #707070; padding:7px; }
QDialog, QMessageBox { background:#292929; }
QStatusBar { background:#202020; color:#b0b0b0; }
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
