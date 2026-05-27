#ifndef CONFIG_PANEL_H
#define CONFIG_PANEL_H

#include <QWidget>

#include "llm_config.h"

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;

class ConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit ConfigPanel(LlmConfigManager *configManager, QWidget *parent = nullptr);

private slots:
    void applyConfig();

private:
    void loadFromConfig();
    LlmConfig buildFromUi() const;

    LlmConfigManager *configManager_;

    QLineEdit *endpointEdit_;
    QLineEdit *modelEdit_;
    QSpinBox *timeoutSpin_;
    QDoubleSpinBox *temperatureSpin_;
    QSpinBox *maxTokensSpin_;
    QSpinBox *contextWindowSpin_;
    QCheckBox *streamingCheck_;
    QSpinBox *llmSenderIdSpin_;
    QLabel *statusLabel_;
};

#endif
