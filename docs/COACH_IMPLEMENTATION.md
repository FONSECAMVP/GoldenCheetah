# GoldenCheetah AI Coach - Implementation Documentation

## Overview

This document provides detailed implementation information for developers working on the GoldenCheetah AI Coach module.

## Architecture

### Component Hierarchy

```
CoachChatWidget (UI Layer)
    ├── LLMService (Abstract Interface)
    │   ├── OpenAIClient (GPT-4o)
    │   ├── AnthropicClient (Claude)
    │   └── GeminiClient (Gemini)
    ├── AthleteContextAggregator (Data Layer)
    └── PromptBuilder (Prompt Engineering)
```

### Data Flow

1. **User Input** → [`CoachChatWidget::onSendMessage()`](../src/Coach/CoachChatWidget.cpp:182)
2. **Context Aggregation** → [`AthleteContextAggregator::getProfile()`](../src/Coach/AthleteContext.cpp:33)
3. **Prompt Construction** → [`PromptBuilder::buildSystemPrompt()`](../src/Coach/PromptBuilder.cpp:23)
4. **LLM Request** → [`LLMService::sendMessageStream()`](../src/Coach/LLMService.h:78)
5. **Response Streaming** → [`CoachChatWidget::onStreamingChunk()`](../src/Coach/CoachChatWidget.cpp:217)
6. **UI Update** → Display in chat widget

## Core Components

### 1. LLMService (Abstract Interface)

**File:** [`src/Coach/LLMService.h`](../src/Coach/LLMService.h)

**Purpose:** Provides a unified interface for all LLM providers.

**Key Methods:**
- [`configure()`](../src/Coach/LLMService.h:60) - Configure provider settings
- [`sendMessage()`](../src/Coach/LLMService.h:77) - Send non-streaming request
- [`sendMessageStream()`](../src/Coach/LLMService.h:78) - Send streaming request
- [`addSystemMessage()`](../src/Coach/LLMService.h:71) - Set system prompt
- [`clearConversation()`](../src/Coach/LLMService.h:74) - Reset conversation history

**Signals:**
- `responseReady(Response)` - Emitted when full response received
- `streamingChunk(QString)` - Emitted for each streaming chunk
- `streamingFinished()` - Emitted when streaming completes
- `streamingError(QString)` - Emitted on streaming error

### 2. OpenAIClient

**Files:** [`src/Coach/OpenAIClient.h`](../src/Coach/OpenAIClient.h), [`src/Coach/OpenAIClient.cpp`](../src/Coach/OpenAIClient.cpp)

**Supported Models:**
- gpt-4o (default)
- gpt-4o-mini
- gpt-4-turbo
- gpt-4
- gpt-3.5-turbo

**API Endpoint:** `https://api.openai.com/v1/chat/completions`

**Authentication:** Bearer token in Authorization header

**Streaming Format:** Server-Sent Events (SSE)

### 3. AnthropicClient

**Files:** [`src/Coach/AnthropicClient.h`](../src/Coach/AnthropicClient.h), [`src/Coach/AnthropicClient.cpp`](../src/Coach/AnthropicClient.cpp)

**Supported Models:**
- claude-3-5-sonnet-20241022 (default)
- claude-3-5-haiku-20241022
- claude-3-opus-20240229
- claude-3-sonnet-20240229
- claude-3-haiku-20240307

**API Endpoint:** `https://api.anthropic.com/v1/messages`

**Authentication:** x-api-key header

**Streaming Format:** Server-Sent Events (SSE) with event types

### 4. GeminiClient

**Files:** [`src/Coach/GeminiClient.h`](../src/Coach/GeminiClient.h), [`src/Coach/GeminiClient.cpp`](../src/Coach/GeminiClient.cpp)

**Supported Models:**
- gemini-1.5-pro (default)
- gemini-1.5-flash
- gemini-1.0-pro

**API Endpoint:** `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent`

**Authentication:** API key in URL query parameter

**Streaming Format:** Newline-delimited JSON

### 5. AthleteContextAggregator

**Files:** [`src/Coach/AthleteContext.h`](../src/Coach/AthleteContext.h), [`src/Coach/AthleteContext.cpp`](../src/Coach/AthleteContext.cpp)

**Purpose:** Aggregates athlete data from GoldenCheetah core for LLM consumption.

