// Memento Mori - Pure Minimalist Design
// Nothing but the grid. Let the visual speak for itself.

// Configuration
let config = {
    person: {
        birthdate: "1987-08-17",
        expectedLifespan: 80
    },
    specialDays: []
};

// Display constants
const COLS = 52;  // weeks per year
const ROWS = 80;  // years of life
const DISPLAY_WIDTH = 800;
const DISPLAY_HEIGHT = 480;

// Current simulation date (for testing)
let currentDate = new Date();

// Calculate weeks lived since birthdate (birthday-based years)
function calculateWeeksLived(birthdate, referenceDate = new Date()) {
    const birth = new Date(birthdate);
    const now = new Date(referenceDate);

    // Calculate complete years (birthday to birthday)
    let age = now.getFullYear() - birth.getFullYear();

    // Check if birthday has occurred this year
    const thisYearBirthday = new Date(now.getFullYear(), birth.getMonth(), birth.getDate());
    if (now < thisYearBirthday) {
        age--; // Birthday hasn't happened yet this year
    }

    // Calculate weeks into current year (since last birthday)
    const lastBirthday = new Date(now.getFullYear(), birth.getMonth(), birth.getDate());
    if (now < thisYearBirthday) {
        lastBirthday.setFullYear(lastBirthday.getFullYear() - 1);
    }

    const weeksSinceLastBirthday = Math.floor((now - lastBirthday) / (1000 * 60 * 60 * 24 * 7));

    // Total weeks = complete years × 52 + weeks into current year
    return (age * 52) + weeksSinceLastBirthday;
}

// Calculate total weeks for lifespan
function calculateTotalWeeks(lifespan) {
    return lifespan * 52;
}

// Check if today is a special day (using Eastern Time)
function checkSpecialDay(date, specialDays) {
    // Convert to Eastern Time (America/New_York) for special day detection
    const options = { timeZone: 'America/New_York', month: '2-digit', day: '2-digit' };
    const formatter = new Intl.DateTimeFormat('en-US', options);
    const parts = formatter.formatToParts(date);

    const month = parts.find(part => part.type === 'month').value;
    const day = parts.find(part => part.type === 'day').value;
    const today = `${month}-${day}`;

    for (const specialDay of specialDays) {
        if (specialDay.date === today) {
            return specialDay;
        }
    }
    return null;
}

// Render grid - small circular dots with spacing
function renderGrid(ctx, weeksLived, totalWeeks) {
    // Pure white background
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Grid: 52 columns (weeks) × 80 rows (years)
    // Use full display with circular dots
    const cellWidth = DISPLAY_WIDTH / COLS;   // ~15px
    const cellHeight = DISPLAY_HEIGHT / ROWS; // 6px

    // Make dots small enough to see spacing
    const dotRadius = Math.min(cellWidth, cellHeight) / 3; // Small circles with breathing room

    // Center any remaining pixels
    const totalGridWidth = COLS * cellWidth;
    const totalGridHeight = ROWS * cellHeight;
    const offsetX = (DISPLAY_WIDTH - totalGridWidth) / 2;
    const offsetY = (DISPLAY_HEIGHT - totalGridHeight) / 2;

    // Draw circular dots
    for (let row = 0; row < ROWS; row++) {
        for (let col = 0; col < COLS; col++) {
            const weekIndex = row * COLS + col;

            // Center of this cell
            const x = offsetX + (col * cellWidth) + cellWidth / 2;
            const y = offsetY + (row * cellHeight) + cellHeight / 2;

            ctx.beginPath();
            ctx.arc(x, y, dotRadius, 0, Math.PI * 2);

            if (weekIndex < weeksLived) {
                // Past: solid black circle
                ctx.fillStyle = '#000000';
                ctx.fill();
            } else if (weekIndex < totalWeeks) {
                // Future: subtle outline circle
                ctx.strokeStyle = '#d8d8d8';
                ctx.lineWidth = 1;
                ctx.stroke();
            }
        }
    }
}

