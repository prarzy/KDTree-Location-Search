const data = require('./coordinates.json');
const fs = require('fs');

const csv = data.map(d => `${d.amenity},"${d.name}",${d.lon},${d.lat}`).join('\n');
fs.writeFileSync('coordinates.csv', csv);
console.log('CSV file created successfully');
