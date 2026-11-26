#include "ayu/ui/custom_style/custom_dashboard.h"
#include "ayu/ui/custom_style/custom_style.h"

#include "ui/rp_widget.h"
#include "ui/wrap/vertical_layout.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <array>

namespace Settings {
namespace {

using CustomUI::StyleEngine;

// 1. Живое превью: мини-чат, нижняя панель отправки и меню вложений
class LivePreviewWidget final : public Ui::RpWidget {
public:
	explicit LivePreviewWidget(QWidget *parent) : Ui::RpWidget(parent) {
		setMinimumHeight(190);
		auto &engine = StyleEngine::Instance();
		engine.changes() | rpl::start_with_next([this] {
			update();
		}, lifetime());
		engine.shimmerPhase() | rpl::start_with_next([this] {
			update();
		}, lifetime());
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);

		const auto &engine = StyleEngine::Instance();
		const auto &c = engine.current();
		const auto accent = engine.currentShimmerColor(0.0f);

		// Фон контейнера превью
		p.fillRect(rect(), QColor(0x0E, 0x0E, 0x0E));
		const auto box = rect().adjusted(16, 8, -16, -8);
		engine.paintCard(p, box, QColor(0x12, 0x12, 0x12));

		// Мини-превью чата (слева сверху)
		const auto chatRect = QRect(box.left() + 14, box.top() + 14, box.width() - 150, 56);
		engine.paintCard(p, chatRect);
		p.setBrush(accent);
		p.setPen(Qt::NoPen);
		p.drawEllipse(chatRect.left() + 12, chatRect.top() + 10, 36, 36);

		QFont titleFont = font();
		titleFont.setBold(true);
		titleFont.setPixelSize(13);
		p.setFont(titleFont);
		p.setPen(Qt::white);
		p.drawText(chatRect.adjusted(58, 10, -10, -24), Qt::AlignLeft | Qt::AlignVCenter, "Избранное");

		QFont subFont = font();
		subFont.setPixelSize(11);
		p.setFont(subFont);
		p.setPen(QColor(255, 255, 255, 150));
		p.drawText(chatRect.adjusted(58, 30, -10, -6), Qt::AlignLeft | Qt::AlignVCenter, "Живое превью стиля...");

		// Мини-меню вложений (справа)
		const auto menuRect = QRect(box.right() - 124, box.top() + 14, 110, 88);
		engine.paintCard(p, menuRect, QColor(0x1A, 0x1A, 0x1A));
		const std::array<QString, 3> items = { "Фото", "Файл", "Опрос" };
		for (int i = 0; i < 3; ++i) {
			const auto y = menuRect.top() + 12 + i * 24;
			p.setBrush(engine.currentShimmerColor(i * 0.18f));
			p.setPen(Qt::NoPen);
			p.drawEllipse(menuRect.left() + 12, y + 3, 10, 10);
			p.setPen(Qt::white);
			p.drawText(QRect(menuRect.left() + 30, y, 72, 18), Qt::AlignLeft | Qt::AlignVCenter, items[i]);
		}

		// Мини-панель сообщения и кнопка отправки (снизу)
		const auto inputRect = QRect(box.left() + 14, box.bottom() - 52, box.width() - 82, 38);
		engine.paintCard(p, inputRect, QColor(0x1B, 0x1B, 0x1B));
		p.setPen(QColor(255, 255, 255, 120));
		p.drawText(inputRect.adjusted(14, 0, -14, 0), Qt::AlignLeft | Qt::AlignVCenter, "Сообщение...");