// Render special day - minimal quote display
function renderSpecialDay(ctx, specialDay, weeksLived, totalWeeks) {
    // Pure white background
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Centered quote - nothing else
    ctx.fillStyle = 'black';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    // Quote only - large, serif, centered
    ctx.font = '36px serif';
    const lines = wrapText(ctx, specialDay.quote, DISPLAY_WIDTH - 120);
    const lineHeight = 50;
    const startY = (DISPLAY_HEIGHT / 2) - ((lines.length - 1) * lineHeight / 2);

    lines.forEach((line, index) => {
        ctx.fillText(line, DISPLAY_WIDTH / 2, startY + (index * lineHeight));
    });
}

// Word wrap utility
function wrapText(ctx, text, maxWidth) {
    const words = text.split(' ');
    const lines = [];
    let currentLine = '';

    words.forEach(word => {
        const testLine = currentLine + word + ' ';
        const metrics = ctx.measureText(testLine);

        if (metrics.width > maxWidth && currentLine !== '') {
            lines.push(currentLine.trim());
            currentLine = word + ' ';
        } else {
            currentLine = testLine;
        }
    });

    if (currentLine.trim() !== '') {
        lines.push(currentLine.trim());
    }

    return lines;
}

// Main render function
function render() {
    const canvas = document.getElementById('display');
    const ctx = canvas.getContext('2d');

    const weeksLived = calculateWeeksLived(config.person.birthdate, currentDate);
    const totalWeeks = calculateTotalWeeks(config.person.expectedLifespan);

    // Check for special day
    const specialDay = checkSpecialDay(currentDate, config.specialDays);

    // Update info display
    updateInfo(weeksLived, totalWeeks, specialDay);

    // Simple logic: special day = show quote, otherwise show grid
    if (specialDay) {
        renderSpecialDay(ctx, specialDay, weeksLived, totalWeeks);
    } else {
        renderGrid(ctx, weeksLived, totalWeeks);
    }
}

// Update information display (below canvas)
function updateInfo(weeksLived, totalWeeks, specialDay) {
    const weeksInfo = document.getElementById('weeksInfo');
    const specialDayInfo = document.getElementById('specialDayInfo');

    const birthDate = new Date(config.person.birthdate);
    const age = currentDate.getFullYear() - birthDate.getFullYear() -
                (currentDate < new Date(currentDate.getFullYear(), birthDate.getMonth(), birthDate.getDate()) ? 1 : 0);

    weeksInfo.innerHTML = `
        <strong>Weeks Lived:</strong> ${weeksLived} of ${totalWeeks} (${((weeksLived / totalWeeks) * 100).toFixed(1)}%)<br>
        <strong>Current Age:</strong> ${age} years<br>
        <strong>Simulated Date:</strong> ${currentDate.toLocaleDateString()}
    `;

    if (specialDay) {
        specialDayInfo.innerHTML = `<strong>Special Day:</strong> ${specialDay.title}`;
        specialDayInfo.style.display = 'block';
    } else {
        specialDayInfo.style.display = 'none';
    }
}

// Load configuration
async function loadConfig() {
    try {
        const response = await fetch('config.json');
        const loadedConfig = await response.json();
        config = { ...config, ...loadedConfig };
        render();
    } catch (error) {
        console.log('Config file not found, using defaults');
        render();
    }
}

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    // Set current date input
    const dateInput = document.getElementById('currentDate');
    dateInput.valueAsDate = currentDate;

    // Event listeners
    dateInput.addEventListener('change', (e) => {
        // Add time component to avoid UTC midnight shifting to previous day in local time
        currentDate = new Date(e.target.value + 'T12:00:00');
        render();
    });

    document.getElementById('resetDate').addEventListener('click', () => {
        currentDate = new Date();
        dateInput.valueAsDate = currentDate;
        render();
    });

    // Load config and render
    loadConfig();
});
