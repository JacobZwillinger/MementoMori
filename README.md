# Memento Mori

A minimalist life visualization displaying your entire life as a grid of dots on an e-paper display. Each dot represents one week—a visual reminder that time is finite.

## Overview

**4,160 weeks. That's 80 years.**

- **52 columns** (weeks per year) × **80 rows** (years of life)
- **Filled dots**: weeks you've lived
- **Empty dots**: time remaining
- **Special days**: Display Stoic quotes on configured dates

## Hardware

- **Display**: reTerminal E1001 (800×480 e-paper, black and white)
- **Orientations**: Horizontal (800×480) or Vertical (480×800)
- **Processor**: Raspberry Pi CM4
- **Power**: Deep sleep support for extended battery life

## Design Philosophy

Built with radical minimalism and UX principles:

- **Hierarchy**: Grid dominates, typography recedes
- **Information Architecture**: Centered title, top-right battery indicator
- **Typography**: Ultra-light system fonts (300 weight), uppercase geometric forms
- **Breathing Space**: Generous whitespace for contemplative viewing
- **User Intent**: Immediate emotional impact through data visualization

## Project Structure

```
MementoMori/
├── web-preview/
│   ├── index.html              # Horizontal layout (800×480)
│   ├── vertical-designs.html   # Vertical layout (480×800) ⭐
│   ├── memento.js             # Core rendering logic
│   ├── config.json            # Personal configuration
│   └── styles.css             # Preview styling
│
└── arduino/
    └── MementoMori/           # Hardware implementation
```

## Quick Start

1. **Start local server:**
   ```bash
   cd web-preview
   python3 -m http.server 8888
   ```

2. **View designs:**
   - Horizontal: `http://localhost:8888/index.html`
   - Vertical: `http://localhost:8888/vertical-designs.html` ⭐

3. **Test features:**
   - Use date picker to simulate different dates
   - Click "Birthday" button to jump to your birthday
   - Adjust battery slider to test indicator (1-5 dots)

## Configuration

Edit `web-preview/config.json`:

```json
{
  "person": {
    "birthdate": "1987-08-17",
    "expectedLifespan": 80,
    "timezone": "America/New_York"
  },
  "specialDays": [
    {
      "date": "08-17",
      "title": "Your Birthday",
      "quote": "It is not that we have a short time to live, but that we waste much of it.\n—Seneca"
    }
  ]
}
```

**Important Typography Note**: Attribution uses em dash attached to author name (`—Seneca`) for proper typographic hierarchy.

## Key Features

### Core Visualization
- **Birthday-aligned weeks**: Grid rows align with your age (resets on your birthday)
- **Pure minimalism**: White background, black filled dots, light gray outlines
- **Optical balance**: Centered title, minimal battery indicator

### Battery Indicator
- Displays as 1-5 dots in top-right corner
- No decorative icons, just pure data
- Maps percentage: 1-20% = 1 dot, 21-40% = 2 dots, etc.

### Special Days
- Replaces grid with centered philosophical quote
- Stoic quotes from Seneca, Marcus Aurelius, Epictetus
- Quote text word-wraps, attribution stays on single line
- Timezone-aware detection (Eastern Time)

### Vertical Layout (New)
- **480×800 orientation** for vertical displays
- Edge-to-edge grid for maximum visual impact
- Centered "MEMENTO MORI" title (18px, weight 300)
- Same dot-based battery indicator
- Optimized for emotional impact

## Philosophy

*"Memento Mori"* - Remember that you must die.

Each dot is a week. You can see your entire life at a glance. The filled dots show where you've been. The unfilled dots show what remains. This is not morbid - it's liberating. When you remember that time is finite, you're free to make each week count.

## License

MIT

## Credits

Inspired by stoic philosophy and the concept of time visualization pioneered by Wait But Why's "Your Life in Weeks" article.
