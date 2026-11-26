#include "ayu/ui/custom_style/language_picker.h"

#include "lang/lang_cloud_manager.h"
#include "lang/lang_instance.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <vector>

namespace CustomUI {
namespace {

struct LangItem {
	QString id;
	QString name;
	QString badge;
	QColor colorTop;
	QColor colorBottom;
};

const std::vector<LangItem> &SupportedLanguages() {
	static const std::vector<LangItem> items = {
		{ u"ru"_q, u"Русский"_q, u"RU"_q, QColor(0x00, 0x39, 0xA6), QColor(0xD5, 0x2B, 0x1E) },
		{ u"en"_q, u"English"_q, u"EN"_q, QColor(0x01, 0x21, 0x69), QColor(0xC8, 0x10, 0x2E) },
		{ u"uk"_q, u"Українська"_q, u"UA"_q, QColor(0x00, 0x5B, 0xBB), QColor(0xFF, 0xD5, 0x00) },
		{ u"de"_q, u"Deutsch"_q, u"DE"_q, QColor(0xDD, 0x00, 0x00), QColor(0xFF, 0xCE, 0x00) },
		{ u"fr"_q, u"Français"_q, u"FR"_q, QColor(0x00, 0x55, 0xA4), QColor(0xEF, 0x41, 0x35) },
		{ u"es"_q, u"Español"_q, u"ES"_q, QColor(0xAA, 0x15, 0x1B), QColor(0xF1, 0xBF, 0x00) },
		{ u"it"_q, u"Italiano"_q, u"IT"_q, QColor(0x00, 0x92, 0x46), QColor(0xCE, 0x2B, 0x37) },
		{ u"pt-br"_q, u"Português"_q, u"PT"_q, QColor(0x00, 0x9C, 0x3B), QColor(0xFF, 0xDF, 0x00) },
		{ u"tr"_q, u"Türkçe"_q, u"TR"_q, QColor(0xE3, 0x0A, 0x17), QColor(0xB0, 0x08, 0x12) },
		{ u"pl"_q, u"Polski"_q, u"PL"_q, QColor(0xEE, 0xEE, 0xEE), QColor(0xDC, 0x14, 0x3C) },
		{ u"ko"_q, u"한국어"_q, u"KR"_q, QColor(0x00, 0x47, 0xA0), QColor(0xCD, 0x2E, 0x3A) },
		{ u"ja"_q, u"日本語"_q, u"JP"_q, QColor(0xF5, 0xF5, 0xF5), QColor(0xBC, 0x00, 0x2D) },
	};
	return items;
}

const LangItem &FindLang(const QString &id) {
	const auto &list = SupportedLanguages();
	for (const auto &item : list) {
		if (id.startsWith(item.id, Qt::CaseInsensitive)) {
			return item;
		}
	}
	return list[0];
}

void DrawCircleFlag(QPainter &p, const QRect &r, const LangItem &item) {
	p.save();
	QPainterPath clip;
	clip.addEllipse(r);
	p.setClipPath(clip);

	p.fillRect(QRect(r.left(), r.top(), r.width(), r.height() / 2), item.colorTop);
	p.fillRect(QRect(r.left(), r.top() + r.height() / 2, r.width(), r.height() - r.height() / 2), item.colorBottom);

	p.setPen(QColor(255, 255, 255, 230));
	QFont f = p.font();
	f.setBold(true);
	f.setPixelSize(std::max(8, r.height() / 2 - 1));
	p.setFont(f);
	p.drawText(r, Qt::AlignCenter, item.badge);
	p.restore();
}

// Строка одного языка в выпадающем списке
class LangRowButton final : public QWidget {
public:
	LangRowButton(const LangItem &item, std::function<void()> onClick, QWidget *parent)
	: QWidget(parent)
	, _item(item)
	, _onClick(std::move(onClick)) {
		setFixedHeight(42);
		setCursor(Qt::PointingHandCursor);
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);

		if (_hovered) {
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(255, 255, 255, 16));
			p.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 10, 10);
		}

		DrawCircleFlag(p, QRect(14, 9, 24, 24), _item);

		QFont f = font();
		f.setPixelSize(13);
		f.setBold(true);
		p.setFont(f);
		p.setPen(Qt::white);
		p.drawText(rect().adjusted(48, 0, -14, 0), Qt::AlignLeft | Qt::AlignVCenter, _item.name);
	}

	void enterEvent(QEnterEvent *) override {
		_hovered = true;
		update();
	}

	void leaveEvent(QEvent *) override {
		_hovered = false;
		update();
	}

	void mouseReleaseEvent(QMouseEvent *e) override {
		if (e->button() == Qt::LeftButton && rect().contains(e->pos()) && _onClick) {
			_onClick();
		}
	}

private:
	LangItem _item;
	std::function<void()> _onClick;
	bool _hovered = false;
};

