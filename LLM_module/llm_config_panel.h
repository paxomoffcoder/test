#ifndef LLM_CONFIG_PANEL_H
#define LLM_CONFIG_PANEL_H

#include <QWidget>

#include "llm_config.h"

class LlmConfigApiClient;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;

class LlmConfigPanel : public QWidget {
    Q_OBJECT
public:
    explicit LlmConfigPanel(QWidget *parent = nullptr);

private slots:
    void onLoadClicked();
    void onApplyClicked();
    void onFetchSucceeded(const LlmConfig &cfg);
    void onUpdateSucceeded();
    void onRequestFailed(const QString &message);

private:
    void fillUi(const LlmConfig &cfg);
    LlmConfig readUi() const;

    LlmConfigApiClient *api_;
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
