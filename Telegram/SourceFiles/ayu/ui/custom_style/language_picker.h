#pragma once

#include "ui/rp_widget.h"

namespace CustomUI {

class LanguagePillWidget final : public Ui::RpWidget {
public:
	explicit LanguagePillWidget(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *e) override;
	void enterEvent(QEnterEvent *e) override;
	void leaveEvent(QEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;

private:
	void openPickerModal();
	QString currentLangCode() const;

	bool _hovered = false;
};

} // namespace CustomUI