**Key Methods:**
- [`updateContext()`](../src/Coach/AthleteContext.cpp:33) - Refresh athlete data
- [`getProfile()`](../src/Coach/AthleteContext.h:107) - Get current athlete profile
- [`getRecentRides()`](../src/Coach/AthleteContext.cpp:103) - Get ride history
- [`getPMCHistory()`](../src/Coach/AthleteContext.cpp:143) - Get PMC data
- [`getHRVHistory()`](../src/Coach/AthleteContext.cpp:161) - Get HRV data
- [`analyzePatterns()`](../src/Coach/AthleteContext.cpp:195) - Analyze training patterns
- [`getDailyRecommendations()`](../src/Coach/AthleteContext.cpp:202) - Get training recommendations

**Data Structures:**
- `AthleteProfile` - Complete athlete profile with PMC, HRV, performance metrics
- `RideSummary` - Individual ride summary
- `PatternAnalysis` - Detected patterns and insights
- `TrainingRecommendation` - Daily training recommendations

### 6. PromptBuilder

**Files:** [`src/Coach/PromptBuilder.h`](../src/Coach/PromptBuilder.h), [`src/Coach/PromptBuilder.cpp`](../src/Coach/PromptBuilder.cpp)

**Purpose:** Constructs domain-specific prompts for different coaching phases.

**Coaching Phases:**
1. **Assessment** - Initial athlete evaluation
2. **Analysis** - Training data analysis
3. **Planning** - Training plan creation
4. **Coaching** - Daily coaching guidance

**Key Methods:**
- [`buildSystemPrompt()`](../src/Coach/PromptBuilder.cpp:23) - Build phase-specific system prompt
- [`buildAssessmentPrompt()`](../src/Coach/PromptBuilder.cpp:35) - Assessment phase prompt
- [`buildAnalysisPrompt()`](../src/Coach/PromptBuilder.cpp:48) - Analysis phase prompt
- [`buildPlanningPrompt()`](../src/Coach/PromptBuilder.cpp:67) - Planning phase prompt
- [`buildDailyCoachingPrompt()`](../src/Coach/PromptBuilder.cpp:88) - Daily coaching prompt
- [`getSuggestedFollowups()`](../src/Coach/PromptBuilder.cpp:289) - Get suggested questions

**Formatting Methods:**
- [`formatAthleteBrief()`](../src/Coach/PromptBuilder.cpp:327) - Format athlete summary
- [`formatRideHistory()`](../src/Coach/PromptBuilder.cpp:356) - Format ride history
- [`formatTrainingPlan()`](../src/Coach/PromptBuilder.cpp:387) - Format recommendations

### 7. CoachChatWidget

**Files:** [`src/Coach/CoachChatWidget.h`](../src/Coach/CoachChatWidget.h), [`src/Coach/CoachChatWidget.cpp`](../src/Coach/CoachChatWidget.cpp)

**Purpose:** Main UI component for coach interaction.

**UI Components:**
- Header with phase selector and settings
- Scrollable chat area
- Quick question suggestions
- Input text area with send button
- Loading indicator

**Key Methods:**
- [`onSendMessage()`](../src/Coach/CoachChatWidget.cpp:182) - Handle user message
- [`onStreamingChunk()`](../src/Coach/CoachChatWidget.cpp:217) - Handle streaming response
- [`onPhaseChanged()`](../src/Coach/CoachChatWidget.cpp:281) - Handle phase change
- [`updateSuggestions()`](../src/Coach/CoachChatWidget.cpp:353) - Update quick questions

## Configuration

### API Keys

API keys are loaded from [`Secrets.h`](../src/Secrets.h) or environment variables:

```cpp
// In Secrets.h
#define GCOpenAIAPIKey "your-openai-key"
#define GCAnthropicAPIKey "your-anthropic-key"
#define GCGeminiAPIKey "your-gemini-key"
```

### Settings Storage

Settings are stored using QSettings:

```cpp
QSettings settings;
settings.setValue("coach/openai_apikey", apiKey);
settings.setValue("coach/model", "gpt-4o");
settings.setValue("coach/temperature", 0.7);
```

## Building

### Prerequisites

- Qt 5.15+
- CMake 3.16+
- C++17 compiler
- GoldenCheetah core libraries

### Build Commands

```bash
cd build
cmake ..
make Coach
```

### CMake Configuration

The Coach module is defined in [`src/Coach/CMakeLists.txt`](../src/Coach/CMakeLists.txt):

```cmake
add_library(Coach MODULE ${COACH_SOURCES})
target_link_libraries(Coach
    PRIVATE
        GoldenCheetah::Core
        GoldenCheetah::Metrics
        GoldenCheetah::Gui
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Widgets
)
```

## Testing

### Unit Tests

