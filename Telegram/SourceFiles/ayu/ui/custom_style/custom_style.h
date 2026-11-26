#pragma once

#include <QColor>
#include <QPainter>
#include <QRect>
#include <QTimer>
#include <optional>
#include <rpl/variable.h>

namespace CustomUI {

struct StyleConfig {
    bool customEnabled = false;       // false = системный стиль (чёрный/белый)
    QColor accentColor = QColor(0xB3, 0x47, 0xFF); // Фиолетовый акцент (skiper23)
    QColor cardBgColor = QColor(0x16, 0x16, 0x16); // Графитовый фон карточек
    float opacity = 0.88f;            // Прозрачность: 0.25 .. 1.0
    int borderRadius = 20;            // Закруглённость: 4 .. 28 px
    bool neonEnabled = false;         // Неоновое свечение и контур
    bool iconShimmer = false;         // Переливание иконок

    bool operator==(const StyleConfig &) const = default;
};

class StyleEngine {
public:
    static StyleEngine &Instance();

    const StyleConfig &current() const;
    rpl::producer<StyleConfig> changes() const;
    rpl::producer<float> shimmerPhase() const;

    void update(const StyleConfig &config, bool saveToDisk = false);
    void save();
    void load();

    // Общий метод отрисовки карточки (используется и в превью, и в реальном UI)
    void paintCard(
        QPainter &p,
        const QRect &rect,
        std::optional<QColor> customFill = std::nullopt) const;

    // Текущий цвет переливания для иконок
    QColor currentShimmerColor(float offset = 0.0f) const;

private:
    StyleEngine();
    void updateShimmerTimer();

    rpl::variable<StyleConfig> _config;
    rpl::variable<float> _shimmerPhase = 0.0f;
    QTimer _shimmerTimer;
};

} // namespace CustomUI