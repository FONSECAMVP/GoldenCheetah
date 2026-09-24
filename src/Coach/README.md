# GoldenCheetah AI Coach Module

AI-powered cycling coach that provides personalized training guidance based on your GoldenCheetah data.

## Features

- **Multiple LLM Providers**: OpenAI (GPT-4o), Anthropic (Claude), Google (Gemini)
- **Coaching Phases**: Assessment, Analysis, Planning, Daily Coaching
- **Data Integration**: PMC, HRV, ride history, performance metrics
- **Streaming Responses**: Real-time AI responses
- **Rate Limiting**: Built-in protection against API limits

## Quick Start

### 1. Configure API Key

Copy the template and add your API key:

```bash
cp Secrets.h.template Secrets.h
# Edit Secrets.h and add your API key
```

### 2. Build

```bash
cd build
cmake ..
make Coach
```

### 3. Use

1. Open GoldenCheetah
2. Navigate to Coach tab
3. Click Settings to configure
4. Start chatting with your AI coach!

## Supported Models

### OpenAI
- gpt-4o (recommended)
- gpt-4o-mini
- gpt-4-turbo

### Anthropic
- claude-3-5-sonnet-20241022 (recommended)
- claude-3-5-haiku-20241022
- claude-3-opus-20240229

### Google Gemini
- gemini-1.5-pro (recommended)
- gemini-1.5-flash
- gemini-1.0-pro

## Architecture

```
CoachChatWidget (UI)
├── LLMService (Abstract)
│   ├── OpenAIClient
│   ├── AnthropicClient
│   └── GeminiClient
├── AthleteContextAggregator (Data)
└── PromptBuilder (Prompts)
```

## Files

| File | Purpose |
|------|---------|
| [`LLMService.h`](LLMService.h) | Abstract LLM interface |
| [`OpenAIClient.cpp`](OpenAIClient.cpp) | OpenAI implementation |
| [`AnthropicClient.cpp`](AnthropicClient.cpp) | Anthropic implementation |
| [`GeminiClient.cpp`](GeminiClient.cpp) | Google Gemini implementation |
| [`AthleteContext.cpp`](AthleteContext.cpp) | Data aggregation |
| [`PromptBuilder.cpp`](PromptBuilder.cpp) | Prompt engineering |
| [`CoachChatWidget.cpp`](CoachChatWidget.cpp) | UI implementation |

## Documentation

- [Development Guide](../../docs/COACH_DEV_GUIDE.md) - Original specification
- [Implementation Details](../../docs/COACH_IMPLEMENTATION.md) - Technical documentation
- [User Guide](../../docs/COACH_USER_GUIDE.md) - End-user documentation

## Requirements

- Qt 5.15+
- CMake 3.16+
- C++17
- GoldenCheetah core libraries
- API key from at least one LLM provider

## Security

- API keys stored securely using QSettings
- Never logged or displayed
- Transmitted only over HTTPS
- Local conversation history only

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

## License

GNU General Public License v2.0 - see [COPYING](../../COPYING)

---

*For detailed documentation, see [`docs/COACH_DEV_GUIDE.md`](../../docs/COACH_DEV_GUIDE.md)*
