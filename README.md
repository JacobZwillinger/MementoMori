# Memento Mori

A minimalist life visualization for the reTerminal E1001 7.5" e-paper display. Each dot represents one week of your life - a visual reminder that time is finite.

## Concept

- **52 columns** (weeks per year) × **80 rows** (years of life) = 4,160 weeks total
- **Filled dots**: weeks you've lived
- **Unfilled dots**: weeks remaining (assuming 80-year lifespan)
- **Special days**: On configured dates (birthday, memorial days), display shows a centered stoic quote instead of the grid

## Hardware

- **Display**: reTerminal E1001 (800×480 monochrome e-paper)
- **Processor**: ESP32-S3
- **Battery Life**: 3+ months with daily midnight updates
- **Update Schedule**: Daily at midnight, minimal power consumption via deep sleep

## Project Structure

```
MementoMori/
├── web-preview/          # Browser-based preview (develop here first)
│   ├── index.html        # Main preview interface
│   ├── memento.js        # Core rendering logic
│   ├── config.json       # Configuration (birthdate, special days, quotes)
│   ├── styles.css        # Styling
│   └── test-special-days.html  # Test suite for special day detection
│
└── arduino/              # ESP32 firmware (TODO: coming soon)
    ├── MementoMori.ino
    ├── config.h
    └── data/
        └── config.json
```

## Web Preview

1. **Start local server:**
   ```bash
   cd web-preview
   python3 -m http.server 8888
   ```

2. **Open in browser:**
   ```
   http://localhost:8888/index.html
   ```

3. **Test special day detection:**
   ```
   http://localhost:8888/test-special-days.html
   ```

4. **Simulate dates:**
   Use the date picker to test different dates, including your birthday and special days

## Configuration

Edit `web-preview/config.json`:

```json
{
  "person": {
    "birthdate": "1987-08-17",
    "expectedLifespan": 80,
    "timezone": "America/Los_Angeles"
  },
  "specialDays": [
    {
      "date": "08-17",
      "title": "Your Birthday",
      "quote": "It is not that we have a short time to live, but that we waste much of it.\n— Seneca"
    }
  ]
}
```

**Note**: Special days use `MM-DD` format and recur every year.

## Features

- **Birthday-based week calculation**: Rows align with your age (week counter resets on your birthday)
- **Pure minimalist design**: White background, black dots, no distractions
- **Special day quotes**: Stoic philosophy quotes from Seneca, Marcus Aurelius, Epictetus
- **Low power**: Deep sleep between updates, WiFi only for weekly NTP sync
- **Easy configuration**: JSON-based, no code changes needed

## Development Workflow

1. **Make changes in web preview** - instant visual feedback
2. **Test thoroughly** - use date simulator and test suite
3. **Port to Arduino** - once web version is verified
4. **Deploy to hardware** - upload via Arduino IDE

## Philosophy

*"Memento Mori"* - Remember that you must die.

Each dot is a week. You can see your entire life at a glance. The filled dots show where you've been. The unfilled dots show what remains. This is not morbid - it's liberating. When you remember that time is finite, you're free to make each week count.

## License

MIT

## Credits

Inspired by stoic philosophy and the concept of time visualization pioneered by Wait But Why's "Your Life in Weeks" article.
