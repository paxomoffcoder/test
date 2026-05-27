#include "llm_config_panel.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "llm_config_api_client.h"

LlmConfigPanel::LlmConfigPanel(QWidget *parent)
    : QWidget(parent)
    , api_(new LlmConfigApiClient(QStringLiteral("http://localhost:8080"), this))
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
    setWindowTitle(QStringLiteral("LLM Config Panel (HTTP)"));

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

    auto *loadBtn = new QPushButton(QStringLiteral("Load"), this);
    auto *applyBtn = new QPushButton(QStringLiteral("Apply"), this);
    connect(loadBtn, &QPushButton::clicked, this, &LlmConfigPanel::onLoadClicked);
    connect(applyBtn, &QPushButton::clicked, this, &LlmConfigPanel::onApplyClicked);

    connect(api_, &LlmConfigApiClient::fetchSucceeded, this, &LlmConfigPanel::onFetchSucceeded);
    connect(api_, &LlmConfigApiClient::updateSucceeded, this, &LlmConfigPanel::onUpdateSucceeded);
    connect(api_, &LlmConfigApiClient::requestFailed, this, &LlmConfigPanel::onRequestFailed);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(loadBtn);
    btnRow->addWidget(applyBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(btnRow);
    layout->addWidget(statusLabel_);

    fillUi(LlmConfig{});
    statusLabel_->setText(QStringLiteral("Ready. Press Load to fetch from server."));
}

void LlmConfigPanel::onLoadClicked()
{
    statusLabel_->setText(QStringLiteral("Loading config..."));
    api_->fetchConfig();
}

void LlmConfigPanel::onApplyClicked()
{
    statusLabel_->setText(QStringLiteral("Applying config..."));
    api_->updateConfig(readUi());
}

void LlmConfigPanel::onFetchSucceeded(const LlmConfig &cfg)
{
    fillUi(cfg);
    statusLabel_->setText(QStringLiteral("Config loaded"));
}

void LlmConfigPanel::onUpdateSucceeded()
{
    statusLabel_->setText(QStringLiteral("Config applied"));
}

void LlmConfigPanel::onRequestFailed(const QString &message)
{
    statusLabel_->setText(message);
}

void LlmConfigPanel::fillUi(const LlmConfig &cfg)
{
    endpointEdit_->setText(cfg.endpoint);
    modelEdit_->setText(cfg.model);
    timeoutSpin_->setValue(cfg.timeout_ms);
    temperatureSpin_->setValue(cfg.temperature);
    maxTokensSpin_->setValue(cfg.max_tokens);
    contextWindowSpin_->setValue(cfg.context_window);
    streamingCheck_->setChecked(cfg.streaming);
    llmSenderIdSpin_->setValue(static_cast<int>(cfg.llm_sender_id));
}

LlmConfig LlmConfigPanel::readUi() const
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