		const auto sendRect = QRect(box.right() - 58, box.bottom() - 52, 44, 38);
		engine.paintCard(p, sendRect, accent);
		p.setPen(Qt::white);
		p.setFont(titleFont);
		p.drawText(sendRect, Qt::AlignCenter, "➤");
	}
};

// 2. Сетка 2x2 карточек в стиле skiper23
class SkiperGridWidget final : public Ui::RpWidget {
public:
	explicit SkiperGridWidget(QWidget *parent) : Ui::RpWidget(parent) {
		setMinimumHeight(250);
		setCursor(Qt::PointingHandCursor);
		auto &engine = StyleEngine::Instance();
		engine.changes() | rpl::start_with_next([this] { update(); }, lifetime());
		engine.shimmerPhase() | rpl::start_with_next([this] { update(); }, lifetime());
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.fillRect(rect(), QColor(0x0E, 0x0E, 0x0E));

		const auto &engine = StyleEngine::Instance();
		const auto &c = engine.current();
		const auto rects = cardRects();

		struct CardInfo {
			QString title;
			QString subtitle;
			QString icon;
			QColor color;
			bool active;
		};

		const std::array<CardInfo, 4> cards = {{
			{ "Кастом-стиль", c.customEnabled ? "Включён" : "Системный", "✦", c.accentColor, c.customEnabled },
			{ "Неон", c.neonEnabled ? "Свечение Вкл" : "Выкл", "◈", QColor(0x16, 0x16, 0x16), c.neonEnabled },
			{ "Переливание", c.iconShimmer ? "Анимация Вкл" : "Выкл", "◎", QColor(0x00, 0xC2, 0xD1), c.iconShimmer },
			{ "Сброс", "По умолчанию", "↺", QColor(0x2A, 0x7C, 0xFF), false },
		}};

		for (int i = 0; i < 4; ++i) {
			const auto &card = cards[i];
			const auto r = rects[i];
			const auto fill = (i == 1) ? c.cardBgColor : card.color;
			engine.paintCard(p, r, fill);

			// Иконка слева сверху
			QFont iconFont = font();
			iconFont.setPixelSize(20);
			p.setFont(iconFont);
			p.setPen(c.iconShimmer ? engine.currentShimmerColor(i * 0.2f) : Qt::white);
			p.drawText(QRect(r.left() + 16, r.top() + 14, 28, 28), Qt::AlignCenter, card.icon);

			// Кружок статуса (•••) справа сверху
			p.setBrush(QColor(255, 255, 255, card.active ? 90 : 35));
			p.setPen(Qt::NoPen);
			p.drawEllipse(r.right() - 36, r.top() + 14, 22, 22);

			// Заголовок и статус снизу слева
			QFont titleFont = font();
			titleFont.setBold(true);
			titleFont.setPixelSize(14);
			p.setFont(titleFont);
			p.setPen(Qt::white);
			p.drawText(QRect(r.left() + 16, r.bottom() - 48, r.width() - 32, 20), Qt::AlignLeft | Qt::AlignVCenter, card.title);

			QFont subFont = font();
			subFont.setPixelSize(12);
			p.setFont(subFont);
			p.setPen(QColor(255, 255, 255, 190));
			p.drawText(QRect(r.left() + 16, r.bottom() - 28, r.width() - 32, 18), Qt::AlignLeft | Qt::AlignVCenter, card.subtitle);
		}
	}

	void mouseReleaseEvent(QMouseEvent *e) override {
		const auto rects = cardRects();
		auto cfg = StyleEngine::Instance().current();
		bool changed = false;

		if (rects[0].contains(e->pos())) {
			cfg.customEnabled = !cfg.customEnabled;
			changed = true;
		} else if (rects[1].contains(e->pos())) {
			cfg.neonEnabled = !cfg.neonEnabled;
			cfg.customEnabled = true;
			changed = true;
		} else if (rects[2].contains(e->pos())) {
			cfg.iconShimmer = !cfg.iconShimmer;
			cfg.customEnabled = true;
			changed = true;
		} else if (rects[3].contains(e->pos())) {
			cfg = CustomUI::StyleConfig{};
			changed = true;
		}

		if (changed) {
			StyleEngine::Instance().update(cfg, true);
		}
	}

private:
	std::array<QRect, 4> cardRects() const {
		const int pad = 16;
		const int gap = 12;
		const int w = std::max(80, (width() - pad * 2 - gap) / 2);
		const int h = 108;
		return {{
			QRect(pad, 8, w, h),
			QRect(pad + w + gap, 8, w, h),
			QRect(pad, 8 + h + gap, w, h),
			QRect(pad + w + gap, 8 + h + gap, w, h),
		}};
	}
};

// 3. Ползунки (Скругление и Прозрачность) + Палитра акцентов
class StyleSlidersWidget final : public Ui::RpWidget {
public:
	explicit StyleSlidersWidget(QWidget *parent) : Ui::RpWidget(parent) {
		setMinimumHeight(210);
		setCursor(Qt::PointingHandCursor);
		StyleEngine::Instance().changes() | rpl::start_with_next([this] {
			update();
		}, lifetime());
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.fillRect(rect(), QColor(0x0E, 0x0E, 0x0E));

		const auto &engine = StyleEngine::Instance();
		const auto &c = engine.current();
		const auto box = rect().adjusted(16, 4, -16, -12);
		engine.paintCard(p, box, QColor(0x14, 0x14, 0x14));

		QFont labelFont = font();
		labelFont.setPixelSize(12);
		p.setFont(labelFont);

		// Ползунок 1: Закруглённость (4 .. 28 px)
		drawSlider(
			p,
			radiusBarRect(),
			QString("Закруглённость: %1 px").arg(c.borderRadius),
			(c.borderRadius - 4.0f) / 24.0f,
			c.accentColor);

		// Ползунок 2: Прозрачность (25% .. 100%)
		drawSlider(
			p,
			opacityBarRect(),
			QString("Прозрачность: %1%").arg(int(c.opacity * 100)),
			(c.opacity - 0.25f) / 0.75f,
			c.accentColor);

		// Палитра акцентов
		p.setPen(QColor(255, 255, 255, 180));
		p.drawText(QRect(box.left() + 16, box.bottom() - 56, 200, 18), Qt::AlignLeft, "Акцентный цвет:");

		const auto swatches = paletteColors();
		for (int i = 0; i < int(swatches.size()); ++i) {
			const auto sr = swatchRect(i);
			p.setBrush(swatches[i]);
			p.setPen(swatches[i] == c.accentColor
				? QPen(Qt::white, 2.0)
				: QPen(QColor(255, 255, 255, 40), 1.0));
			p.drawEllipse(sr);
		}
	}