// Всплывающее окно выбора языка (стиль skiper20)
class LanguagePickerModal final : public QDialog {
public:
	explicit LanguagePickerModal(QWidget *parent)
	: QDialog(parent, Qt::Popup | Qt::FramelessWindowHint) {
		setAttribute(Qt::WA_TranslucentBackground, true);
		setFixedSize(300, 380);

		auto *root = new QVBoxLayout(this);
		root->setContentsMargins(16, 14, 16, 14);
		root->setSpacing(10);

		// Шапка + крестик
		auto *header = new QHBoxLayout();
		auto *title = new QPushButton(u"Select language"_q, this);
		title->setFlat(true);
		title->setEnabled(false);
		title->setStyleSheet(u"QPushButton { color: #FFFFFF; font-size: 14px; font-weight: 600; text-align: left; border: none; }"_q);

		auto *closeBtn = new QPushButton(u"×"_q, this);
		closeBtn->setFixedSize(26, 26);
		closeBtn->setCursor(Qt::PointingHandCursor);
		closeBtn->setStyleSheet(u"QPushButton { color: #9A9A9A; font-size: 18px; border: none; background: transparent; } QPushButton:hover { color: #FFFFFF; }"_q);
		connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

		header->addWidget(title);
		header->addStretch();
		header->addWidget(closeBtn);
		root->addLayout(header);

		// Поле поиска
		_search = new QLineEdit(this);
		_search->setPlaceholderText(u"Search language..."_q);
		_search->setFixedHeight(36);
		_search->setStyleSheet(
			u"QLineEdit { background: #1C1C1C; color: #FFFFFF; border: 1px solid rgba(255,255,255,28); border-radius: 10px; padding: 0 12px; font-size: 13px; }"_q);
		root->addWidget(_search);

		// Прокручиваемый список языков
		auto *scroll = new QScrollArea(this);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		scroll->setStyleSheet(u"QScrollArea { background: transparent; } QScrollBar:vertical { width: 4px; background: transparent; } QScrollBar::handle:vertical { background: rgba(255,255,255,40); border-radius: 2px; }"_q);

		_listContainer = new QWidget(scroll);
		_listContainer->setStyleSheet(u"background: transparent;"_q);
		_listLayout = new QVBoxLayout(_listContainer);
		_listLayout->setContentsMargins(0, 0, 0, 0);
		_listLayout->setSpacing(2);
		scroll->setWidget(_listContainer);
		root->addWidget(scroll, 1);

		connect(_search, &QLineEdit::textChanged, this, [this](const QString &text) {
			rebuildList(text);
		});
		rebuildList(QString());
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setBrush(QColor(0x14, 0x14, 0x14));
		p.setPen(QPen(QColor(255, 255, 255, 22), 1.0));
		p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 20, 20);
	}

private:
	void rebuildList(const QString &query) {
		while (auto *item = _listLayout->takeAt(0)) {
			if (auto *w = item->widget()) {
				w->deleteLater();
			}
			delete item;
		}

		const auto q = query.trimmed();
		for (const auto &lang : SupportedLanguages()) {
			if (!q.isEmpty()
				&& !lang.name.contains(q, Qt::CaseInsensitive)
				&& !lang.id.contains(q, Qt::CaseInsensitive)
				&& !lang.badge.contains(q, Qt::CaseInsensitive)) {
				continue;
			}
			const auto langId = lang.id;
			auto *row = new LangRowButton(lang, [this, langId] {
				Lang::CurrentCloudManager().switchToLanguage({ langId });
				accept();
			}, _listContainer);
			_listLayout->addWidget(row);
		}
		_listLayout->addStretch();
	}

	QLineEdit *_search = nullptr;
	QWidget *_listContainer = nullptr;
	QVBoxLayout *_listLayout = nullptr;
};

} // namespace

LanguagePillWidget::LanguagePillWidget(QWidget *parent)
: Ui::RpWidget(parent) {
	setFixedSize(62, 32);
	setCursor(Qt::PointingHandCursor);
}

QString LanguagePillWidget::currentLangCode() const {
	return Lang::GetInstance().id();
}

void LanguagePillWidget::paintEvent(QPaintEvent *) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	// Тёмная пилюля (#242424)
	p.setBrush(_hovered ? QColor(0x30, 0x30, 0x30) : QColor(0x24, 0x24, 0x24));
	p.setPen(QPen(QColor(255, 255, 255, 24), 1.0));
	p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 15, 15);

	// Круглый флаг слева
	const auto &lang = FindLang(currentLangCode());
	DrawCircleFlag(p, QRect(6, 5, 22, 22), lang);

	// Стрелка вниз справа
	p.setPen(QPen(QColor(255, 255, 255, 200), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	const int cx = 44;
	const int cy = 15;
	p.drawLine(cx - 4, cy - 2, cx, cy + 2);
	p.drawLine(cx, cy + 2, cx + 4, cy - 2);
}

void LanguagePillWidget::enterEvent(QEnterEvent *) {
	_hovered = true;
	update();
}

void LanguagePillWidget::leaveEvent(QEvent *) {
	_hovered = false;
	update();
}

void LanguagePillWidget::mouseReleaseEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
		openPickerModal();
	}
}

void LanguagePillWidget::openPickerModal() {
	auto *modal = new LanguagePickerModal(window());
	modal->setAttribute(Qt::WA_DeleteOnClose, true);
	const auto globalPos = mapToGlobal(QPoint(width() - modal->width(), height() + 6));
	modal->move(globalPos);
	modal->show();
}

} // namespace CustomUI