Unit tests should cover:
- LLM client request/response handling
- Prompt construction
- Data aggregation
- Error handling

### Integration Tests

Integration tests should verify:
- End-to-end conversation flow
- Multiple LLM provider switching
- Context updates
- Streaming response handling

### Test Coverage Target

**90% code coverage** across all modules.

## Error Handling

### Network Errors

All LLM clients handle:
- Connection timeouts (60s default)
- Authentication failures
- Rate limiting
- SSL errors (development mode)

### Data Errors

- Missing athlete data (graceful degradation)
- Invalid API responses (error messages)
- Empty conversation history (welcome message)

### User Feedback

Errors are displayed in the chat interface with clear, actionable messages.

## Performance Considerations

### Response Times

| Scenario | Typical | 95th Percentile |
|----------|---------|-----------------|
| Simple query | 2-4s | 6s |
| Complex analysis | 5-10s | 15s |
| Streaming start | <1s | 2s |

### Token Limits

- OpenAI: 128K context window
- Anthropic: 200K context window
- Gemini: 1M context window

### Optimization Strategies

1. **Smart Summarization** - Summarize old conversation history
2. **Selective Context** - Include only relevant data
3. **Streaming** - Use streaming for better UX
4. **Caching** - Cache frequently used prompts (TODO)

## Security

### API Key Storage

- Keys stored in QSettings (encrypted on supported platforms)
- Never logged or displayed in UI
- Transmitted only over HTTPS

### Data Privacy

- No athlete data sent to external servers except LLM APIs
- Conversation history stored locally only
- User controls data sharing

## Extending the Module

### Adding a New LLM Provider

1. Create new class inheriting from [`LLMService`](../src/Coach/LLMService.h)
2. Implement all virtual methods
3. Add to [`LLMService::Provider`](../src/Coach/LLMService.h:34) enum
4. Update [`CMakeLists.txt`](../src/Coach/CMakeLists.txt)
5. Add configuration UI

### Adding New Coaching Phases

1. Add phase to [`PromptBuilder::CoachingPhase`](../src/Coach/PromptBuilder.h:31) enum
2. Implement phase-specific prompt in [`PromptBuilder`](../src/Coach/PromptBuilder.cpp)
3. Add phase to UI combo box
4. Create suggested questions

### Adding New Data Sources

1. Extend [`AthleteProfile`](../src/Coach/AthleteContext.h:36) structure
2. Implement data retrieval in [`AthleteContextAggregator`](../src/Coach/AthleteContext.cpp)
3. Update prompt formatting methods
4. Add to system prompts

## Troubleshooting

### Common Issues

**Issue:** "API key not configured"
**Solution:** Set API key in Secrets.h or settings

**Issue:** "Request timed out"
**Solution:** Check network connection, increase timeout

**Issue:** "Invalid JSON response"
**Solution:** Check API endpoint URL, verify API key

**Issue:** Streaming not working
**Solution:** Verify SSE support, check network proxy settings

### Debug Logging

Enable debug logging:

```cpp
qDebug() << "Coach: Sending message:" << message;
qDebug() << "Coach: Received chunk:" << chunk;
```

## Future Enhancements

### Planned Features

1. **PhaseManager** - Automatic phase transitions
2. **MainWindow Integration** - Sidebar integration
3. **CalendarBridge** - Training plan synchronization
4. **Notification System** - Proactive coaching alerts
5. **Response Caching** - Improve performance
6. **Rate Limiting** - Protect against API limits
7. **Multi-language Support** - Internationalization
8. **Voice Input** - Speech-to-text integration

### Technical Debt

- Basic summarization algorithm (needs improvement)
- No response caching (performance impact)
- Limited error recovery (needs retry logic)
- SSL certificate validation disabled in development

## Contributing

### Code Style

- Follow GoldenCheetah coding standards
- Use camelCase for variables, PascalCase for classes
- 4-space indentation
- Doxygen-style comments

### Pull Request Process

1. Fork repository
2. Create feature branch
3. Implement with tests
4. Update documentation
5. Submit PR with detailed description

### Review Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] Documentation updated
- [ ] No memory leaks
- [ ] Error handling implemented
- [ ] Code style consistent

## References

- [OpenAI API Documentation](https://platform.openai.com/docs/api-reference)
- [Anthropic API Documentation](https://docs.anthropic.com/claude/reference)
- [Google Gemini API Documentation](https://ai.google.dev/docs)
- [GoldenCheetah Developer Guide](https://github.com/GoldenCheetah/GoldenCheetah/wiki)

---

*Last Updated: 2026-02-08*
*Version: 1.0*
