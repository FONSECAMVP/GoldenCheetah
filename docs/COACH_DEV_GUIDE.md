# GoldenCheetah AI Coach Development Guide

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture and Process Flow](#architecture-and-process-flow)
3. [Implemented Features](#implemented-features)
4. [Features in Development](#features-in-development)
5. [Technical Implementation Details](#technical-implementation-details)
6. [API and Integration Points](#api-and-integration-points)
7. [Development Roadmap](#development-roadmap)
8. [Known Limitations and Technical Debt](#known-limitations-and-technical-debt)
9. [Contribution Guidelines](#contribution-guidelines)
10. [Qt6 Migration Notes](#qt6-migration-notes)

---

## Project Overview

The GoldenCheetah AI Coach is a cloud-based LLM (Large Language Model) coaching system designed to provide intelligent, personalized guidance to cyclists based on their training data. The module integrates seamlessly with the existing GoldenCheetah ecosystem, leveraging decades of performance analysis research and modern AI capabilities to deliver evidence-based coaching advice.

### Core Components

| Component | Purpose |
|-----------|---------|
| LLMService | Abstract interface for LLM providers (OpenAI, Anthropic, Gemini) |
| OpenAIClient | GPT-4o integration with streaming support |
| AnthropicClient | Claude 3.5 Sonnet integration with SSE streaming |
| GeminiClient | Gemini 1.5 Pro integration with JSON streaming |
| AthleteContextAggregator | Data aggregation from GoldenCheetah core |
| PromptBuilder | Domain-specific prompt construction |
| CoachChatWidget | Main UI for conversational interaction |

### Integration Points

- **Context** - Access to current athlete state
- **Athlete** - Profile data including zones and preferences
- **RideCache** - Historical ride data for pattern analysis
- **PMCData** - Performance Manager Chart metrics
- **Measures** - HRV and body composition tracking
- **Seasons** - Event planning and season structure

---

## Architecture and Process Flow

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    USER INTERFACE LAYER                     │
│              CoachChatWidget (Chat Interface)              │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   COACHING ENGINE LAYER                    │
│  ┌─────────────────┐  ┌─────────────────────────────────┐ │
│  │  PromptBuilder  │  │ AthleteContextAggregator         │ │
│  │  - Templates    │  │  - PMC Metrics  - HRV Data     │ │
│  │  - Formatting   │  │  - Ride History - Performance  │ │
│  └─────────────────┘  └─────────────────────────────────┘ │
│                           │                                │
│                           ▼                                │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              PhaseManager (State Machine)            │  │
│  │  Assessment │ Analysis │ Planning │ Coaching       │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   LLM SERVICE LAYER                        │
│  LLMService (Abstract) → OpenAI │ Anthropic │ Gemini      │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                  GOLDENCHEETAH CORE                        │
│  Context │ Athlete │ RideCache │ PMCData │ Measures       │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow Sequence

1. **User Input** → Message entered in chat widget
2. **Context Retrieval** → AthleteContextAggregator queries core data
3. **Prompt Construction** → PromptBuilder creates domain-specific prompt
4. **LLM Processing** → Selected LLM client sends to API (OpenAI/Anthropic/Gemini)
5. **Response Display** → Streaming response shown in chat
6. **Conversation Update** → History maintained for context

---

## Implemented Features

### Data Integration

| Data Source | Type | Usage |
|-------------|------|-------|
| Athlete Profile | FTP, weight, sport | Personalization |
| Ride History | 90-day data | Pattern analysis |
| PMC Metrics | CTL, ATL, TSB, Ramp Rate | Training load |
| HRV Data | RMSSD, trends | Recovery |
| Season Events | Goals, dates | Planning |
| Training Recommendations | Daily advice | Coaching |

### LLM Providers

| Provider | Model | Status | Features |
|----------|-------|--------|----------|
| OpenAI | GPT-4o | ✅ Complete | Streaming, rate limiting (50 req/min) |
| Anthropic | Claude 3.5 Sonnet | ✅ Complete | SSE streaming, structured output |
| Gemini | Gemini 1.5 Pro | ✅ Complete | JSON streaming, large context |

### Prompt Engineering

- System prompts for each coaching phase
- Natural language data formatting
- Evidence-based training principles
- Context-aware personalization
- Suggested follow-up questions
- Performance trend analysis

### UI Components

- Conversational chat interface
- Streaming response display
- Quick action suggestions
- Settings management (API key, model selection)
- Phase selector (Assessment, Analysis, Planning, Coaching)
- Error handling with user feedback
- MainWindow sidebar integration

### API Integration

- Multi-provider LLM support (OpenAI, Anthropic, Gemini)
- Streaming response support
- Error handling and fallback
- Rate limiting protection (50 requests/minute)
- Configuration persistence
- Qt6 compatibility

---

## Features in Development

### Short-Term (Next Release)

| Feature | Description | Status |
|---------|-------------|--------|
| MainWindow Integration | Add to sidebar | ✅ Complete |
| CalendarBridge | Plan synchronization | 🔄 In Progress |
| Notification System | Proactive alerts | 📋 Planned |

### Medium-Term (Upcoming Quarters)

| Feature | Description |
|---------|-------------|
| Pattern Detection | ML-based fatigue/performance patterns |
| Recovery Score | Composite recovery algorithm |
| Workout Generator | AI-generated interval workouts |

---

## Technical Implementation Details

### Key Classes

#### LLMService

```cpp
class LLMService : public QObject {
    // Configuration
    virtual void configure(const QVariantMap& settings) = 0;
    virtual Provider provider() const = 0;
    virtual bool isAvailable() const = 0;

    // Conversation
    virtual void addSystemMessage(const QString& content) = 0;
    virtual void addUserMessage(const QString& content) = 0;
    virtual void sendMessageStream(const QString& content) = 0;

signals:
    void responseReady(const Response& response);
    void streamingChunk(const QString& chunk);
    void streamingFinished();
    void errorOccurred(const QString& error);
};
```

#### OpenAIClient

```cpp
class OpenAIClient : public LLMService {
    // API configuration
    void setApiKey(const QString& key);
    void setModel(const QString& model);

    // Streaming support
    void sendMessageStream(const QString& content) override;
    void stopStreaming();

    // Rate limiting
    bool checkRateLimit();

signals:
    void rateLimitWarning(const QString& message);
};
```

#### AthleteContextAggregator

```cpp
class AthleteContextAggregator : public QObject {
    // Data retrieval
    AthleteProfile getProfile() const;
    QList<RideSummary> getRecentRides(int days = 90) const;
    QVector<double> getPMCHistory(int days = 90) const;
    HRVMetrics getHRVData() const;

    // Analysis
    PatternAnalysis analyzePatterns() const;
    QList<TrainingRecommendation> getDailyRecommendations() const;
    double calculateRampRate() const;

signals:
    void profileUpdated(const AthleteProfile& profile);
    void dataRefreshed();
};
```

### Data Structures

#### AthleteProfile

```cpp
struct AthleteProfile {
    QString name;
    QString sport = "cycling";
    double weight = 0.0;
    int ftp = 0;

    struct PMCMetrics {
        double ctl = 0.0;      // Chronic Training Load
        double atl = 0.0;      // Acute Training Load
        double tsb = 0.0;      // Training Stress Balance
        double rampRate = 0.0; // CTL ramp rate
    } pmc;

    struct HRVMetrics {
        double rmssd = 0.0;
        QString trend;         // "improving", "stable", "declining"
        QDate lastReading;
    } hrv;

    QString primaryGoal;
    int weeksToEvent = 0;
    QList<RideSummary> recentRides;
};
```

#### CoachingPhase

```cpp
enum class CoachingPhase {
    Assessment,    // Initial athlete evaluation
    Analysis,      // Data pattern analysis
    Planning,      // Training plan development
    Coaching       // Daily coaching interactions
};
```

### Signal-Slot Connections

```cpp
// User input
connect(sendButton_, &QPushButton::clicked, this, &CoachChatWidget::onSendMessage);

// LLM responses
connect(llmService_, &LLMService::responseReady,
        this, &CoachChatWidget::onResponseReceived);
connect(llmService_, &LLMService::streamingChunk,
        this, &CoachChatWidget::onStreamingChunk);
connect(llmService_, &LLMService::errorOccurred,
        this, &CoachChatWidget::onError);

// Context updates
connect(context_, &Context::dateRangeSelected,
        this, &CoachChatWidget::onDateRangeChanged);

// Phase changes
connect(phaseSelector_, &QComboBox::currentIndexChanged,
        this, &CoachChatWidget::onPhaseChanged);
```

---

## API and Integration Points

### Context Access

```cpp
Athlete* athlete = context_->athlete;
int ftp = athlete->zones("Bike")->getCP(0);
PMCData* pmc = context_->athlete->getPMCFor("TSS");
Season* season = context_->currentSeason();
RideCache* cache = &context_->athlete->rideCache;
Measures* measures = &context_->athlete->measures;
```

### Data Model Integration

| System | Integration Method |
|--------|-------------------|
| RideCache | `context_->athlete->rideCache` |
| PMCData | `context_->athlete->getPMCFor()` |
| Measures | `context_->athlete->measures` |
| Seasons | `context_->currentSeason()` |

### Configuration

```cpp
// Coach settings structure
struct CoachSettings {
    QString apiKey;
    QString model;
    LLMProvider provider;
    int maxTokens = 1000;
    double temperature = 0.7;
    bool streamingEnabled = true;
    int rateLimitRpm = 50;
};
```

### Extension Points

```cpp
// Custom LLM provider
class CustomLLMService : public LLMService {
    void configure(const QVariantMap& settings) override;
    Provider provider() const override { return CustomProvider; }
};

// Custom prompt templates
class CustomPromptBuilder : public PromptBuilder {
    QString buildSystemPrompt(CoachingPhase phase,
                             const AthleteProfile& profile) override;
};
```

---

## Development Roadmap

### Current Phase (Phase 1: Production Release)

**Goal:** Production-ready AI Coach with full Qt6 compatibility

### Completed Milestones

| Milestone | Status | Date |
|-----------|--------|------|
| Core Infrastructure | ✅ Complete | Week 1-2 |
| Data Integration | ✅ Complete | Week 2-3 |
| UI Implementation | ✅ Complete | Week 3-4 |
| Prompt Engineering | ✅ Complete | Week 4-5 |
| Multi-Provider LLM | ✅ Complete | Week 5-6 |
| Rate Limiting | ✅ Complete | Week 6 |
| Build Integration | ✅ Complete | Week 6 |
| Qt6 Migration | ✅ Complete | Week 7 |
| MainWindow Integration | ✅ Complete | Week 7 |
| Production Build | ✅ Complete | Week 7 |

### Testing Timeline

- **Week 7:** Unit testing (90% coverage target)
- **Week 8:** Integration testing (50+ scenarios)
- **Week 9:** User acceptance testing

### Release Plan

| Version | Features | Status |
|---------|----------|--------|
| 1.0 (Alpha) | Basic chat, all phases, multiple providers | ✅ Complete |
| 1.1 (Beta) | Calendar sync, notifications | 🔄 In Progress |
| 2.0 (Production) | Analytics, workout generator | 📋 Planned |

---

## Known Limitations

### Context Window

| Limitation | Impact | Mitigation |
|------------|--------|------------|
| Token limit (128K) | Large histories truncated | Smart summarization |
| Historical data | Long-term patterns lost | Trend extraction |
| Claude 200K context | Larger context window | Use for complex analysis |

### Response Latency

| Scenario | Typical | 95th Percentile |
|----------|---------|-----------------|
| Simple query | 2-4s | 6s |
| Complex analysis | 5-10s | 15s |
| Streaming start | <1s | 2s |

### API Risks

- Provider service dependency
- Potential rate limiting
- API pricing changes
- Network connectivity requirements

---

## Technical Debt

| Item | Status | Notes |
|------|--------|-------|
| Basic summarization algorithm | ⚠️ Pending | Needs improvement |
| Response caching | ❌ Not Implemented | Future enhancement |
| Limited error recovery | ⚠️ Pending | Needs enhancement |
| No offline mode | ❌ Not Implemented | Future enhancement |
| Qt6 migration completion | ✅ Complete | All issues resolved |

---

## Contribution Guidelines

### Development Setup

```bash
# Prerequisites
Qt 6.x | CMake 3.16+ | C++17 | Git 2.0+

# Build with Coach module
mkdir build && cd build
cmake -DGC_WANT_COACH=ON ..
make GoldenCheetah

# Run standalone Coach tests
ctest -R Coach
```

### Configuration

1. Copy `src/Coach/Secrets.h.template` to `src/Coach/Secrets.h`
2. Add your API keys:
   ```cpp
   #define OPENAI_API_KEY "your-key-here"
   #define ANTHROPIC_API_KEY "your-key-here"
   #define GEMINI_API_KEY "your-key-here"
   ```

### Testing

```bash
# Run all tests
ctest --verbose

# Run Coach module tests
ctest -R Coach

# Run with coverage
ctest --coverage
```

### Code Style

| Rule | Standard |
|------|----------|
| Naming | camelCase variables, PascalCase classes |
| Indentation | 4 spaces |
| Comments | Doxygen-style |
| Files | One class per .h/.cpp pair |

### Pull Request Process

1. Fork repository
2. Create feature branch
3. Implement with tests
4. Run `ctest -R Coach`
5. Commit with description
6. Submit PR with changelog

### Build Verification

```bash
# Verify Coach module compiles
cd build
make Coach  # Should complete without errors

# Check for warnings
make Coach VERBOSE=1 | grep -i warning
```

---

## Build Configuration

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| GC_WANT_COACH | OFF | Enable AI Coach module |
| GC_WANT_PYTHON | OFF | Python scripting |
| GC_WANT_R | OFF | R statistical computing |
| GC_WANT_HTTP | ON | HTTP API web services |

### Dependencies

- **Qt6 Core/Network/Widgets** - UI and networking
- **Qt6 Charts** - Data visualization
- **Qt6 Core5Compat** - Qt5 compatibility layer (for QXmlDefaultHandler, QRegExp)
- **GSL** - Scientific computing
- **ZLIB** - Compression

---

## Qt6 Migration Notes

### Completed Migrations

| Component | Issue | Solution |
|-----------|-------|----------|
| Perspective.h | QXmlDefaultHandler unavailable | Added `#include <QtCore5Compat/QXmlDefaultHandler>` |
| RideFile.cpp | QRegExp deprecated | Migrated to `QRegularExpression` with `CaseInsensitiveOption` |
| KurtInRide.cpp | QBluetoothUuid::toUInt128() removed | Used `QUuid` conversion with `toByteArray()` |
| KurtInRide.cpp | QBluetoothAddress::toUInt64() removed | Parse address string with `QByteArray::fromHex()` |

### Qt6 API Changes

#### QRegExp → QRegularExpression

```cpp
// Qt5
QRegExp rx("pattern", Qt::CaseInsensitive);

// Qt6
QRegularExpression rx("pattern");
rx.setPatternOption(QRegularExpression::CaseInsensitiveOption);
```

#### QBluetooth Address Handling

```cpp
// Qt5
const QBluetoothAddress addr = devinfo.address();
uint64_t addr64 = addr.toUInt64();

// Qt6
QString addrStr = devinfo.address().toString();
QByteArray addrBytes = QByteArray::fromHex(addrStr.remove(':').toLatin1());
```

#### UUID Handling

```cpp
// Qt5
quint128 uuid = uuidObj.toUInt128();

// Qt6
QUuid quuid = uuidObj;  // Implicit conversion
QByteArray uuidBytes = quuid.toByteArray();
```

---

*Document Version 1.2 | GoldenCheetah AI Coach Module*
*Last Updated: February 2025*