	void mousePressEvent(QMouseEvent *e) override {
		handlePointer(e->pos(), false);
	}

	void mouseMoveEvent(QMouseEvent *e) override {
		if (e->buttons() & Qt::LeftButton) {
			handlePointer(e->pos(), false);
		}
	}

	void mouseReleaseEvent(QMouseEvent *e) override {
		handlePointer(e->pos(), true);
	}

private:
	static std::array<QColor, 6> paletteColors() {
		return {{
			QColor(0xB3, 0x47, 0xFF), // Фиолетовый
			QColor(0x00, 0xC2, 0xD1), // Циан
			QColor(0x2A, 0x7C, 0xFF), // Синий
			QColor(0xFF, 0x3B, 0x8B), // Розовый
			QColor(0x22, 0xC5, 0x5E), // Зелёный
			QColor(0xFF, 0x7A, 0x00), // Оранжевый
		}};
	}

	QRect radiusBarRect() const {
		return QRect(32, 38, std::max(100, width() - 64), 10);
	}

	QRect opacityBarRect() const {
		return QRect(32, 92, std::max(100, width() - 64), 10);
	}

	QRect swatchRect(int index) const {
		return QRect(32 + index * 38, height() - 44, 26, 26);
	}

	void drawSlider(
			QPainter &p,
			const QRect &bar,
			const QString &label,
			float ratio,
			const QColor &accent) const {
		ratio = std::clamp(ratio, 0.0f, 1.0f);
		p.setPen(QColor(255, 255, 255, 200));
		p.drawText(QRect(bar.left(), bar.top() - 22, bar.width(), 18), Qt::AlignLeft, label);

		p.setPen(Qt::NoPen);
		p.setBrush(QColor(255, 255, 255, 30));
		p.drawRoundedRect(bar, 5, 5);

		auto filled = bar;
		filled.setWidth(std::max(10, int(bar.width() * ratio)));
		p.setBrush(accent);
		p.drawRoundedRect(filled, 5, 5);
	}

	void handlePointer(const QPoint &pos, bool saveToDisk) {
		auto cfg = StyleEngine::Instance().current();
		const auto rBar = radiusBarRect().adjusted(0, -10, 0, 10);
		const auto oBar = opacityBarRect().adjusted(0, -10, 0, 10);

		if (rBar.contains(pos)) {
			const float t = std::clamp(
				float(pos.x() - rBar.left()) / float(std::max(1, rBar.width())),
				0.0f,
				1.0f);
			cfg.borderRadius = 4 + int(std::round(t * 24.0f));
			cfg.customEnabled = true;
			StyleEngine::Instance().update(cfg, saveToDisk);
			return;
		}

		if (oBar.contains(pos)) {
			const float t = std::clamp(
				float(pos.x() - oBar.left()) / float(std::max(1, oBar.width())),
				0.0f,
				1.0f);
			cfg.opacity = 0.25f + t * 0.75f;
			cfg.customEnabled = true;
			StyleEngine::Instance().update(cfg, saveToDisk);
			return;
		}

		if (saveToDisk) {
			const auto swatches = paletteColors();
			for (int i = 0; i < int(swatches.size()); ++i) {
				if (swatchRect(i).contains(pos)) {
					cfg.accentColor = swatches[i];
					cfg.customEnabled = true;
					StyleEngine::Instance().update(cfg, true);
					return;
				}
			}
		}
	}
};

} // namespace

CustomDashboard::CustomDashboard(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent) {
	setupContent();
}

rpl::producer<QString> CustomDashboard::title() {
	return rpl::single(QString("Custom Studio"));
}

void CustomDashboard::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	content->add(object_ptr<LivePreviewWidget>(content));
	content->add(object_ptr<SkiperGridWidget>(content));
	content->add(object_ptr<StyleSlidersWidget>(content));
	Ui::ResizeFitChild(this, content);
}

Type CustomDashboardId() {
	return CustomDashboard::Id();
}

} // namespace Settings