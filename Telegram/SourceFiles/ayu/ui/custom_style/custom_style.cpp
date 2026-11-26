#include "ayu/ui/custom_style/custom_style.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainterPath>
#include <cmath>

namespace CustomUI {

namespace {

QString ConfigFilePath() {
    return QCoreApplication::applicationDirPath() + "/custom_style.json";
}

} // namespace

StyleEngine &StyleEngine::Instance() {
    static StyleEngine instance;
    return instance;
}

StyleEngine::StyleEngine() {
    load();
    _shimmerTimer.setInterval(33); // ~30 FPS, минимальная нагрузка на CPU
    QObject::connect(&_shimmerTimer, &QTimer::timeout, [this] {
        auto next = _shimmerPhase.current() + 0.015f;
        if (next >= 1.0f) {
            next -= 1.0f;
        }
        _shimmerPhase = next;
    });
    updateShimmerTimer();
}

const StyleConfig &StyleEngine::current() const {
    return _config.current();
}

rpl::producer<StyleConfig> StyleEngine::changes() const {
    return _config.changes();
}

rpl::producer<float> StyleEngine::shimmerPhase() const {
    return _shimmerPhase.changes();
}

void StyleEngine::update(const StyleConfig &config, bool saveToDisk) {
    _config = config;
    updateShimmerTimer();
    if (saveToDisk) {
        save();
    }
}

void StyleEngine::updateShimmerTimer() {
    if (_config.current().iconShimmer) {
        if (!_shimmerTimer.isActive()) {
            _shimmerTimer.start();
        }
    } else {
        if (_shimmerTimer.isActive()) {
            _shimmerTimer.stop();
        }
    }
}

void StyleEngine::save() {
    const auto &c = _config.current();
    QJsonObject obj;
    obj["customEnabled"] = c.customEnabled;
    obj["accentColor"] = c.accentColor.name(QColor::HexRgb);
    obj["cardBgColor"] = c.cardBgColor.name(QColor::HexRgb);
    obj["opacity"] = static_cast<double>(c.opacity);
    obj["borderRadius"] = c.borderRadius;
    obj["neonEnabled"] = c.neonEnabled;
    obj["iconShimmer"] = c.iconShimmer;

    QFile file(ConfigFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

void StyleEngine::load() {
    QFile file(ConfigFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }
    const auto obj = doc.object();
    StyleConfig c;
    c.customEnabled = obj["customEnabled"].toBool(false);
    c.accentColor = QColor(obj["accentColor"].toString("#B347FF"));
    c.cardBgColor = QColor(obj["cardBgColor"].toString("#161616"));
    c.opacity = static_cast<float>(obj["opacity"].toDouble(0.88));
    c.borderRadius = obj["borderRadius"].toInt(20);
    c.neonEnabled = obj["neonEnabled"].toBool(false);
    c.iconShimmer = obj["iconShimmer"].toBool(false);
    _config = c;
}

void StyleEngine::paintCard(
        QPainter &p,
        const QRect &rect,
        std::optional<QColor> customFill) const {
    const auto &c = _config.current();
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto r = static_cast<qreal>(c.borderRadius);
    auto fill = customFill.value_or(c.cardBgColor);
    fill.setAlphaF(std::clamp(c.opacity, 0.2f, 1.0f));

    // Неоновое свечение (мягкий многослойный контур)
    if (c.neonEnabled) {
        auto glow = c.accentColor;
        for (int i = 3; i >= 1; --i) {
            glow.setAlpha(28 / i);
            p.setPen(QPen(glow, i * 2.5));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(rect.adjusted(1, 1, -1, -1), r, r);
        }
    }

    // Основная заливка карточки
    p.setBrush(fill);
    if (c.neonEnabled) {
        auto border = c.accentColor;
        border.setAlpha(210);
        p.setPen(QPen(border, 1.2));
    } else {
        p.setPen(QPen(QColor(255, 255, 255, 22), 1.0));
    }
    p.drawRoundedRect(rect.adjusted(1, 1, -1, -1), r, r);

    p.restore();
}

QColor StyleEngine::currentShimmerColor(float offset) const {
    const auto &c = _config.current();
    if (!c.iconShimmer) {
        return c.accentColor;
    }
    float hue = std::fmod(_shimmerPhase.current() + offset, 1.0f);
    if (hue < 0.0f) {
        hue += 1.0f;
    }
    return QColor::fromHsvF(hue, 0.78f, 1.0f);
}

} // namespace CustomUI