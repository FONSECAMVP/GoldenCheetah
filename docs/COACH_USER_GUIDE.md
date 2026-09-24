# GoldenCheetah AI Coach - User Guide

## Welcome to Your AI Coach!

The GoldenCheetah AI Coach is your personal cycling coach powered by advanced AI. It analyzes your training data and provides personalized guidance to help you achieve your goals.

## Getting Started

### 1. Setting Up Your API Key

The AI Coach requires an API key from one of the supported providers:

#### Option A: OpenAI (GPT-4o)
1. Visit [OpenAI Platform](https://platform.openai.com/)
2. Create an account or sign in
3. Navigate to API Keys section
4. Create a new API key
5. Copy the key

#### Option B: Anthropic (Claude)
1. Visit [Anthropic Console](https://console.anthropic.com/)
2. Create an account or sign in
3. Navigate to API Keys
4. Create a new API key
5. Copy the key

#### Option C: Google (Gemini)
1. Visit [Google AI Studio](https://makersuite.google.com/app/apikey)
2. Sign in with your Google account
3. Create an API key
4. Copy the key

### 2. Configuring GoldenCheetah

1. Open GoldenCheetah
2. Navigate to the Coach tab
3. Click the **Settings** button
4. Paste your API key
5. Select your preferred model
6. Click **Save**

## Using the AI Coach

### Chat Interface

The Coach interface consists of:

- **Phase Selector** - Choose your coaching phase (Assessment, Analysis, Planning, Coaching)
- **Chat Area** - View conversation history
- **Quick Questions** - Pre-written questions for common scenarios
- **Input Box** - Type your questions
- **Send Button** - Submit your message

### Coaching Phases

#### 1. Assessment Phase

**Purpose:** Understand your current fitness level and goals

**What to Ask:**
- "What are my current strengths and weaknesses?"
- "How fit am I right now?"
- "What should I focus on improving?"
- "Am I training too hard or not hard enough?"

**Example Conversation:**
```
You: What are my current strengths and weaknesses?

Coach: Based on your recent training data, here's what I see:

Strengths:
- Your CTL (fitness) is at 65, showing good base fitness
- Consistent training with 12 rides in the last 30 days
- Good recovery patterns with stable HRV

Weaknesses:
- Limited high-intensity work (most rides at endurance pace)
- Average power trending slightly downward
- Could benefit from more structured intervals

Would you like me to suggest specific workouts to address these areas?
```

#### 2. Analysis Phase

**Purpose:** Deep dive into your training patterns and performance

**What to Ask:**
- "Analyze my training load progression"
- "How is my intensity distribution?"
- "Am I recovering adequately?"
- "What patterns do you see in my training?"

**Example Conversation:**
```
You: Analyze my training load progression

Coach: Let me analyze your PMC data from the last 90 days:

Training Load Analysis:
- CTL (Fitness): Started at 45, now at 65 (+44%)
- ATL (Fatigue): Currently at 58
- TSB (Form): Currently at +7 (well rested)
- Ramp Rate: +2.8 per week (sustainable)

Key Observations:
1. Excellent progressive overload - you've built fitness steadily
2. Good balance between training and recovery
3. Current form suggests you're ready for harder training
4. No signs of overtraining

Recommendation: This is a great time to add some high-intensity work while maintaining your base volume.
```

#### 3. Planning Phase

**Purpose:** Create structured training plans for your goals

**What to Ask:**
- "Create a 12-week training plan"
- "How should I periodize my training?"
- "What workouts should I do this week?"
- "Help me prepare for my upcoming event"

**Example Conversation:**
```
You: Create a 12-week training plan for a century ride

Coach: I'll create a periodized plan for your century ride:

Phase 1: Base Building (Weeks 1-4)
Goal: Build aerobic endurance
- 4-5 rides per week
- Focus: Long steady rides (2-4 hours)
- Intensity: 65-75% FTP
- Weekly TSS: 400-500

Phase 2: Build (Weeks 5-8)
Goal: Increase sustainable power
- 4-5 rides per week
- Add: 2x20min threshold intervals
- Mix: Endurance + tempo rides
- Weekly TSS: 500-600

Phase 3: Peak (Weeks 9-11)
Goal: Race-specific fitness
- 4 rides per week
- Include: Long rides at race pace
- Maintain: Threshold work
- Weekly TSS: 550-650

Phase 4: Taper (Week 12)
Goal: Fresh for event
- 3 rides per week
- Reduce volume by 40%
- Maintain intensity
- Weekly TSS: 300-350

Would you like detailed workouts for Week 1?
```

#### 4. Daily Coaching Phase

**Purpose:** Day-to-day training guidance

**What to Ask:**
- "What should I do today?"
- "How am I progressing toward my goal?"
- "Should I rest or train today?"
- "Give me a specific workout recommendation"

**Example Conversation:**
```
You: What should I do today?

Coach: Based on your current status:

Current Metrics:
- TSB: -15 (moderate fatigue)
- HRV: 45ms (slightly below your average)
- Last hard workout: 2 days ago
- Recent training: 3 consecutive days

Today's Recommendation: RECOVERY RIDE

Workout Details:
- Duration: 60 minutes
- Intensity: 50-60% FTP (Zone 1-2)
- Focus: Easy spinning, high cadence (90+ rpm)
- Goal: Active recovery

Rationale:
Your body needs recovery after 3 days of training. Your HRV is slightly suppressed, indicating accumulated fatigue. An easy ride will promote blood flow and recovery without adding stress.

Tomorrow: If you feel recovered, we can do threshold intervals.
```

### Quick Questions

Click any suggested question to instantly ask the coach. Questions change based on your selected phase.

### Tips for Best Results

1. **Be Specific** - Instead of "How am I doing?", ask "How has my FTP progressed over the last 3 months?"

2. **Provide Context** - Mention upcoming events, injuries, or constraints: "I have a race in 6 weeks and can train 8 hours per week"

3. **Ask Follow-ups** - The coach remembers your conversation, so you can ask clarifying questions

4. **Use Different Phases** - Switch phases based on what you need:
   - Assessment: When starting or checking in
   - Analysis: When reviewing performance
   - Planning: When creating training plans
   - Coaching: For daily guidance

5. **Share Feedback** - Tell the coach how workouts went: "That threshold workout felt too hard" or "I felt great today"

## Understanding Your Data

### PMC Metrics

**CTL (Chronic Training Load)** - Your fitness level
- Higher = More fit
- Builds slowly over weeks/months
- Target: 60-100 for recreational riders, 100+ for competitive

**ATL (Acute Training Load)** - Your fatigue level
- Higher = More tired
- Changes quickly day-to-day
- Should be managed with recovery

**TSB (Training Stress Balance)** - Your form/freshness
- Positive = Fresh/rested
- Negative = Fatigued
- Target: -10 to +10 for training, +15 to +25 for racing

### HRV (Heart Rate Variability)

**RMSSD** - Recovery indicator
- Higher = Better recovered
- Declining trend = Need more recovery
- Stable/improving = Good adaptation

### Training Zones

**Zone 1 (Active Recovery)** - 0-55% FTP
- Easy spinning
- Recovery rides

**Zone 2 (Endurance)** - 56-75% FTP
- Base building
- Long rides

**Zone 3 (Tempo)** - 76-90% FTP
- Sustainable hard effort
- "All day" pace

**Zone 4 (Threshold)** - 91-105% FTP
- Lactate threshold
- 20-60 minute efforts

**Zone 5 (VO2max)** - 106-120% FTP
- Very hard
- 3-8 minute intervals

**Zone 6 (Anaerobic)** - 121-150% FTP
- Sprint power
- <3 minute efforts

## Privacy and Data

### What Data is Shared?

When you use the AI Coach, the following data is sent to the LLM provider:

- Your training metrics (power, heart rate, duration)
- PMC data (CTL, ATL, TSB)
- HRV data (if available)
- Ride summaries
- Your questions and conversation history

### What is NOT Shared?

- Your name (unless you provide it)
- GPS data or routes
- Personal information
- Data from other athletes

### Data Storage

- Conversation history is stored locally on your computer
- API keys are encrypted (on supported platforms)
- No data is sent to GoldenCheetah servers

## Troubleshooting

### "API key not configured"

**Solution:** Enter your API key in Settings

### "Request timed out"

**Possible Causes:**
- Slow internet connection
- LLM provider service issues
- Firewall blocking requests

**Solutions:**
- Check your internet connection
- Try again in a few minutes
- Check provider status page

### "Invalid response"

**Possible Causes:**
- API key expired or invalid
- Provider service issues
- Network proxy interfering

**Solutions:**
- Verify your API key is correct
- Try a different provider
- Check network settings

### Coach gives generic advice

**Solutions:**
- Make sure you have recent training data
- Be more specific in your questions
- Provide more context about your goals

### Streaming stops mid-response

**Solutions:**
- Check internet connection
- Increase timeout in settings
- Try non-streaming mode

## Cost Considerations

### API Pricing (Approximate)

**OpenAI GPT-4o:**
- Input: $2.50 per 1M tokens
- Output: $10.00 per 1M tokens
- Typical conversation: $0.01-0.05

**Anthropic Claude:**
- Input: $3.00 per 1M tokens
- Output: $15.00 per 1M tokens
- Typical conversation: $0.01-0.06

**Google Gemini:**
- Free tier: 60 requests per minute
- Paid: $0.35 per 1M tokens
- Typical conversation: $0.001-0.01

### Cost Management Tips

1. **Use Gemini for frequent queries** - Free tier is generous
2. **Be concise** - Shorter questions = lower cost
3. **Clear conversation history** - Reduces context size
4. **Use quick questions** - Pre-optimized prompts

## Frequently Asked Questions

**Q: Can the coach create workouts for my bike computer?**
A: Not yet, but this feature is planned for a future release.

**Q: Does the coach work offline?**
A: No, it requires an internet connection to communicate with the LLM provider.

**Q: Can I use multiple LLM providers?**
A: Yes! You can configure multiple API keys and switch between providers.

**Q: How accurate is the coaching advice?**
A: The coach uses evidence-based training principles and your actual data. However, it's not a replacement for a human coach, especially for complex situations.

**Q: Can I share my conversation with my real coach?**
A: Yes! You can copy and paste the conversation or export it (feature coming soon).

**Q: Does the coach learn from my feedback?**
A: Within a conversation, yes. But it doesn't retain information between sessions (privacy by design).

## Getting Help

### Support Resources

- **Documentation:** [GoldenCheetah Wiki](https://github.com/GoldenCheetah/GoldenCheetah/wiki)
- **Forum:** [GoldenCheetah Google Group](https://groups.google.com/g/golden-cheetah-users)
- **Issues:** [GitHub Issues](https://github.com/GoldenCheetah/GoldenCheetah/issues)

### Reporting Bugs

If you encounter a bug:
1. Note the exact error message
2. Describe what you were doing
3. Include your GoldenCheetah version
4. Report on GitHub Issues

## Best Practices

### For Beginners

1. Start with Assessment phase
2. Ask about your current fitness level
3. Set realistic goals
4. Follow the coach's progressive plan
5. Don't skip recovery days

### For Experienced Athletes

1. Use Analysis phase to identify weaknesses
2. Request specific workout prescriptions
3. Discuss periodization strategies
4. Ask about race preparation
5. Use for second opinions on training decisions

### For Coaches

1. Use as a tool for athlete education
2. Generate training plan templates
3. Analyze athlete data patterns
4. Explain training concepts
5. Supplement, don't replace, human coaching

## Advanced Features

### Custom Prompts

You can ask the coach to adopt specific coaching styles:

"Act as a coach focused on time-crunched athletes"
"Explain this like I'm a beginner"
"Give me the scientific explanation"

### Multi-Week Planning

Request detailed plans:

"Create a 16-week plan with specific workouts for each week"
"Design a polarized training plan for an Ironman"

### Performance Analysis

Deep dive into your data:

"Compare my performance this year vs last year"
"Analyze my power duration curve"
"What's my optimal training volume?"

## Conclusion

The GoldenCheetah AI Coach is a powerful tool to enhance your training. Use it regularly, ask specific questions, and combine its insights with your own experience and intuition.

Remember: The coach is here to support you, but you're ultimately in control of your training!

---

**Happy Training! 🚴**

*Last Updated: 2026-02-08*
*Version: 1.0*
