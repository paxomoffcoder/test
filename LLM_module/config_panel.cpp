#include "config_panel.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ConfigPanel::ConfigPanel(LlmConfigManager *configManager, QWidget *parent)
    : QWidget(parent)
    , configManager_(configManager)
    , endpointEdit_(new QLineEdit(this))
    , modelEdit_(new QLineEdit(this))
    , timeoutSpin_(new QSpinBox(this))
    , temperatureSpin_(new QDoubleSpinBox(this))
    , maxTokensSpin_(new QSpinBox(this))
    , contextWindowSpin_(new QSpinBox(this))
    , streamingCheck_(new QCheckBox(QStringLiteral("Streaming"), this))
    , llmSenderIdSpin_(new QSpinBox(this))
    , statusLabel_(new QLabel(this))
{
    setWindowTitle(QStringLiteral("LLM Config Panel"));

    timeoutSpin_->setRange(1, 3'600'000);
    maxTokensSpin_->setRange(1, 1'000'000);
    contextWindowSpin_->setRange(1, 10'000);
    llmSenderIdSpin_->setRange(0, 1'000'000);
    temperatureSpin_->setRange(0.0, 1.0);
    temperatureSpin_->setSingleStep(0.1);
    temperatureSpin_->setDecimals(3);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("endpoint"), endpointEdit_);
    form->addRow(QStringLiteral("model"), modelEdit_);
    form->addRow(QStringLiteral("timeout_ms"), timeoutSpin_);
    form->addRow(QStringLiteral("temperature"), temperatureSpin_);
    form->addRow(QStringLiteral("max_tokens"), maxTokensSpin_);
    form->addRow(QStringLiteral("context_window"), contextWindowSpin_);
    form->addRow(QStringLiteral("llm_sender_id"), llmSenderIdSpin_);
    form->addRow(QStringLiteral("streaming"), streamingCheck_);

    auto *applyBtn = new QPushButton(QStringLiteral("Apply"), this);
    connect(applyBtn, &QPushButton::clicked, this, &ConfigPanel::applyConfig);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(applyBtn);
    layout->addWidget(statusLabel_);

    loadFromConfig();
}

void ConfigPanel::loadFromConfig()
{
    if (configManager_ == nullptr) {
        statusLabel_->setText(QStringLiteral("Config manager is null"));
        return;
    }

    const LlmConfig cfg = configManager_->current();
    endpointEdit_->setText(cfg.endpoint);
    modelEdit_->setText(cfg.model);
    timeoutSpin_->setValue(cfg.timeout_ms);
    temperatureSpin_->setValue(cfg.temperature);
    maxTokensSpin_->setValue(cfg.max_tokens);
    contextWindowSpin_->setValue(cfg.context_window);
    streamingCheck_->setChecked(cfg.streaming);
    llmSenderIdSpin_->setValue(static_cast<int>(cfg.llm_sender_id));

    statusLabel_->setText(QStringLiteral("Loaded current config"));
}

LlmConfig ConfigPanel::buildFromUi() const
{
    LlmConfig cfg;
    cfg.endpoint = endpointEdit_->text();
    cfg.model = modelEdit_->text();
    cfg.timeout_ms = timeoutSpin_->value();
    cfg.temperature = temperatureSpin_->value();
    cfg.max_tokens = maxTokensSpin_->value();
    cfg.context_window = contextWindowSpin_->value();
    cfg.streaming = streamingCheck_->isChecked();
    cfg.llm_sender_id = llmSenderIdSpin_->value();
    return cfg;
}

void ConfigPanel::applyConfig()
{
    if (configManager_ == nullptr) {
        statusLabel_->setText(QStringLiteral("Config manager is null"));
        return;
    }

    QString error;
    const LlmConfig newConfig = buildFromUi();
    if (configManager_->apply(newConfig, &error)) {
        statusLabel_->setText(QStringLiteral("Config applied"));
    } else {
        statusLabel_->setText(QStringLiteral("Apply failed: %1").arg(error));
    }
}
