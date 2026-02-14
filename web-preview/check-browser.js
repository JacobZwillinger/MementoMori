// Quick check if the main index.html would work
const fs = require('fs');

console.log('Checking files...\n');

const files = ['index.html', 'memento.js', 'config.json', 'styles.css'];
files.forEach(file => {
    if (fs.existsSync(file)) {
        const size = fs.statSync(file).size;
        console.log(`✓ ${file} exists (${size} bytes)`);
    } else {
        console.log(`✗ ${file} MISSING`);
    }
});

console.log('\nTesting config load...');
const config = JSON.parse(fs.readFileSync('config.json', 'utf8'));
console.log(`✓ Config loaded: ${config.specialDays.length} special days`);
console.log(`✓ Birthdate: ${config.person.birthdate}`);
