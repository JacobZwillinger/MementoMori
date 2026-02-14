// Multiple layout options for Memento Mori

// OPTION 1: Archimedean Spiral (from center outward)
function renderSpiral(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const centerX = 400;
    const centerY = 240;
    const dotSize = 4;
    const angleStep = 0.3;
    const radiusGrowth = 0.18;

    for (let i = 0; i < totalWeeks; i++) {
        const angle = i * angleStep;
        const radius = i * radiusGrowth;
        const x = centerX + Math.cos(angle) * radius;
        const y = centerY + Math.sin(angle) * radius;

        if (i < weeksLived) {
            ctx.fillStyle = '#000000';
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.fill();
        } else {
            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.stroke();
        }
    }
}

// OPTION 2: Concentric Circles (tree rings)
function renderConcentric(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const centerX = 400;
    const centerY = 240;
    const weeksPerRing = 52; // One ring per year
    const maxRadius = 220; // Fit in display
    const years = 80;

    for (let year = 0; year < years; year++) {
        const radius = (year + 1) * (maxRadius / years);
        const weeksInYear = 52;
        const angleStep = (Math.PI * 2) / weeksInYear;

        for (let week = 0; week < weeksInYear; week++) {
            const weekIndex = year * 52 + week;
            const angle = week * angleStep - Math.PI / 2; // Start at top

            const x = centerX + Math.cos(angle) * radius;
            const y = centerY + Math.sin(angle) * radius;

            if (weekIndex < weeksLived) {
                ctx.fillStyle = '#000000';
                ctx.beginPath();
                ctx.arc(x, y, 2, 0, Math.PI * 2);
                ctx.fill();
            } else if (weekIndex < totalWeeks) {
                ctx.strokeStyle = '#e0e0e0';
                ctx.lineWidth = 1;
                ctx.beginPath();
                ctx.arc(x, y, 2, 0, Math.PI * 2);
                ctx.stroke();
            }
        }
    }
}

// OPTION 3: Wave/Sine Pattern
function renderWave(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const dotSize = 4;
    const cols = 80; // weeks across
    const rows = Math.ceil(totalWeeks / cols);
    const cellWidth = 800 / cols;
    const cellHeight = 480 / rows;
    const amplitude = 30;
    const frequency = 0.1;

    for (let i = 0; i < totalWeeks; i++) {
        const col = i % cols;
        const row = Math.floor(i / cols);

        const baseX = col * cellWidth + cellWidth / 2;
        const baseY = row * cellHeight + cellHeight / 2;

        // Add sine wave offset
        const waveOffset = Math.sin(i * frequency) * amplitude;
        const x = baseX;
        const y = baseY + waveOffset;

        if (i < weeksLived) {
            ctx.fillStyle = '#000000';
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.fill();
        } else {
            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.stroke();
        }
    }
}

// OPTION 4: Horizontal Timeline (years as rows)
function renderTimeline(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const years = 80;
    const weeksPerYear = 52;
    const rowHeight = 480 / years; // 6px per year
    const cellWidth = 800 / weeksPerYear; // ~15px per week
    const dotWidth = cellWidth - 1;
    const dotHeight = rowHeight - 1;

    for (let year = 0; year < years; year++) {
        for (let week = 0; week < weeksPerYear; week++) {
            const weekIndex = year * weeksPerYear + week;
            const x = week * cellWidth;
            const y = year * rowHeight;

            if (weekIndex < weeksLived) {
                ctx.fillStyle = '#000000';
                ctx.fillRect(x, y, dotWidth, dotHeight);
            } else if (weekIndex < totalWeeks) {
                ctx.strokeStyle = '#f0f0f0';
                ctx.lineWidth = 1;
                ctx.strokeRect(x + 0.5, y + 0.5, dotWidth - 1, dotHeight - 1);
            }
        }
    }
}

// OPTION 5: Circular Progress (single ring)
function renderCircularProgress(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const centerX = 400;
    const centerY = 240;
    const radius = 200;
    const dotSize = 3;

    for (let i = 0; i < totalWeeks; i++) {
        const angle = (i / totalWeeks) * Math.PI * 2 - Math.PI / 2; // Start at top
        const x = centerX + Math.cos(angle) * radius;
        const y = centerY + Math.sin(angle) * radius;

        if (i < weeksLived) {
            ctx.fillStyle = '#000000';
            ctx.beginPath();
            ctx.arc(x, y, dotSize, 0, Math.PI * 2);
            ctx.fill();
        } else {
            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(x, y, dotSize, 0, Math.PI * 2);
            ctx.stroke();
        }
    }
}

// OPTION 6: Fibonacci Spiral
function renderFibonacci(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const centerX = 400;
    const centerY = 240;
    const phi = (1 + Math.sqrt(5)) / 2; // Golden ratio
    const dotSize = 4;

    for (let i = 0; i < totalWeeks; i++) {
        const angle = i * 2 * Math.PI / phi;
        const radius = 3 * Math.sqrt(i);

        const x = centerX + Math.cos(angle) * radius;
        const y = centerY + Math.sin(angle) * radius;

        if (i < weeksLived) {
            ctx.fillStyle = '#000000';
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.fill();
        } else {
            ctx.strokeStyle = '#e0e0e0';
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.arc(x, y, dotSize / 2, 0, Math.PI * 2);
            ctx.stroke();
        }
    }
}

// OPTION 7: Vertical Columns (weeks as columns, years as rows)
function renderVerticalColumns(ctx, weeksLived, totalWeeks) {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 800, 480);

    const weeks = 52;
    const years = 80;
    const colWidth = 800 / weeks; // ~15px
    const rowHeight = 480 / years; // 6px
    const dotWidth = colWidth - 1;
    const dotHeight = rowHeight - 1;

    for (let week = 0; week < weeks; week++) {
        for (let year = 0; year < years; year++) {
            const weekIndex = year * weeks + week;
            const x = week * colWidth;
            const y = year * rowHeight;

            if (weekIndex < weeksLived) {
                ctx.fillStyle = '#000000';
                ctx.fillRect(x, y, dotWidth, dotHeight);
            } else if (weekIndex < totalWeeks) {
                ctx.strokeStyle = '#f0f0f0';
                ctx.lineWidth = 1;
                ctx.strokeRect(x + 0.5, y + 0.5, dotWidth - 1, dotHeight - 1);
            }
        }
    }
}
